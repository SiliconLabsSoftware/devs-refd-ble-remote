import os
import sys
import threading
import time
from pathlib import Path
from typing import List, Optional, Dict
from dataclasses import dataclass
from audio.audio_app import AudioApp
from bt.bt_app import BtApp
from ir.ir_app import IrApp
from ai.ai_app import AiApp
from ui.ui import EventHandlers, EventType
from ui.tkinter.ui_tkinter import UiTkinter
from ui.event_bus import EventBus
from log import get_logger
from config import Config
from platform_handler import Platform
from validation import validate_device_address, ValidationError
from errors import BleConnectionError, BleConfigurationError, BleScanError

# Initialize logger
logger = get_logger(__name__)


@dataclass
class AppDependencies:
    """Container for all App dependencies."""
    config: Config
    audio_file_path: str
    scan_timeout: float
    remote_adv_name: str
    remote_characteristics: Dict[str, str]
    event_bus: EventBus


class App:
    """Main application coordinator. Orchestrates BLE, Audio, IR, and AI domain apps."""

    def __init__(self, deps: AppDependencies):
        """
        Initialize App with dependencies and create domain apps.

        Args:
            deps: AppDependencies containing all required dependencies

        Raises:
            RuntimeError: If Python version is below 3.11.0
        """
        Platform.check_python_version_requirement(3, 11)
        logger.debug("Python version check passed: %s", Platform.get_python_version_string())

        self.config = deps.config
        self.audio_file_path = deps.audio_file_path
        self.scan_timeout = deps.scan_timeout
        self.remote_adv_name = deps.remote_adv_name
        self.remote_characteristics = deps.remote_characteristics
        self.event_bus = deps.event_bus
        self.ui = None

        logger.info("Audio file will be saved to: %s", self.audio_file_path)
        logger.info("Using platform-specific scan timeout: %s", self.scan_timeout)

        logger.debug("Creating domain apps with factory methods...")
        logger.debug("Creating AudioApp...")
        self.audio_app = AudioApp.create(
            audio_file_path=self.audio_file_path,
            event_bus=self.event_bus
        )
        logger.debug("AudioApp created successfully")

        logger.debug("Creating BtApp...")
        self.bt_app = BtApp.create(
            audio_app=self.audio_app,
            config=self.config,
            scan_timeout=self.scan_timeout,
            remote_adv_name=self.remote_adv_name,
            remote_characteristics=self.remote_characteristics,
            event_bus=self.event_bus
        )
        logger.debug("BtApp created successfully")

        logger.debug("Creating IrApp...")
        self.ir_app = IrApp.create(
            config=self.config,
            event_bus=self.event_bus
        )
        logger.debug("IrApp created successfully")

        logger.debug("Creating AiApp...")
        self.ai_app = AiApp.create(
            audio_app=self.audio_app,
            config=self.config,
            event_bus=self.event_bus
        )
        logger.debug("AiApp created successfully")
        logger.debug("All domain apps initialized")

    def run(self):
        logger.debug("Starting application run() method")

        logger.debug("Creating EventHandlers with domain app delegates")
        event_handlers = EventHandlers(
            scan=self.scan,
            connect=self.connect,
            disconnect=self.bt_app.bt.disconnect,
            audio_play=self._audio_play,
            audio_stop=self._audio_stop,
            ir_start=self._ir_start,
            ir_stop=self._ir_stop,
            ai_analyze=self._ai_analyze
        )
        logger.debug("EventHandlers created")

        logger.debug("Creating UI (UiTkinter)...")
        self.ui = UiTkinter(event_handlers, event_bus=self.event_bus)
        logger.debug("UI created successfully")

        logger.debug("Initializing IR handler (after UI creation)...")
        self.ir_app._initialize_ir_handler()
        logger.debug("IR handler initialized")

        if hasattr(self.ui, 'ir_pane'):
            logger.debug("Setting IR handler directly in UI (fallback)")
            self.ui.ir_pane.set_ir_handler(self.ir_app.ir_receiver)

        logger.debug("Publishing initial audio recording status")
        self.event_bus.publish(EventType.AUDIO_RECORDING_STATUS, "waiting for audio transmission")


        logger.debug("Checking IR receiver status: running=%s", self.ir_app.ir_receiver.is_running)
        if not self.ir_app.ir_receiver.is_running:
            logger.debug("Auto-starting IR receiver...")
            self._ir_start()
            logger.debug("IR receiver auto-start completed")
        else:
            logger.debug("IR receiver not auto-started (not available or already running)")

        logger.debug("Starting UI main loop...")
        self.ui.run()

    def scan(self) -> List[str]:
        result = self.bt_app.scan()
        return result

    def connect(self, device_address: str) -> bool:
        """Connect to a BLE device.

        Args:
            device_address: The address of the BLE device to connect to.

        Raises:
            ValidationError: If device_address format is invalid
            BleConnectionError: If connection fails
            BleConfigurationError: If device configuration fails after successful connection
        """
        # Validate device address at entry point
        try:
            device_address = validate_device_address(device_address)
        except ValidationError as e:
            logger.error("Invalid device address in App.connect(): %s", e)
            raise

        logger.debug("App.connect() called for device: %s", device_address)
        try:
            self.bt_app.connect(device_address)
            return True
        except (BleConnectionError, BleConfigurationError) as e:
            logger.error("Connection failed: %s", e)
            raise

    def _audio_play(self):
        self.audio_app._audio_play()

    def _audio_stop(self):
        self.audio_app._audio_stop()

    def _ir_start(self) -> bool:
        result = self.ir_app._ir_start()
        return result

    def _ir_stop(self) -> bool:
        result = self.ir_app._ir_stop()
        return result

    def _ai_analyze(self):
        self.ai_app._ai_analyze()

def create_app_dependencies() -> AppDependencies:
    """Create all dependencies for App. Composition root for dependency injection.

    Returns:
        AppDependencies: Container with all initialized dependencies

    Raises:
        RuntimeError: If Python version is below 3.11.0
        ValidationError: If configuration values are invalid
    """
    Platform.check_python_version_requirement(3, 11)

    logger.debug("Loading configuration...")
    config = Config()

    audio_filename = config.get("audio_file_path")
    executable_dir = Platform.get_executable_directory()
    audio_file_path = str(executable_dir / audio_filename)
    scan_timeout = Platform.get_scan_timeout(config)
    remote_adv_name = config.get("remote_adv_name")
    remote_characteristics = config.get("remote_characteristics")

    from validation import validate_scan_timeout
    try:
        scan_timeout = validate_scan_timeout(scan_timeout)
    except ValidationError as e:
        logger.error("Invalid scan_timeout from configuration: %s", e)
        raise

    if not remote_adv_name or not isinstance(remote_adv_name, str):
        raise ValidationError("remote_adv_name must be a non-empty string in config")

    if not remote_characteristics or not isinstance(remote_characteristics, dict):
        raise ValidationError("remote_characteristics must be a non-empty dictionary in config")

    logger.info("Audio file will be saved to: %s", audio_file_path)
    logger.info("Using platform-specific scan timeout:   %ss", scan_timeout)

    event_bus = EventBus()

    return AppDependencies(
        config=config,
        audio_file_path=audio_file_path,
        scan_timeout=scan_timeout,
        remote_adv_name=remote_adv_name,
        remote_characteristics=remote_characteristics,
        event_bus=event_bus
    )
