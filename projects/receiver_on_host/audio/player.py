import os
import threading
from typing import Optional
from playsound3 import playsound
from log import get_logger

logger = get_logger(__name__)

class AudioPlayer:
    """Cross-platform audio player for WAV files. Uses playsound3 library."""

    def __init__(self):
        self._lock = threading.RLock()  # Thread-safe access to playback state
        self.current_sound = None
        self.is_playing = False
        logger.debug("Initialized AudioPlayer with playsound3")

    def play(self, file_path: str) -> bool:
        if not os.path.exists(file_path):
            logger.error("Audio file not found: %s", file_path)
            return False

        if os.path.getsize(file_path) <= 0:
            logger.warning("Audio file is empty: %s", file_path)
            return False

        with self._lock:
            try:
                self.current_sound = playsound(file_path, block=False)
                self.is_playing = True
                logger.info("Playing audio file: %s", file_path)
                return True
            except Exception as e:
                logger.error("Error playing audio file %s: %s", file_path, e)
                self.current_sound = None
                self.is_playing = False
                return False

    def stop(self) -> bool:
        with self._lock:
            if not self.is_playing or not self.current_sound:
                logger.info("No audio currently playing")
                return True

            try:
                self.current_sound.stop()
                self.current_sound = None
                self.is_playing = False
                logger.info("Audio playback stopped successfully")
                return True
            except Exception as e:
                logger.error("Error stopping audio playback: %s", e)
                self.current_sound = None
                self.is_playing = False
                return False

    def is_playing_active(self) -> bool:
        with self._lock:
            if not self.is_playing or not self.current_sound:
                return False

            try:
                if not self.current_sound.is_alive():
                    self.is_playing = False
                    self.current_sound = None
                    return False

                return True
            except Exception as e:
                logger.error("Error checking playback status: %s", e)
                self.is_playing = False
                self.current_sound = None
                return False
