import os
import sys
import threading
import time
from pathlib import Path
from typing import List, Optional
from dataclasses import dataclass, field
from audio.audio import Audio
from audio.player import AudioPlayer
from log import get_logger
from config import Config
from ui.ui import EventType
from ui.event_bus import EventBus

logger = get_logger(__name__)


@dataclass
class AudioState:
    """Thread-safe shared state for audio operations. Auto-publishes events on state changes."""
    _lock: threading.Lock = field(default_factory=threading.Lock)
    _recording_active: bool = False
    _has_received_audio: bool = False
    _event_bus: Optional[EventBus] = None

    def set_event_bus(self, event_bus: Optional[EventBus]):
        with self._lock:
            self._event_bus = event_bus

    @property
    def recording_active(self) -> bool:
        with self._lock:
            return self._recording_active

    @recording_active.setter
    def recording_active(self, value: bool):
        from log import get_logger
        logger = get_logger(__name__)
        event_bus = None
        with self._lock:
            if self._recording_active != value:
                old_value = self._recording_active
                self._recording_active = value
                logger.debug("AudioState.recording_active changed: %s -> %s", old_value, value)
                event_bus = self._event_bus
        if event_bus:
            status = "audio transmission ongoing" if value else "waiting for audio transmission"
            logger.debug("Publishing AUDIO_RECORDING_STATUS event: '%s'", status)
            event_bus.publish(EventType.AUDIO_RECORDING_STATUS, status)

    @property
    def has_received_audio(self) -> bool:
        with self._lock:
            return self._has_received_audio

    @has_received_audio.setter
    def has_received_audio(self, value: bool):
        from log import get_logger
        logger = get_logger(__name__)
        event_bus = None
        with self._lock:
            if self._has_received_audio != value:
                old_value = self._has_received_audio
                self._has_received_audio = value
                logger.debug("AudioState.has_received_audio changed: %s -> %s", old_value, value)
                event_bus = self._event_bus
        if event_bus and value:
            logger.debug("Audio received - publishing AUDIO_RECORDING_STATUS event: 'incoming audio received'")
            event_bus.publish(EventType.AUDIO_RECORDING_STATUS, "incoming audio received")
            logger.debug("Enabling AI button via event bus")
            event_bus.publish(EventType.AI_BUTTON_STATE_CHANGED, {'enabled': True})

    def reset(self):
        from log import get_logger
        logger = get_logger(__name__)
        logger.debug("AudioState.reset() called - resetting all audio state")
        with self._lock:
            self._recording_active = False
            self._has_received_audio = False
            event_bus = self._event_bus
        logger.debug("AudioState reset complete - recording_active=False, has_received_audio=False")
        if event_bus:
            logger.debug("Publishing AUDIO_RECORDING_STATUS event: 'waiting for audio transmission'")
            event_bus.publish(EventType.AUDIO_RECORDING_STATUS, "waiting for audio transmission")


