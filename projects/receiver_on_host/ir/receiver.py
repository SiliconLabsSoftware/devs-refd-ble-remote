from abc import ABC, abstractmethod
from typing import Callable

class IrReceiver(ABC):
    @property
    @abstractmethod
    def is_running(self) -> bool:
        pass

    @abstractmethod
    def start(self, handler: Callable[[bool, int], None]) -> None:
        pass

    @abstractmethod
    def stop(self) -> None:
        pass

class IrReceiverProxy(IrReceiver):
    def __init__(self, impl: IrReceiver):
        self._impl = impl

    @property
    def is_running(self) -> bool:
        return self._impl.is_running

    def start(self, handler: Callable[[bool, int], None]) -> None:
        self._impl.start(handler)

    def stop(self) -> None:
        self._impl.stop()
