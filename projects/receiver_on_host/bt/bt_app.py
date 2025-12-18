import os
import sys
import threading
import time
from pathlib import Path
from typing import List, Optional, Dict, TYPE_CHECKING
from audio.audio import Audio
from audio.audio_app import AudioState
from bt.central import BtCentral
from log import get_logger
from config import Config
from ui.ui import EventType
from ui.event_bus import EventBus
from validation import validate_device_address, ValidationError
from errors import BleScanError, BleConnectionError, BleConfigurationError

if TYPE_CHECKING:
    from audio.audio_app import AudioApp

logger = get_logger(__name__)


class BtApp:
    """Manages BLE device scanning, connection, and notification handling. Routes BLE notifications to appropriate handlers."""

    @classmethod
    def create(cls,
               audio_app: 'AudioApp',
               config: Config,
               scan_timeout: float,
               remote_adv_name: str,
               remote_characteristics: Dict[str, str],
               event_bus: Optional[EventBus] = None) -> 'BtApp':
        from bt.central_proxy import BtCentralProxy
        from bt.central_bleak import BtCentralBleak

        bt_central = BtCentralProxy(BtCentralBleak())
        logger.debug("BtApp dependencies created successfully")
        return cls(
            bt=bt_central,
            config=config,
            audio=audio_app.audio,
            state=audio_app.state,
            scan_timeout=scan_timeout,
            remote_adv_name=remote_adv_name,
            remote_characteristics=remote_characteristics,
            event_bus=event_bus
        )

    def __init__(self,
                 bt: BtCentral,
                 config: Config,
                 audio: Audio,
                 state: AudioState,
                 scan_timeout: float,
                 remote_adv_name: str,
                 remote_characteristics: Dict[str, str],
                 event_bus: Optional[EventBus] = None):
        """Initialize BtApp with explicit dependencies.

        Args:
            bt: BLE central interface
            config: Configuration object
            audio: Audio processing object
            state: Shared AudioState object for thread-safe state management
            scan_timeout: Timeout for BLE scanning (validated at composition root)
            remote_adv_name: Name of the target BLE device (validated at composition root)
            remote_characteristics: Dictionary of BLE characteristic UUIDs (validated at composition root)
            event_bus: Optional event bus for publishing events
        """
        self.bt = bt
        self.config = config
        self.audio = audio
        self.state = state
        self.scan_timeout = scan_timeout
        self.remote_adv_name = remote_adv_name.strip() if isinstance(remote_adv_name, str) else remote_adv_name
        self.remote_characteristics = remote_characteristics
        self.event_bus = event_bus
        self._last_scan_results: Dict[str, str] = {}

    def scan(self) -> List[str]:
        """Scan for BLE devices and return addresses matching remote_adv_name.

        Returns:
            List of device addresses that match the target device name
        """
        try:
            logger.debug("BtApp.scan() called - looking for device: '%s'", self.remote_adv_name)
            logger.info("Starting BLE scan with timeout:   %ss", self.scan_timeout)
            scan_results = self.bt.scan(self.scan_timeout)
            logger.debug("bt.scan() returned %d devices", len(scan_results) if scan_results else 0)

            if scan_results:
                # Store all scan results for debugging
                self._last_scan_results = {result.address: result.name for result in scan_results}
                logger.debug("Stored %d scan results for lookup", len(self._last_scan_results))
                logger.debug("All scanned devices: %s", [(addr, name) for addr, name in self._last_scan_results.items()])

                # Filter to only include target devices
                target_devices = []

                for result in scan_results:
                    logger.debug("Checking device: address=%s, name='%s'", result.address, result.name)
                    if result.name == self.remote_adv_name:
                        target_devices.append(result.address)
                        logger.info("Found target device '%s': %s", self.remote_adv_name, result.address)

                logger.info("Found %d total devices, %d matching target name", len(scan_results), len(target_devices))
                if target_devices:
                    logger.info("Target device addresses: %s", target_devices)
                else:
                    logger.warning("No devices found with name '%s'", self.remote_adv_name)

                return target_devices
            else:
                logger.info("No devices found during scan (this is a valid result)")
                return []
        except BleScanError:
            raise
        except Exception as e:
            logger.error("Exception in scan: %s: %s", type(e).__name__, e)
            raise BleScanError(f"BLE scan failed: {e}") from e

    def get_device_name(self, address: str) -> str:
        if address in self._last_scan_results:
            name = self._last_scan_results[address]
            return name if name else address
        return address

    def on_ble_notification(self, uuid: str, data: bytes):
        """Handle BLE notifications from remote device. Routes to appropriate handlers based on UUID.

        Args:
            uuid: UUID of BLE characteristic that triggered notification
            data: Data payload of notification
        """
        try:
            logger.info('NOTIFICATION: %s - %s', uuid, data.hex())
            if uuid == self.remote_characteristics['Battery']:
                logger.debug('Processing Battery notification: %sV', int.from_bytes(data, "little") / 1_000_000.0)
                if self.event_bus:
                    self.event_bus.publish(EventType.BATTERY_VOLTAGE, int.from_bytes(data, 'little') / 1_000_000.0)
            elif uuid == self.remote_characteristics['AudioData']:
                logger.info('Processing AudioData notification')
                logger.debug('AudioData packet size: %d bytes, hex: %s...', len(data), data.hex()[:32])
                logger.debug('Current AudioState.recording_active: %s', self.state.recording_active)
                try:
                    logger.debug("Calling audio.process()...")
                    self.audio.process(data)
                    stats = self.audio.get_statistics()
                    logger.debug('Audio statistics - frames_processed: %d, frames_lost: %d', stats.frames_processed, stats.frames_lost)
                except Exception as e:
                    logger.error('Exception type: %s, message: %s', type(e).__name__, str(e))
                    return

                try:
                    if self.event_bus:
                        logger.debug("Publishing audio frame statistics via event bus")
                        stats = self.audio.get_statistics()
                        self.event_bus.publish(EventType.AUDIO_FRAMES_RECEIVED, stats.frames_processed)
                        self.event_bus.publish(EventType.AUDIO_FRAMES_LOST, stats.frames_lost)
                except Exception as e:
                    logger.error('Exception type: %s, message: %s', type(e).__name__, str(e))

                try:
                    if not self.state.recording_active:
                        logger.debug("Setting AudioState.recording_active = True (first audio packet)")
                        self.state.recording_active = True
                    else:
                        logger.debug("AudioState.recording_active already True - continuing recording")
                except Exception as e:
                    logger.error('Error updating AudioState.recording_active: %s', e)
                    logger.debug('Exception type: %s, message: %s', type(e).__name__, str(e))
            elif uuid == self.remote_characteristics['TransferStatus']:
                if len(data) < 1:
                    logger.warning('TransferStatus notification insufficient length (length=%d, expected >=1)', len(data))
                    return
                transfer_status = data[0]
                logger.info('Processing TransferStatus notification: %s', transfer_status)
                logger.debug('Current AudioState - recording_active: %s, has_received_audio: %s', self.state.recording_active, self.state.has_received_audio)

                if 0 == transfer_status:
                    logger.debug("TransferStatus=0: Audio recording stopped")
                    try:
                        logger.debug("Calling audio.reset()...")
                        self.audio.reset()
                        logger.debug("audio.reset() completed successfully")
                    except Exception as e:
                        logger.error('Exception type: %s, message: %s', type(e).__name__, str(e))

                    try:
                        if self.state.recording_active:
                            logger.debug("Setting AudioState.recording_active = False")
                            self.state.recording_active = False
                            logger.debug("Setting AudioState.has_received_audio = True")
                            self.state.has_received_audio = True
                        else:
                            logger.debug("AudioState.recording_active already False")
                    except Exception as e:
                        logger.error('Exception type: %s, message: %s', type(e).__name__, str(e))
                else:
                    logger.debug("TransferStatus=%s: Audio recording started", transfer_status)
                    if not self.state.recording_active:
                        logger.debug("Setting AudioState.recording_active = True")
                        self.state.recording_active = True
                    else:
                        logger.debug("AudioState.recording_active already True")
                if self.event_bus:
                    logger.debug("Publishing AUDIO_STATUS event: %s", transfer_status)
                    self.event_bus.publish(EventType.AUDIO_STATUS, transfer_status)
            elif uuid == self.remote_characteristics['KeyStatus']:
                if len(data) < 2:
                    logger.warning('KeyStatus notification with insufficient data (length=%d, expected >=2)', len(data))
                    return
                key_id = data[0]
                key_status = data[1]
                logger.info('Processing KeyStatus notification - Key ID: %s, Status: %s', key_id, key_status)
                logger.debug('KeyStatus details - Key ID: %d (0x%02x), Status: %d (1=new press, 0=release), Data length: %d, Full data: %s', key_id, key_id, key_status, len(data), data.hex())

                # Filter out button 255 (0xFF) - it's a "release all" signal, not a real button
                if key_id != 0xFF and key_id != 255:
                    if self.event_bus:
                        logger.debug("Publishing KEYS_ID event: %s", key_id)
                        self.event_bus.publish(EventType.KEYS_ID, key_id)
                        logger.debug("Publishing KEYS_STATUS event: %s", key_status)
                        self.event_bus.publish(EventType.KEYS_STATUS, key_status)
                        logger.debug("KeyStatus events published - UI should highlight button")
                else:
                    logger.debug("Ignoring button 255 (0xFF) - release all signal, bitmap will handle state")
                    # Still publish KEYS_STATUS for button 255 to maintain compatibility
                    if self.event_bus:
                        logger.debug("Publishing KEYS_STATUS event: %s", key_status)
                        self.event_bus.publish(EventType.KEYS_STATUS, key_status)

                if len(data) >= 10:
                    bitmap = int.from_bytes(data[2:10], 'little')
                    logger.info('Publishing key bitmap: %016x', bitmap)
                    if self.event_bus:
                        logger.debug("Publishing KEYS_BITMAP event")
                        self.event_bus.publish(EventType.KEYS_BITMAP, bitmap)
                else:
                    logger.debug("KeyStatus data too short for bitmap (length=%d, need >=10)", len(data))
            else:
                logger.warning('UNSUPPORTED UUID: %s!', uuid)
        except Exception as e:
            logger.error('Error processing BLE notification: %s', e)

    def config_audio(self):
        logger.info('Configuring audio stream...')
        try:
            sample_rate_khz = int(self.audio.config.sample_rate / 1000)
            logger.debug("Audio config - sample_rate: %d Hz (%d kHz), encoding: %d, channels: %d", self.audio.config.sample_rate, sample_rate_khz, self.audio.config.encoding, self.audio.config.channels)

            logger.debug("Writing SampleRate: %d to characteristic %s", sample_rate_khz, self.remote_characteristics['SampleRate'])
            self.bt.write(self.remote_characteristics['SampleRate'], sample_rate_khz.to_bytes(1, 'little'))

            logger.debug("Writing EncodingEnable: %d to characteristic %s", self.audio.config.encoding, self.remote_characteristics['EncodingEnable'])
            self.bt.write(self.remote_characteristics['EncodingEnable'], self.audio.config.encoding.to_bytes(1, 'little'))

            logger.debug("Writing AudioChannels: %d to characteristic %s", self.audio.config.channels, self.remote_characteristics['AudioChannels'])
            self.bt.write(self.remote_characteristics['AudioChannels'], self.audio.config.channels.to_bytes(1, 'little'))

            logger.debug("Enabling notifications for AudioData: %s", self.remote_characteristics['AudioData'])
            self.bt.notify(self.remote_characteristics['AudioData'], self.on_ble_notification)

            logger.debug("Enabling notifications for TransferStatus: %s", self.remote_characteristics['TransferStatus'])
            self.bt.notify(self.remote_characteristics['TransferStatus'], self.on_ble_notification)
            logger.debug("Audio configuration complete")
        except Exception as e:
            error_msg = f"Failed to configure audio stream: {e}"
            logger.error(error_msg)
            raise BleConfigurationError(error_msg) from e

    def config_keys(self):
        logger.info('Enabling remote key notifications...')
        try:
            logger.debug("Enabling notifications for KeyStatus: %s", self.remote_characteristics['KeyStatus'])
            self.bt.notify(self.remote_characteristics['KeyStatus'], self.on_ble_notification)
            logger.debug("Key notifications enabled")
        except Exception as e:
            error_msg = f"Failed to configure key notifications: {e}"
            logger.error(error_msg)
            raise BleConfigurationError(error_msg) from e

    def config_battery(self):
        logger.info('Measuring and enabling battery voltage notification...')
        try:
            logger.debug("Reading battery voltage from characteristic: %s", self.remote_characteristics['Battery'])
            raw_data = self.bt.read(self.remote_characteristics['Battery'])
            if raw_data:
                logger.debug("Battery raw data: %s", raw_data.hex())
                voltage = int.from_bytes(raw_data, 'little') / 1000000.0
                logger.debug("Battery voltage calculated: %sV", voltage)
                if self.event_bus:
                    logger.debug("Publishing BATTERY_VOLTAGE event: %sV", voltage)
                    self.event_bus.publish(EventType.BATTERY_VOLTAGE, voltage)
            else:
                logger.warning("Failed to read battery data - received None")
            logger.debug("Enabling notifications for Battery: %s", self.remote_characteristics['Battery'])
            self.bt.notify(self.remote_characteristics['Battery'], self.on_ble_notification)
            logger.debug("Battery configuration complete")
        except Exception as e:
            error_msg = f"Failed to configure battery notifications: {e}"
            logger.error(error_msg)
            raise BleConfigurationError(error_msg) from e

    def connect(self, device_address: str) -> None:
        try:
            device_address = validate_device_address(device_address)
        except ValidationError as e:
            logger.error("Invalid device address: %s", e)
            raise

        logger.info('Attempting to connect to device: %s', device_address)

        if self.event_bus:
            logger.debug("Creating disconnected callback for event bus publishing")
            event_bus = self.event_bus  # Capture for lambda
            disconnected_callback = lambda: event_bus.publish(EventType.DISCONNECTED, device_address)
        else:
            # Provide a no-op callback if no event bus
            disconnected_callback = lambda: None

        success = self.bt.connect(device_address, disconnected_callback)
        logger.debug("bt.connect() returned: %s", success)

        if not success:
            error_msg = f"Failed to connect to device: {device_address}"
            logger.error(error_msg)
            raise BleConnectionError(error_msg)

        try:
            logger.debug("Connection successful - configuring device...")
            self.state.reset()

            if self.event_bus:
                logger.debug("Resetting AI button state via event bus")
                self.event_bus.publish(EventType.AI_BUTTON_STATE_CHANGED, {'enabled': False})

            logger.debug("Configuring keys...")
            self.config_keys()
            logger.debug("Configuring audio...")
            self.config_audio()
            logger.debug("Configuring battery...")
            self.config_battery()
            logger.debug("All device configuration complete")

            if self.event_bus:
                logger.debug("Publishing CONNECTED event via event bus")
                self.event_bus.publish(EventType.CONNECTED, device_address)
            logger.info('Successfully connected to device: %s', device_address)
        except BleConfigurationError:
            raise
        except Exception as e:
            logger.error('Failed to configure device %s: %s', device_address, e)
            try:
                self.bt.disconnect()
            except Exception as disconnect_error:
                logger.warning("Error disconnecting after configuration failure: %s", disconnect_error)
            raise BleConfigurationError(f"Failed to configure device {device_address}: {e}") from e
