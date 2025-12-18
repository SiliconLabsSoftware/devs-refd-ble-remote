import os
import sys
import threading
import time
from pathlib import Path
from typing import Optional
from ai.client import AiClient
from audio.audio_app import AudioState
from ui.ui import EventType
from ui.event_bus import EventBus
from log import get_logger
from config import Config

logger = get_logger(__name__)


class AiApp:
    """Manages AI-powered audio analysis."""

    @classmethod
    def create(cls,
               audio_app: 'AudioApp',
               config: Config,
               event_bus: Optional[EventBus] = None) -> 'AiApp':
        """Factory method to create AiApp with all its dependencies. """
        from ai.client import AiClientProxy
        from ai.client_gemini import GeminiClient

        logger.debug("AiApp.create() called - creating dependencies...")
        ai_client = None
        try:
            gemini_client = GeminiClient(config=config)
            ai_client = AiClientProxy(gemini_client)
            logger.info("AI client initialized successfully")
        except Exception as e:
            logger.warning("Failed to initialize AI client: %s", e)
            logger.debug("AI client will not be available")
        logger.debug("AiApp dependencies created successfully")
        return cls(
            ai_client=ai_client,
            config=config,
            audio_file_path=audio_app.audio_file_path,
            state=audio_app.state,
            event_bus=event_bus
        )

    def __init__(self,
                 ai_client: Optional[AiClient],
                 config: Config,
                 audio_file_path: str,
                 state: AudioState,
                 event_bus: Optional[EventBus] = None):
        """Initialize AiApp with explicit dependencies. """
        self.ai_client = ai_client
        self.config = config
        self.audio_file_path = audio_file_path
        self.state = state
        self.event_bus = event_bus

    def _ai_analyze(self):
        """Analyze the recorded audio file using AI."""
        logger.debug("AiApp._ai_analyze() called")

        if not self.ai_client:
            logger.warning("AI client not available")
            logger.debug("AI client is None - cannot analyze")
            if self.event_bus:
                logger.debug("Publishing AI_ERROR event: 'AI client not available'")
                self.event_bus.publish(EventType.AI_ERROR, {"error": "AI client not available"})
            return

        logger.debug("Checking AudioState.has_received_audio: %s", self.state.has_received_audio)
        if not self.state.has_received_audio:
            logger.warning("No audio recording available for AI analysis")
            logger.debug("AudioState.has_received_audio is False - cannot analyze")
            if self.event_bus:
                logger.debug("Publishing AI_ERROR event: 'No audio recording available'")
                self.event_bus.publish(EventType.AI_ERROR, {"error": "No audio recording available for AI analysis"})
            return

        logger.debug("Checking audio file existence: %s", self.audio_file_path)
        if not os.path.exists(self.audio_file_path):
            logger.warning("Audio file not found: %s", self.audio_file_path)
            logger.debug("Audio file does not exist - cannot analyze")
            return

        logger.info("Starting AI analysis of audio file: %s", self.audio_file_path)
        logger.debug("Audio file exists, size: %s bytes", os.path.getsize(self.audio_file_path))

        def analyze_audio():
            logger.debug("AI analysis thread started")
            try:
                logger.debug("Getting AI prompt from config...")
                prompt = self.config.get('ai_prompt')
                if not prompt:
                    error_msg = "AI prompt not found in configuration"
                    raise ValueError(error_msg)

                logger.debug("AI prompt retrieved (length: %s chars)", len(prompt))
                logger.debug("Calling ai_client.send() with prompt and audio file...")
                response = self.ai_client.send(prompt, self.audio_file_path)
                logger.info("AI Analysis Result: %s", response)
                logger.debug("AI analysis completed - response length: %s chars", len(response))

                if self.event_bus:
                    logger.debug("Publishing AI_ANALYSIS event with response")
                    self.event_bus.publish(EventType.AI_ANALYSIS, response)
                    logger.debug("AI_ANALYSIS event published successfully")
            except Exception as e:
                logger.error("AI analysis failed: %s", e)
                logger.debug("Exception in AI analysis: %s: %s", type(e).__name__, e)
                import traceback
                traceback.print_exc()

                if self.event_bus:
                    logger.debug("Publishing AI_ERROR event: %s", str(e))
                    self.event_bus.publish(EventType.AI_ERROR, {"error": str(e)})

        logger.debug("Spawning thread for AI analysis...")
        thread = threading.Thread(target=analyze_audio, daemon=True)
        thread.start()
        logger.debug("AI analysis thread started (thread ID: %s)", thread.ident)
