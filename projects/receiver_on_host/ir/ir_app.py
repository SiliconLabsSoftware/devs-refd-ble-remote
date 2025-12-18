import threading
from typing import Optional, Callable, Union
from dataclasses import dataclass, field
from enum import Enum, auto

from ir.receiver import IrReceiver
from ir.receiver_flirc_irtool import IrReceiverFlircIrTool
from ui.ui import EventType
from ui.event_bus import EventBus
from log import get_logger
from config import Config

logger = get_logger(__name__)


class IrState(Enum):
    UNINITIALIZED = auto()
    INITIALIZED = auto()
    RUNNING = auto()
    STOPPED = auto()
    ERROR = auto()


@dataclass
class IrStatus:
    """Thread-safe IR status container. Auto-publishes events on state changes."""
    _lock: threading.Lock = field(default_factory=threading.Lock)
    _state: IrState = IrState.UNINITIALIZED
    _enabled: bool = False
    _is_running: bool = False
    _event_bus: Optional[EventBus] = None

    def __post_init__(self):
        """Initialize lock after dataclass creation."""
        if self._lock is None:
            self._lock = threading.Lock()

    def set_event_bus(self, event_bus: Optional[EventBus]):
        with self._lock:
            self._event_bus = event_bus

    @property
    def state(self) -> IrState:
        with self._lock:
            return self._state

    @state.setter
    def state(self, value: IrState):
        with self._lock:
            if self._state == value:
                return
            old_state = self._state
            self._state = value
            logger.debug("IrStatus.state changed: %s -> %s", old_state, value)
            self._update_enabled_running()
            event_bus = self._event_bus
            enabled = self._enabled
            is_running = self._is_running

        # Publish outside the lock to avoid deadlocks
        # UI is guaranteed to be ready when set_ui() is called, so we can always publish
        self._publish_status(event_bus, enabled, is_running)

    @property
    def enabled(self) -> bool:
        with self._lock:
            return self._enabled

    @property
    def is_running(self) -> bool:
        with self._lock:
            return self._is_running

    def _update_enabled_running(self):
        """Update enabled and is_running based on state.

        NOTE: This method assumes the lock is already held by the caller.
        Do NOT acquire the lock here to avoid deadlocks.
        """
        self._enabled = self._state in (IrState.INITIALIZED, IrState.RUNNING, IrState.STOPPED)
        self._is_running = self._state == IrState.RUNNING

    def _publish_status(self, event_bus: Optional[EventBus], enabled: bool, is_running: bool):
        """Publish status via event bus if available.

        Args:
            event_bus: Event bus instance to publish to
            enabled: Current enabled status
            is_running: Current running status
        """
        if not event_bus:
            return

        try:
            logger.debug("Publishing IR_BUTTONS_ENABLED: enabled=%s, is_running=%s", enabled, is_running)
            event_bus.publish(EventType.IR_BUTTONS_ENABLED, {
                'enabled': enabled,
                'is_running': is_running
            })
        except Exception as e:
            logger.warning("Error publishing IR_BUTTONS_ENABLED: %s", e, exc_info=True)


class IrEventHandlers:

    def __init__(self,
                 event_bus: Optional[EventBus] = None,
                 button_highlight_callback: Optional[Callable[[int], None]] = None):

        self.event_bus = event_bus
        self.button_highlight_callback = button_highlight_callback

    def handle_key_event(self, pressed: bool, key_code: int):

        logger.info("IR key event: pressed=%s, key_code=%s (0x%02x)", pressed, key_code, key_code)

        if self.event_bus:
            logger.debug("Publishing IR_KEY_EVENT: button_id=%s, pressed=%s", key_code, pressed)
            self.event_bus.publish(EventType.IR_KEY_EVENT, {
                'button_id': key_code,
                'pressed': pressed,
                'source': 'ir'
            })

        if pressed and self.button_highlight_callback:
            try:
                logger.debug("Calling button highlight callback for key_code: %s", key_code)
                self.button_highlight_callback(key_code)
            except Exception as e:
                logger.error("Error in button highlight callback: %s", e, exc_info=True)

    def handle_status_update(self, message: str):
        logger.info("IR Status: %s", message)

class IrApp:
    """Manages IR receiver initialization and control."""

    @classmethod
    def create(cls,
               config: Config,
               event_bus: Optional[EventBus] = None,
               button_highlight_callback: Optional[Callable[[int], None]] = None) -> 'IrApp':

        ir_receiver = IrReceiverFlircIrTool()
        status = IrStatus()
        if event_bus:
            status.set_event_bus(event_bus)
        event_handlers = IrEventHandlers(event_bus=event_bus, button_highlight_callback=button_highlight_callback)
        return cls(
            config=config,
            ir_receiver=ir_receiver,
            status=status,
            event_handlers=event_handlers,
            event_bus=event_bus
        )

    def __init__(self,
                 config: Config,
                 ir_receiver: Union[IrReceiver, IrReceiverFlircIrTool],
                 status: IrStatus,
                 event_handlers: IrEventHandlers,
                 event_bus: Optional[EventBus] = None):

        self.config = config
        self.ir_receiver = ir_receiver
        self.status = status
        self.event_handlers = event_handlers
        self.event_bus = event_bus
        self._ir_key_callback = None

    def _initialize_ir_handler(self):
        """Initialize IR receiver and set up UI integration."""
        try:
            logger.debug("Initializing IR handler...")

            def ir_key_callback(pressed: bool, key_code: int):
                """Handle IR key events from the receiver."""
                self.event_handlers.handle_key_event(pressed, key_code)

            if isinstance(self.ir_receiver, IrReceiverFlircIrTool):
                self.ir_receiver.status_callback = self.event_handlers.handle_status_update

            self._ir_key_callback = ir_key_callback

            if self.event_bus:
                logger.debug("Publishing IR_HANDLER_READY event")
                self.event_bus.publish(EventType.IR_HANDLER_READY, {'ir_receiver': self.ir_receiver})

            self.status.state = IrState.INITIALIZED
            logger.info("IR receiver initialized successfully")
        except Exception as e:
            logger.warning("IR receiver initialization failed: %s", e, exc_info=True)
            self.status.state = IrState.ERROR

    def _ir_start(self) -> bool:
        if self.status.state == IrState.ERROR:
            logger.warning("IR receiver is in error state, cannot start")
            return False

        if self.status.state == IrState.RUNNING:
            logger.debug("IR receiver already running")
            return True

        try:
            callback = getattr(self, '_ir_key_callback', None)
            if not callback:
                logger.warning("No IR key callback available")
                return False

            logger.debug("Starting IR receiver...")
            self.ir_receiver.start(callback)

            self.status.state = IrState.RUNNING

            logger.info("IR start successful")
            return True
        except Exception as e:
            logger.error("Failed to start IR: %s", e, exc_info=True)
            self.status.state = IrState.ERROR
            return False

    def _ir_stop(self) -> bool:
        logger.debug("IrApp._ir_stop() called")

        if self.status.state != IrState.RUNNING:
            logger.debug("IR receiver not running (state: %s)", self.status.state)
            return False

        try:
            self.ir_receiver.stop()
            self.status.state = IrState.STOPPED
            logger.info("IR stop successful")
            return True
        except Exception as e:
            logger.error("Failed to stop IR: %s", e, exc_info=True)
            self.status.state = IrState.ERROR
            return False
