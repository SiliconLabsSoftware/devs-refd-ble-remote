import asyncio
import threading
import atexit
from typing import Dict, List, Callable, Any, Optional
from log import get_logger
from ui.ui import EventType

logger = get_logger(__name__)


class EventBus:
    """Thread-safe event bus for publishing and subscribing to application events. """

    def __init__(self):
        self._subscribers: Dict[EventType, List[Callable]] = {}
        self._lock = threading.RLock()

        self._loop = asyncio.new_event_loop()
        self._loop_thread = threading.Thread(target=self._run_loop, daemon=True, name="EventBus")
        self._loop_thread.start()
        atexit.register(self.close)

        logger.debug("EventBus initialized")

    def publish(self, event_type: EventType, data: Any = None) -> None:
        logger.debug1("EventBus.publish called with event_type=%s, data=%s", event_type, data)
        asyncio.run_coroutine_threadsafe(self._publish_async(event_type, data), self._loop)

    def subscribe(self, event_type: EventType, callback: Callable[[Any], None]) -> None:
        if callback is None:
            raise ValueError("Callback cannot be None")

        with self._lock:
            if event_type not in self._subscribers:
                self._subscribers[event_type] = []
            if callback not in self._subscribers[event_type]:
                self._subscribers[event_type].append(callback)
                logger.debug("Subscribed callback to %s (total subscribers: %s)",
                           event_type, len(self._subscribers[event_type]))
            else:
                logger.debug("Callback already subscribed to %s", event_type)

    def unsubscribe(self, event_type: EventType, callback: Callable[[Any], None]) -> None:
        with self._lock:
            if event_type in self._subscribers and callback in self._subscribers[event_type]:
                self._subscribers[event_type].remove(callback)
                logger.debug("Unsubscribed callback from %s (remaining subscribers: %s)",
                           event_type, len(self._subscribers.get(event_type, [])))
                if not self._subscribers[event_type]:
                    del self._subscribers[event_type]

    async def _publish_async(self, event_type: EventType, data: Any = None) -> None:
        with self._lock:
            callbacks = list(self._subscribers.get(event_type, []))

        logger.debug1("EventBus._publish_async: %s, %d subscribers", event_type, len(callbacks))

        if callbacks:
            tasks = []
            for cb in callbacks:
                logger.debug1("Scheduling callback for %s", event_type)
                if asyncio.iscoroutinefunction(cb):
                    tasks.append(cb(data))
                else:
                    tasks.append(self._loop.run_in_executor(None, cb, data))

            results = await asyncio.gather(*tasks, return_exceptions=True)

            for i, result in enumerate(results):
                if isinstance(result, Exception):
                    logger.error("Exception in callback %s for %s: %s", i, event_type, result, exc_info=result)
                else:
                    logger.debug1("Callback %d for %s completed successfully", i, event_type)
        else:
            logger.debug1("No subscribers for %s", event_type)

    def _run_loop(self) -> None:
        asyncio.set_event_loop(self._loop)
        try:
            self._loop.run_forever()
        except Exception as e:
            logger.error("EventBus event loop error: %s", e, exc_info=True)
        finally:
            logger.debug("EventBus event loop stopped")

    def close(self) -> None:
        if getattr(self, '_loop', None) and getattr(self, '_loop_thread', None):
            try:
                logger.debug("Closing EventBus...")
                self._loop.call_soon_threadsafe(self._loop.stop)
                self._loop_thread.join(timeout=1.0)
                if self._loop_thread.is_alive():
                    logger.warning("EventBus thread did not stop within timeout")
                else:
                    logger.debug("EventBus closed successfully")
            except Exception as e:
                logger.error("Error closing EventBus: %s", e, exc_info=True)

    def get_subscriber_count(self, event_type: EventType) -> int:
        with self._lock:
            return len(self._subscribers.get(event_type, []))

    def has_subscribers(self, event_type: EventType) -> bool:
        return self.get_subscriber_count(event_type) > 0
