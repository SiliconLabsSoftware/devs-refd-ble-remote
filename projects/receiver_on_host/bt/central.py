
from abc import ABC, abstractmethod
from typing import Optional, List, Callable

class BtCentral(ABC):
    """Abstract base class for Bluetooth Central operations. Defines interface for BLE device management."""
    class ScanResult:
        def __init__(self, address: str, name: str):
            self.address = address
            self.name = name

        def __repr__(self):
            return f"ScanResult(address={self.address}, name={self.name})"

    @abstractmethod
    def scan(self, timeout: float) -> Optional[List[ScanResult]]:
        pass

    def filter(self, devices: List[ScanResult], name: str) -> Optional[List[ScanResult]]:
        return [device for device in devices if (device.name == name)]

    @abstractmethod
    def connect(self, address: str, disconnected_callback: Callable[[], None]) -> bool:
        pass

    @abstractmethod
    def disconnect(self) -> None:
        pass

    @abstractmethod
    def read(self, characteristic_uuid: str) -> Optional[bytes]:
        pass

    @abstractmethod
    def write(self, characteristic_uuid: str, data: bytes) -> None:
        pass

    @abstractmethod
    def notify(self, characteristic_uuid: str, handler: Callable[[str, bytes], None]) -> None:
        pass
