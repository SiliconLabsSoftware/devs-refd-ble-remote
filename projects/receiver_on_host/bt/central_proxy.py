from bt.central import BtCentral
from typing import Optional, List, Callable
from log import get_logger

logger = get_logger(__name__)

class BtCentralProxy(BtCentral):
    """Proxy class for BtCentral interface. Delegates method calls to underlying instance."""

    def __init__(self, central: BtCentral):
        self._central = central

    def scan(self, timeout: float = 10.0) -> Optional[List[BtCentral.ScanResult]]:
        return self._central.scan(timeout)

    def connect(self, device_address: str, disconnected_callback: Callable[[], None]) -> bool:
        return self._central.connect(device_address, disconnected_callback=disconnected_callback)

    def disconnect(self) -> None:
        self._central.disconnect()

    def read(self, characteristic_uuid: str) -> Optional[bytes]:
        return self._central.read(characteristic_uuid)

    def write(self, characteristic_uuid: str, data: bytes) -> None:
        self._central.write(characteristic_uuid, data)

    def notify(self, characteristic_uuid: str, handler: Callable[[str, bytes], None]) -> None:
        self._central.notify(characteristic_uuid, handler)

    def filter(self, devices: Optional[List[BtCentral.ScanResult]], name: str) -> Optional[List[BtCentral.ScanResult]]:
        result = self._central.filter(devices, name) if devices else None
        logger.debug("filter() returning %s devices: %s",
                    len(result) if result else 0,
                    [d.address for d in result] if result else [])
        return result

    def is_connected(self) -> bool:
        return self._central.is_connected()
