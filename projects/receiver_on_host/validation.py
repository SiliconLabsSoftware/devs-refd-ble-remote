"""Input validation utilities for application methods. """

import re
from typing import Optional


# Regex patterns for validation
UUID_PATTERN = r'^[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}$'
MAC_PATTERN = r'^([0-9A-F]{2}[:-]?){5}([0-9A-F]{2})$'


class ValidationError(ValueError):
    """Raised when input validation fails."""
    pass


def validate_scan_timeout(timeout: float, min_value: float = 0.1, max_value: float = 300.0) -> float:
    """Validate BLE scan timeout value.

    Args:
        timeout: Scan timeout in seconds
        min_value: Minimum allowed timeout (default: 0.1s)
        max_value: Maximum allowed timeout (default: 300s / 5 minutes)

    Returns:
        The validated timeout value

    Raises:
        ValidationError: If timeout is invalid
    """
    if timeout is None:
        raise ValidationError("Scan timeout cannot be None")

    if not isinstance(timeout, (int, float)):
        raise ValidationError(f"Scan timeout must be a number, got {type(timeout).__name__}")

    if timeout <= 0:
        raise ValidationError(f"Scan timeout must be positive, got {timeout}")

    if timeout < min_value:
        raise ValidationError(f"Scan timeout must be at least {min_value}s, got {timeout}s")

    if timeout > max_value:
        raise ValidationError(f"Scan timeout exceeds maximum of {max_value}s, got {timeout}s")

    return float(timeout)


def validate_device_address(address: str) -> str:
    """Validate BLE device address format."""
    if address is None:
        raise ValidationError("Device address cannot be None")

    if not isinstance(address, str):
        raise ValidationError(f"Device address must be a string, got {type(address).__name__}")

    address = address.strip()

    if not address:
        raise ValidationError("Device address cannot be empty")

    # Check if it's a UUID format (macOS uses this for BLE devices)
    # Pattern: 8-4-4-4-12 hex digits separated by dashes
    if re.match(UUID_PATTERN, address):
        # Return UUID in uppercase
        return address.upper()

    # Try MAC address format
    normalized = address.replace('-', ':').replace(' ', '').upper()

    # Check if it's a valid MAC address format
    # Pattern: 6 groups of 2 hex digits, optionally separated by colons
    if not re.match(MAC_PATTERN, normalized):
        raise ValidationError(
            f"Invalid device address format: '{address}'. "
            f"Expected MAC address format (e.g., 'AA:BB:CC:DD:EE:FF') or UUID format (e.g., 'XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX')"
        )

    if ':' not in normalized:
        normalized = ':'.join([normalized[i:i+2] for i in range(0, len(normalized), 2)])

    return normalized