class AudioApp:
    """Manages audio playback and monitoring. Handles cross-platform audio player integration."""

    @classmethod
    def create(cls, audio_file_path: str, event_bus: Optional[EventBus] = None) -> 'AudioApp':
        """ Factory method to create AudioApp with all its dependencies. """
        logger.debug("AudioApp.create() called - creating dependencies...")
        audio = Audio(Audio.Config(), wav_path=audio_file_path)
        audio_player = AudioPlayer()
        audio_state = AudioState()
        if event_bus:
            audio_state.set_event_bus(event_bus)
        logger.debug("AudioApp dependencies created successfully")
        return cls(
            audio=audio,
            audio_player=audio_player,
            audio_file_path=audio_file_path,
            state=audio_state,
            event_bus=event_bus
        )

    def __init__(self,
                 audio: Audio,
                 audio_player: AudioPlayer,
                 audio_file_path: str,
                 state: AudioState,
                 event_bus: Optional[EventBus] = None):
        """ Initialize AudioApp with explicit dependencies.

        Args:
            audio: Audio processing object
            audio_player: Audio player for playback
            audio_file_path: Path to audio file
            state: Shared AudioState object for thread-safe state management
            event_bus: Optional event bus for publishing events
        """
        self.audio = audio
        self.audio_player = audio_player
        self.audio_file_path = audio_file_path
        self.state = state
        self.event_bus = event_bus
        self._lock = threading.RLock()
        self._audio_monitor_thread = None
        self._audio_monitoring = False

    def _audio_play(self):
        try:
            logger.debug("AudioApp._audio_play() called")
            logger.info("Attempting to play audio file: %s", self.audio_file_path)
            logger.debug("AudioState.has_received_audio: %s", self.state.has_received_audio)
            logger.debug("Audio file exists: %s", os.path.exists(self.audio_file_path))

            if not self.state.has_received_audio and not os.path.exists(self.audio_file_path):
                logger.warning("No audio file available to play")
                logger.debug("Publishing audio stopped state event")
                if self.event_bus:
                    self.event_bus.publish(EventType.AUDIO_STOPPED_STATE)
                return

            logger.debug("Calling audio_player.play()...")
            success = self.audio_player.play(self.audio_file_path)
            logger.debug("audio_player.play() returned: %s", success)

            if success:
                logger.info("Audio playback initiated successfully")
                logger.debug("Publishing audio playing state event and starting monitoring")
                if self.event_bus:
                    self.event_bus.publish(EventType.AUDIO_PLAYING_STATE)
                self._start_audio_monitoring()
            else:
                logger.error("Failed to initiate audio playback")
                logger.debug("Publishing audio stopped state event")
                if self.event_bus:
                    self.event_bus.publish(EventType.AUDIO_STOPPED_STATE)
        except Exception as e:
            logger.error("Failed to play audio: %s", e)
            logger.debug("Exception in _audio_play - publishing audio stopped state event")
            if self.event_bus:
                self.event_bus.publish(EventType.AUDIO_STOPPED_STATE)
            import traceback
            traceback.print_exc()

    def _audio_stop(self):
        try:
            logger.info("Attempting to stop audio playback")
            success = self.audio_player.stop()
            if success:
                logger.info("Audio playback stopped successfully")
            else:
                logger.error("Failed to stop audio playback")
            self._stop_audio_monitoring()
            if self.event_bus:
                self.event_bus.publish(EventType.AUDIO_STOPPED_STATE)
        except Exception as e:
            logger.error("Failed to stop audio: %s", e)
            if self.event_bus:
                self.event_bus.publish(EventType.AUDIO_STOPPED_STATE)
            import traceback
            traceback.print_exc()

    def _start_audio_monitoring(self):
        with self._lock:
            if self._audio_monitoring:
                logger.debug("Audio monitoring already active - skipping start")
                return

            logger.debug("Starting audio playback monitoring thread...")
            self._audio_monitoring = True
            self._audio_monitor_thread = threading.Thread(target=self._monitor_audio_playback, daemon=True)
            self._audio_monitor_thread.start()
            logger.debug("Audio monitoring thread started")

    def _stop_audio_monitoring(self):
        with self._lock:
            self._audio_monitoring = False

    def _monitor_audio_playback(self):
        logger.debug("Audio monitoring thread started - entering monitoring loop")
        while True:
            with self._lock:
                if not self._audio_monitoring:
                    logger.debug("Audio monitoring stopped - exiting loop")
                    break

            try:
                is_playing = self.audio_player.is_playing_active()
                logger.debug("Audio playback check: is_playing=%s", is_playing)

                if not is_playing:
                    logger.debug("Audio playback finished - stopping monitoring and publishing event")
                    with self._lock:
                        self._audio_monitoring = False
                    if self.event_bus:
                        self.event_bus.publish(EventType.AUDIO_STOPPED_STATE)
                    break
                time.sleep(0.5)
            except Exception as e:
                logger.error("Error monitoring audio playback: %s", e)
                logger.debug("Stopping audio monitoring due to error")
                with self._lock:
                    self._audio_monitoring = False
                break
        logger.debug("Audio monitoring thread exiting")
