import logging
import os
from datetime import datetime
from typing import Optional

DEBUG1 = 11  # For UI-specific debug
DEBUG2 = 12  # For platform-specific debug
logging.addLevelName(DEBUG1, "DEBUG1")
logging.addLevelName(DEBUG2, "DEBUG2")

def debug1(self, message, *args, **kwargs):
    if self.isEnabledFor(logging.DEBUG) or self.isEnabledFor(DEBUG1):
        self._log(DEBUG1, message, args, **kwargs)

def debug2(self, message, *args, **kwargs):
    if self.isEnabledFor(logging.DEBUG) or self.isEnabledFor(DEBUG2):
        self._log(DEBUG2, message, args, **kwargs)

logging.Logger.debug1 = debug1
logging.Logger.debug2 = debug2

# Module-level state (initialized lazily)
_logger_initialized = False
_config: Optional[object] = None
LOG_FILE_PATH: Optional[str] = None


def initialize_logging(config=None):
    """Initialize logging system. Must be called before get_logger(). """
    global _logger_initialized, _config, LOG_FILE_PATH

    if _logger_initialized:
        return

    # Import Config here to avoid circular dependency and module-level side effects
    from config import Config

    if config is None:
        _config = Config()
    else:
        _config = config

    log_file_name = f"receiver_on_host_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"
    LOG_FILE_PATH = os.path.join(os.path.dirname(__file__), log_file_name)

    _level_name = _config.get("logging_level", "INFO").upper()
    _level_map = {
        "DEBUG": logging.DEBUG,
        "DEBUG1": logging.DEBUG,
        "DEBUG2": logging.DEBUG,
        "INFO": logging.INFO,
        "WARNING": logging.WARNING,
        "ERROR": logging.ERROR,
        "CRITICAL": logging.CRITICAL
    }
    _level = _level_map.get(_level_name, logging.INFO)

    handlers = []
    if _level == logging.DEBUG:
        handlers.append(logging.FileHandler(LOG_FILE_PATH))

    handlers.append(logging.StreamHandler())

    logging.basicConfig(
        level=_level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=handlers
    )

    _logger_initialized = True


def get_logger(name: str):
    if not _logger_initialized:
        raise RuntimeError(
            "Logging not initialized. Call initialize_logging() before using get_logger(). "
            "This should be done in main.py or at application startup."
        )
    return logging.getLogger(name)
