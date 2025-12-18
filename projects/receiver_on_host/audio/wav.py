import threading
import wave
import atexit
import os
from typing import Optional


class WavWriter:
    """Helper class for writing 16-bit PCM audio data to WAV files. Thread-safe."""

    def __init__(self, path: str, sample_rate: int, channels: int = 1) -> None:
        """Initialize WavWriter.
        Args:
            path: File path for WAV file
            sample_rate: Sample rate of audio data
            channels: Number of audio channels (default: 1)
        """
        self._path = path
        self._sr = int(sample_rate)
        self._channels = int(channels)
        self._wav: Optional[wave.Wave_write] = None
        self._lock = threading.RLock()
        atexit.register(self._close)

    def write(self, pcm: bytes) -> None:
        with self._lock:
            if self._wav is None:
                self._open()
            self._wav.writeframes(pcm)

    def _open(self) -> None:
        with self._lock:
            if self._wav is not None:
                return
            self._wav = wave.open(self._path, 'wb')
            self._wav.setsampwidth(2)
            self._wav.setnchannels(self._channels)
            self._wav.setframerate(self._sr)

    def close(self) -> None:
        self._close()

    def _close(self) -> None:
        with self._lock:
            if self._wav is not None:
                self._wav.close()
                self._wav = None
