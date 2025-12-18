class AppError(Exception):
    """Base exception for all application errors."""
    pass


class BleError(AppError):
    """Base exception for BLE-related errors."""
    pass


class BleScanError(BleError):
    """Raised when BLE scanning fails."""
    pass


class BleConnectionError(BleError):
    """Raised when BLE connection fails."""
    pass


class BleDisconnectionError(BleError):
    """Raised when BLE disconnection fails."""
    pass


class BleConfigurationError(BleError):
    """Raised when BLE device configuration fails."""
    pass


class AudioError(AppError):
    """Base exception for audio-related errors."""
    pass


class AudioPlaybackError(AudioError):
    """Raised when audio playback fails."""
    pass


class AudioProcessingError(AudioError):
    """Raised when audio processing fails."""
    pass


class AudioFileError(AudioError):
    """Raised when audio file operations fail."""
    pass


class AiError(AppError):
    """Base exception for AI-related errors."""
    pass


class AiClientError(AiError):
    """Raised when AI client is unavailable or misconfigured."""
    pass


class AiAnalysisError(AiError):
    """Raised when AI analysis fails."""
    pass


class IrError(AppError):
    """Base exception for IR-related errors."""
    pass


class IrInitializationError(IrError):
    """Raised when IR receiver initialization fails."""
    pass


class IrOperationError(IrError):
    """Raised when IR receiver operations fail."""
    pass


class ConfigurationError(AppError):
    """Raised when configuration is invalid or missing."""
    pass
