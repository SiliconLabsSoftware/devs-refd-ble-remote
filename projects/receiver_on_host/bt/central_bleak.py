import asyncio
import threading
import atexit
import time
from typing import Optional, List, Callable
from bleak import BleakScanner, BleakClient
from bt.central import BtCentral
from platform_handler import Platform

class BtCentralBleak(BtCentral):
    """Synchronous wrapper for asynchronous bleak library. Runs async methods in separate event loop."""

    def __init__(self):
      try:
        from log import get_logger
        self._logger = get_logger(__name__)
      except ImportError:
        import logging
        self._logger = logging.getLogger(__name__)

      self._event_loop = asyncio.new_event_loop()
      self._client: Optional[BleakClient] = None
      self._thread = threading.Thread(target=self._event_loop.run_forever, daemon=True)
      self._thread.start()
      atexit.register(self._shutdown)

    def _run_async(self, coroutine):
        """Run asynchronous coroutine in event loop. """
        future = asyncio.run_coroutine_threadsafe(coroutine, self._event_loop)
        try:
            return future.result(timeout=30.0)
        except Exception as e:
            # Import here to avoid circular imports
            try:
                from bleak.exc import BleakCharacteristicNotFoundError, BleakError
                if isinstance(e, BleakCharacteristicNotFoundError):
                    self._logger.error("BLE characteristic not found: %s", e)
                elif isinstance(e, BleakError):
                    self._logger.error("BLE operation failed: %s", e)
                else:
                    self._logger.error("Async operation failed or timed out: %s", e)
            except ImportError:
                self._logger.error("Async operation failed or timed out: %s", e)
            raise

    def scan(self, timeout: float) -> Optional[List[BtCentral.ScanResult]]:
        """Scan for BLE devices with macOS-compatible retry logic. Uses shorter intervals on macOS. """
        async def _scan():
            scan_interval = Platform.get_scan_interval(timeout)
            min_scan_time = Platform.get_min_scan_time()
            scan_interval = max(min_scan_time, scan_interval)
            total_timeout = timeout
            start_time = time.time()
            all_devices = {}  # Use dict to avoid duplicates
            scan_count = 0

            self._logger.info("Starting BLE scan for   %s on %s (using   %s intervals)",
                            total_timeout, Platform.get_name(), scan_interval)

            while time.time() - start_time < total_timeout:
                try:
                    # Perform a short scan
                    remaining_time = total_timeout - (time.time() - start_time)
                    current_timeout = min(scan_interval, remaining_time)

                    if current_timeout < min_scan_time:
                        self._logger.debug("Remaining time %.1fs too short, ending scan", current_timeout)
                        break

                    scan_count += 1
                    self._logger.debug("BLE scan attempt #%s with timeout: %.1fs", scan_count, current_timeout)

                    devices = await BleakScanner.discover(
                        timeout=current_timeout,
                        return_adv=False
                    )

                    new_devices = 0
                    for device in devices:
                        if device.address not in all_devices:
                            all_devices[device.address] = device
                            new_devices += 1
                            self._logger.info("Found new device: %s - %s", device.address, device.name)

                    if new_devices > 0:
                        self._logger.info("Scan #%s: found %s new devices (total: %s)",
                                         scan_count, new_devices, len(all_devices))
                    else:
                        self._logger.info("Scan #%s: no new devices found", scan_count)

                except Exception as e:
                    self._logger.warning("BLE scan #%s failed: %s", scan_count, e)
                remaining_time = total_timeout - (time.time() - start_time)
                if remaining_time >= min_scan_time:
                    await asyncio.sleep(0.2)

            results = [BtCentral.ScanResult(device.address, device.name)
                      for device in all_devices.values()]
            self._logger.info("BLE scan completed after %s attempts. Found %s unique devices in %.1fs.",
                            scan_count, len(results), time.time() - start_time)
            return results

        try:
            return self._run_async(_scan())
        except Exception as e:
            self._logger.error("BLE scan completely failed: %s", e)
            return []

    def connect(self, address: str, disconnected_callback: Callable[[], None]) -> bool:
        """Connect to BLE device.

        Args:
            address: Address of BLE device to connect to
            disconnected_callback: Callback invoked when device disconnects

        Returns:
            True if connection successful, False otherwise
        """
        async def _connect():
            # Ensure any existing connection is fully disconnected first
            if self._client is not None:
                try:
                    if self._client.is_connected:
                        self._logger.debug("Disconnecting existing client before connecting to %s", address)
                        await self._client.disconnect()
                    self._client = None
                except Exception as e:
                    self._logger.warning("Error disconnecting existing client: %s", e)
                    self._client = None

            # Create new client and connect
            try:
                self._client = BleakClient(
                    address,
                    disconnected_callback=lambda dummy: disconnected_callback()
                    if disconnected_callback else None,
                )
                await self._client.connect()
                return self._client.is_connected
            except Exception as e:
                self._logger.error("BLE operation failed: %s", e)
                self._client = None
                return False
        return self._run_async(_connect())

    def disconnect(self):
        """Disconnect from BLE device. Ensures proper cleanup."""
        async def _disconnect():
            if self._client is not None:
                try:
                    if self._client.is_connected:
                        await self._client.disconnect()
                except Exception as e:
                    self._logger.warning("Error during disconnect: %s", e)
                finally:
                    self._client = None
                    self._logger.debug("Client cleaned up after disconnect")

        self._run_async(_disconnect())

    def is_connected(self) -> bool:
        return self._client and self._client.is_connected

    def read(self, uuid: str) -> Optional[bytes]:
        async def _read():
            return await self._client.read_gatt_char(uuid)
        return self._run_async(_read()) if self.is_connected() else None

    def write(self, uuid: str, data: bytes) -> None:
        async def _write():
            await self._client.write_gatt_char(uuid, data)
        if self.is_connected():
            self._run_async(_write())

    def notify(self, uuid: str, handler: Callable[[str, bytes], None]) -> None:
        """Enable or disable notifications for BLE characteristic. """
        async def _notify():
            if handler:
                await self._client.start_notify(
                    uuid, lambda _uuid, _data: handler(_uuid.uuid, _data)
                )
            else:
                await self._client.stop_notify(uuid)
        if self.is_connected():
            self._run_async(_notify())

    def _shutdown(self):
        """Shut down event loop and clean up resources. Disconnects client and stops event loop thread."""
        if self.is_connected():
            self.disconnect()
        self._event_loop.call_soon_threadsafe(self._event_loop.stop)
        self._thread.join()
