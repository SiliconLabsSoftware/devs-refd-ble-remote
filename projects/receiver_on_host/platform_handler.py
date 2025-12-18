from enum import Enum, auto
from dataclasses import dataclass
from typing import Optional, Tuple
import sys
import os
from pathlib import Path

# Note: We use sys.platform and os.uname() instead of the standard library
# platform module to avoid naming conflicts. This module is named platform_handler.py
# to prevent shadowing the standard library 'platform' module.

class PlatformType(Enum):
    """Platform type enumeration."""
    WINDOWS = auto()
    MACOS = auto()
    LINUX = auto()
    UNKNOWN = auto()


class MacArchitecture(Enum):
    """macOS architecture enumeration."""
    INTEL = auto()
    ARM = auto()
    UNKNOWN = auto()


@dataclass(frozen=True)
class PlatformConfig:
    """Platform-specific configuration values."""
    ir_tool_path: str
    default_scan_timeout: float
    audio_player_command: str
    scan_interval: float = 2.5
    min_scan_time: float = 1.0


class Platform:
    """Platform detection and configuration utilities. Caches detection results for performance."""

    # Platform constants for backward compatibility
    WINDOWS = "Windows"
    MACOS = "Darwin"
    LINUX = "Linux"
    UNKNOWN = "Unknown"

    _platform_type: Optional[PlatformType] = None
    _platform_name: Optional[str] = None
    _mac_architecture: Optional[MacArchitecture] = None
    _config: Optional[PlatformConfig] = None

    # Platform-specific configurations
    _CONFIGS = {
        PlatformType.WINDOWS: PlatformConfig(
            ir_tool_path=r"C:\Program Files (x86)\Flirc\irtools.exe",
            default_scan_timeout=6.0,
            audio_player_command="powershell",
            scan_interval=5.0,
            min_scan_time=2.0
        ),
        PlatformType.MACOS: PlatformConfig(
            ir_tool_path='/Applications/Flirc.app/Contents/Resources/irtools',
            default_scan_timeout=6.0,
            audio_player_command="afplay",
            scan_interval=2.5,
            min_scan_time=1.0
        ),
        PlatformType.LINUX: PlatformConfig(
            ir_tool_path='/usr/local/bin/irtools',
            default_scan_timeout=6.0,
            audio_player_command="paplay",
            scan_interval=5.0,
            min_scan_time=2.0
        ),
    }

    @classmethod
    def _detect(cls) -> Tuple[PlatformType, str]:
        """Detect platform (cached after first call).

        Returns:
            Tuple of (PlatformType enum, platform name string)
        """
        if cls._platform_type is None:
            sys_platform = sys.platform.lower()

            # Detect platform using sys.platform (avoids naming conflict with this module)
            if sys_platform.startswith('win'):
                cls._platform_type = PlatformType.WINDOWS
                cls._platform_name = cls.WINDOWS
            elif sys_platform == 'darwin':
                cls._platform_type = PlatformType.MACOS
                cls._platform_name = cls.MACOS
                cls._detect_mac_architecture()
            elif sys_platform.startswith('linux'):
                cls._platform_type = PlatformType.LINUX
                cls._platform_name = cls.LINUX
            else:
                cls._platform_type = PlatformType.UNKNOWN
                cls._platform_name = cls.UNKNOWN

        return cls._platform_type, cls._platform_name

    @classmethod
    def _detect_mac_architecture(cls) -> None:
        """Detect macOS architecture (Intel vs Apple Silicon). Uses os.uname().machine."""
        if cls._mac_architecture is None:
            try:
                machine = os.uname().machine.lower()
                if machine == 'x86_64':
                    cls._mac_architecture = MacArchitecture.INTEL
                elif machine == 'arm64':
                    cls._mac_architecture = MacArchitecture.ARM
                else:
                    cls._mac_architecture = MacArchitecture.UNKNOWN
            except (AttributeError, OSError):
                cls._mac_architecture = MacArchitecture.UNKNOWN

    @classmethod
    def get_type(cls) -> PlatformType:
        """Get platform type enum.

        Returns:
            PlatformType enum value
        """
        return cls._detect()[0]

    @classmethod
    def get_name(cls) -> str:
        """Get platform name string (Windows, Darwin, Linux, Unknown).

        Returns:
            Platform name string
        """
        return cls._detect()[1]

    @classmethod
    def get_config(cls) -> PlatformConfig:
        """Get platform-specific configuration.

        Returns:
            PlatformConfig dataclass with platform-specific values
        """
        if cls._config is None:
            platform_type, _ = cls._detect()
            cls._config = cls._CONFIGS.get(platform_type, PlatformConfig(
                ir_tool_path="",
                default_scan_timeout=4.0,
                audio_player_command=""
            ))
        return cls._config

    # Convenience methods for platform checks
    @classmethod
    def is_windows(cls) -> bool:
        """Check if running on Windows.

        Returns:
            True if running on Windows, False otherwise
        """
        return cls.get_type() == PlatformType.WINDOWS

    @classmethod
    def is_macos(cls) -> bool:
        """Check if running on macOS.

        Returns:
            True if running on macOS, False otherwise
        """
        return cls.get_type() == PlatformType.MACOS

    @classmethod
    def is_linux(cls) -> bool:
        """Check if running on Linux.

        Returns:
            True if running on Linux, False otherwise
        """
        return cls.get_type() == PlatformType.LINUX

    # macOS architecture detection methods
    @classmethod
    def get_mac_architecture(cls) -> MacArchitecture:
        """Get macOS architecture (Intel or Apple Silicon).

        Returns:
            MacArchitecture enum (INTEL, ARM, or UNKNOWN)
            Returns UNKNOWN if not running on macOS
        """
        if not cls.is_macos():
            return MacArchitecture.UNKNOWN
        cls._detect()  # Ensure detection has run
        if cls._mac_architecture is None:
            cls._detect_mac_architecture()
        return cls._mac_architecture or MacArchitecture.UNKNOWN

    @classmethod
    def is_macos_intel(cls) -> bool:
        """Check if running on Intel-based macOS.

        Returns:
            True if running on Intel-based macOS, False otherwise
        """
        return cls.is_macos() and cls.get_mac_architecture() == MacArchitecture.INTEL

    @classmethod
    def is_macos_arm(cls) -> bool:
        """Check if running on Apple Silicon macOS.

        Returns:
            True if running on Apple Silicon macOS, False otherwise
        """
        return cls.is_macos() and cls.get_mac_architecture() == MacArchitecture.ARM

    # Platform-specific getters
    @classmethod
    def get_ir_tool_path(cls) -> str:
        return cls.get_config().ir_tool_path

    @classmethod
    def get_scan_timeout(cls, config, default: Optional[float] = None) -> float:
        platform_config = cls.get_config()
        if default is None:
            default = platform_config.default_scan_timeout
        return config.get("scan_timeout", default)

    @classmethod
    def get_scan_interval(cls, timeout: float) -> float:
        config = cls.get_config()
        if cls.is_macos():
            interval = min(config.scan_interval, timeout / 3)
        else:
            interval = min(config.scan_interval, timeout / 2)
        return max(config.min_scan_time, interval)

    @classmethod
    def get_min_scan_time(cls) -> float:
        return cls.get_config().min_scan_time

    @staticmethod
    def get_executable_directory() -> Path:
        try:
            if hasattr(sys, '_MEIPASS'):
                return Path(sys.executable).parent
            else:
                script_path = Path(sys.argv[0]).resolve()

                if script_path.is_file():
                    return script_path.parent
                elif script_path.is_dir():
                    return script_path
                else:
                    try:
                        import __main__
                        if hasattr(__main__, '__file__'):
                            main_file = Path(__main__.__file__).resolve()
                            return main_file.parent
                    except Exception:
                        pass

                    return Path(__file__).parent
        except Exception:
            return Path.cwd()

    @staticmethod
    def is_pyinstaller_bundle() -> bool:
        return hasattr(sys, '_MEIPASS')

    @staticmethod
    def is_script_execution() -> bool:
        return not Platform.is_pyinstaller_bundle()

    # Python version detection methods
    @staticmethod
    def get_python_version() -> Tuple[int, int, int]:
        return sys.version_info[:3]

    @staticmethod
    def get_python_version_string() -> str:
        version_info = sys.version_info
        return f"{version_info.major}.{version_info.minor}.{version_info.micro}"

    @staticmethod
    def get_python_version_full() -> str:
        return sys.version.split('\n')[0]

    @staticmethod
    def is_python_version(major: int, minor: Optional[int] = None, micro: Optional[int] = None) -> bool:
        current = sys.version_info
        if current.major != major:
            return False
        if minor is not None:
            if current.minor != minor:
                return False
            if micro is not None:
                return current.micro == micro
        return True

    @staticmethod
    def is_python_version_at_least(major: int, minor: int = 0, micro: int = 0) -> bool:
        current = sys.version_info
        return (current.major, current.minor, current.micro) >= (major, minor, micro)

    @staticmethod
    def is_python_version_below(major: int, minor: int = 0, micro: int = 0) -> bool:
        current = sys.version_info
        return (current.major, current.minor, current.micro) < (major, minor, micro)

    @staticmethod
    def check_python_version_requirement(min_major: int, min_minor: int = 0, min_micro: int = 0) -> bool:
        if not Platform.is_python_version_at_least(min_major, min_minor, min_micro):
            current = Platform.get_python_version_string()
            required = f"{min_major}.{min_minor}.{min_micro}"
            raise RuntimeError(
                f"Python {required} or higher is required, but found {current}. "
                f"Please upgrade your Python installation."
            )
        return True
