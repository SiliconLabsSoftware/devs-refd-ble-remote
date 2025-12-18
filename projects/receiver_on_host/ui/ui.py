import asyncio
import threading
import atexit
from abc import ABC, abstractmethod
from typing import Dict, List, Callable, Any
from enum import Enum, auto
from log import get_logger

logger = get_logger(__name__)

class EventHandlers:
    """Collection of callable event handlers for the UI."""
    def __init__(self,
                 scan: Callable[[], List[str]] = None,
                 connect: Callable[[str], bool] = None,
                 disconnect: Callable[[], None] = None,
                 audio_play: Callable[[], None] = None,
                 audio_stop: Callable[[], None] = None,
                 ir_start: Callable[[], bool] = None,
                 ir_stop: Callable[[], bool] = None,
                 ai_analyze: Callable[[], None] = None):
        self.scan = scan
        self.connect = connect
        self.disconnect = disconnect
        self.audio_play = audio_play
        self.audio_stop = audio_stop
        self.ir_start = ir_start
        self.ir_stop = ir_stop
        self.ai_analyze = ai_analyze

class EventType(Enum):
    """Asynchronous event types for the UI and event bus."""
    CONNECTED = auto()
    DISCONNECTED = auto()
    BATTERY_VOLTAGE = auto()
    AUDIO_STATUS = auto()
    AUDIO_FRAMES_RECEIVED = auto()
    AUDIO_FRAMES_LOST = auto()
    KEYS_ID = auto()
    KEYS_STATUS = auto()
    KEYS_BITMAP = auto()
    AUDIO_RECORDING_STATUS = auto()
    IR_STATUS = auto()
    IR_BUTTONS_ENABLED = auto()
    IR_KEY_EVENT = auto()
    AI_ANALYSIS = auto()
    AI_ERROR = auto()
    AI_BUTTON_STATE_CHANGED = auto()
    AUDIO_PLAYING_STATE = auto()
    AUDIO_STOPPED_STATE = auto()
    IR_HANDLER_READY = auto()

class ButtonId(Enum):
    """Button ID enumeration for remote control buttons."""
    POWER = 47
    NUM_1 = 7
    NUM_2 = 31
    NUM_3 = 39
    NUM_4 = 14
    NUM_5 = 23
    NUM_6 = 46
    NUM_7 = 6
    NUM_8 = 30
    NUM_9 = 38
    DOT = 13
    NUM_0 = 22
    APPS = 45
    GPLAY = 5
    NETFLIX = 37
    RED = 21
    BLUE = 29
    YELLOW = 12
    GREEN = 44
    GUIDE = 4
    DVR = 36
    INPUT = 11
    MIC = 28
    SETTINGS = 43
    UP = 20
    LEFT = 3
    OK = 27
    RIGHT = 35
    DOWN = 19
    BACK = 10
    TV = 26
    HOME = 42
    VOL_UP = 2
    VOL_DOWN = 9
    JUMP = 18
    MUTE = 25
    CH_UP = 34
    CH_DOWN = 41
    PLAY = 24
    RW = 8
    REC = 0
    PAUSE = 16
    FF = 40
    DISPLAY = 32
    AUDIO = 1
    CC = 17
    HELP = 33

class Ui(ABC):
    def __init__(self, handlers: EventHandlers):
        self.handlers = handlers
        self._subscribers: Dict[EventType, List[Callable]] = {}
        self._lock = threading.RLock()
        # Create a dedicated asyncio event loop that runs on its own thread
        self._loop = asyncio.new_event_loop()
        self._loop_thread = threading.Thread(target=self._run_loop, daemon=True)
        self._loop_thread.start()
        atexit.register(self.close)

    @abstractmethod
    def run(self):
        pass

    def publish(self, event_type: EventType, data: Any = None) -> None:
        logger.debug1("UI.publish called with event_type=%s, data=%s", event_type, data)
        asyncio.run_coroutine_threadsafe(self._publish_asynch(event_type, data), self._loop)

    def _subscribe(self, event_type: EventType, callback: Callable[[Any], None]) -> None:
        with self._lock:
            if event_type not in self._subscribers:
                self._subscribers[event_type] = []
            self._subscribers[event_type].append(callback)

    def _unsubscribe(self, event_type: EventType, callback: Callable[[Any], None]) -> None:
        with self._lock:
            if event_type in self._subscribers and callback in self._subscribers[event_type]:
                self._subscribers[event_type].remove(callback)

    async def _publish_asynch(self, event_type: EventType, data: Any = None) -> None:
        # Snapshot callbacks under lock to avoid races with subscribe/unsubscribe
        with self._lock:
            callbacks = list(self._subscribers.get(event_type, []))
        logger.debug1("_publish_asynch: %s, %d subscribers", event_type, len(callbacks))
        if callbacks:
            tasks = []
            for cb in callbacks:
                logger.debug1("Scheduling callback for %s", event_type)
                if asyncio.iscoroutinefunction(cb):
                    tasks.append(cb(data))
                else:
                    tasks.append(self._loop.run_in_executor(None, cb, data))
            results = await asyncio.gather(*tasks, return_exceptions=True)
            logger.debug1("Callback results for %s: %s", event_type, results)
        else:
            logger.debug1("No subscribers for %s", event_type)

    def _run_loop(self):
        asyncio.set_event_loop(self._loop)
        self._loop.run_forever()

    def close(self):
        if getattr(self, '_loop', None) and getattr(self, '_loop_thread', None):
            try:
                self._loop.call_soon_threadsafe(self._loop.stop)
                self._loop_thread.join(timeout=1.0)
            except Exception:
                pass
