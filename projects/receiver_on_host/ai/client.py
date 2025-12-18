from abc import ABC, abstractmethod
from typing import Callable
import threading

from log import get_logger

logger = get_logger(__name__)

class AiClient(ABC):
    """Abstract base class for AI clients."""
    def __init__(self, api_key: str = None, model: str = None):
        self._api_key = api_key
        self._model = model

    @abstractmethod
    def send_text(self, prompt: str) -> str:
        pass

    @abstractmethod
    def send_file(self, prompt: str, file_path: str) -> str:
        pass

class AiClientProxy:
    """Thread-safe proxy for AI client operations."""
    def __init__(self, impl: AiClient):
        self._impl = impl
        self._lock = threading.RLock()

    def send(self, prompt: str, file_path: str = None) -> str:
        if file_path:
            logger.info("Sending prompt with file: %s", file_path)
        else:
            logger.info("Sending prompt without file")
        logger.debug("Prompt: %s", prompt)
        return self._impl.send_text(prompt) if file_path is None else self._impl.send_file(prompt, file_path)

    def send_async(self, prompt: str, file_path: str = None, response_handler: Callable[[str], None] = None) -> None:
        logger.debug("Starting async AI request")
        threading.Thread(target=self._worker, args=(prompt, file_path, response_handler), daemon=True).start()

    def _worker(self, prompt: str, file_path: str, response_handler: Callable[[str], None]) -> None:
        """Worker thread for async AI requests."""
        with self._lock:
            response = self.send(prompt, file_path)
            if response_handler:
                logger.debug("Calling response handler")
                response_handler(response)
            else:
                logger.debug("AI response: %s", response)
