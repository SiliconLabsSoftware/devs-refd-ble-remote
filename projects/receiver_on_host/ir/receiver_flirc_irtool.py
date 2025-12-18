import os
import sys
import re
import time
import subprocess
import threading
import atexit
import select
from typing import Optional, Callable

from .receiver import IrReceiverProxy
from log import get_logger

logger = get_logger(__name__)

SCANCODE_REGEX = re.compile(r'PRONTO.*NEC.*- scancode: (0x[0-9A-Fa-f]+)')
REPEAT_REGEX = re.compile(r'PRONTO.*NEC.*Repeat')

IR_SETUP_ERROR_MSG = (
    "IR receiver setup failed.\n\n"
    "Please check:\n"
    "1. Flirc USB device is connected\n"
    "2. Flirc application is installed\n"
    "3. 'irtools' command is available in PATH"
)

class IrReceiverFlircIrTool:
    """IR receiver implementation using Flirc irtools. Cross-platform IR communication."""

    def __init__(self) -> None:
        self._stop_event = threading.Event()
        self._lock = threading.RLock()
        self._ir_monitor_thread = None
        self._callback = None
        self._last_key_code: Optional[int] = None
        self._last_repeat_time: Optional[float] = None
        self._process = None
        atexit.register(self.stop)
        self._is_running = False
        self._buttons_enabled = False
        self.status_callback: Optional[Callable[[str], None]] = None
        self._detection_complete = threading.Event()
        self._detection_error = None
        self._detection_timeout = 5.0



    def _detect_flirc_availability(self):
        """Test if irtools is accessible and Flirc USB device is connected."""
        try:
            logger.debug("Testing irtools command availability and USB connection")

            # Run 'irtools decode -l' to check if USB device is connected
            process = subprocess.Popen(
                ['irtools', 'decode', '-l'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            time.sleep(0.5)
            process.terminate()
            try:
                stdout, stderr = process.communicate(timeout=2.0)
            except subprocess.TimeoutExpired:
                process.kill()
                stdout, stderr = process.communicate()
            output = stdout + stderr

            if "Error" in output:
                logger.error(IR_SETUP_ERROR_MSG)
                raise RuntimeError(IR_SETUP_ERROR_MSG)

            logger.info("irtools detected and Flirc USB device is connected")
            return

        except FileNotFoundError:
            logger.error(IR_SETUP_ERROR_MSG)
            raise RuntimeError(IR_SETUP_ERROR_MSG)
        except subprocess.TimeoutExpired:
            logger.error(IR_SETUP_ERROR_MSG)
            raise RuntimeError(IR_SETUP_ERROR_MSG)
        except Exception as e:
            logger.error("%s\n\nError details: %s", IR_SETUP_ERROR_MSG, e)
            raise RuntimeError(IR_SETUP_ERROR_MSG)

    def _detect_flirc_availability_async(self):
        """Run hardware detection in background thread and wait with timeout."""
        self._detection_complete.clear()
        self._detection_error = None

        def detection_worker():
            """Background thread worker for hardware detection."""
            try:
                self._detect_flirc_availability()
                logger.debug("Hardware detection completed successfully")
            except Exception as e:
                logger.error("Hardware detection failed: %s", e)
                self._detection_error = e
            finally:
                self._detection_complete.set()

        detection_thread = threading.Thread(target=detection_worker, daemon=True)
        detection_thread.start()
        logger.debug("Hardware detection started in background thread")

        if self._detection_complete.wait(timeout=self._detection_timeout):
            if self._detection_error:
                raise self._detection_error
            logger.debug("Hardware detection completed, proceeding with IR start")
        else:
            logger.error("Hardware detection timed out after   %s", self._detection_timeout)
            raise RuntimeError(f"Hardware detection timed out after {self._detection_timeout}s")

    def _update_status(self, message: str):
        if self.status_callback:
            self.status_callback(message)
        logger.info("IR Status: %s", message)

    @property
    def is_running(self) -> bool:
        with self._lock:
            return self._is_running

    @property
    def buttons_enabled(self) -> bool:
        with self._lock:
            return self._buttons_enabled

    def start(self, callback) -> None:
        """Start IR receiver thread and monitor thread"""
        self._detect_flirc_availability_async()

        if self._ir_monitor_thread and self._ir_monitor_thread.is_alive():
            logger.info("Stopping previous monitor thread...")
            self.stop()
        try:
            logger.info("Starting irtools process")
            self._process = subprocess.Popen(
                ['irtools', 'decode', '-l'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            logger.info("irtools process started successfully")
        except Exception as e:
            logger.error("Failed to start IR tool process: %s", e)
            raise RuntimeError("Failed to start IR tool process") from e

        with self._lock:
            self._callback = callback
        self._stop_event.clear()

        try:
            self._ir_monitor_thread = threading.Thread(target=self._monitor_tool_output, daemon=True)
            self._ir_monitor_thread.start()
            with self._lock:
                self._is_running = True
                self._buttons_enabled = True
            logger.info("Monitor thread started successfully")
            self._update_status("IR receiver started and ready")
        except Exception as e:
            if self._process:
                self._process.terminate()
                self._process = None
            with self._lock:
                self._is_running = False
                self._buttons_enabled = False
            logger.error("Failed to start monitor thread: %s", e)
            raise RuntimeError("Failed to start monitor thread") from e

    def stop(self) -> None:
        """Stop IR receiver by stopping monitor thread and process."""
        if not self._ir_monitor_thread or not self._ir_monitor_thread.is_alive():
            logger.debug("No active monitor thread to stop")
        else:
            logger.info("Stopping monitor thread...")

        if self._stop_event:
            self._stop_event.set()

        self._stop_process()
        self._stop_thread()

        with self._lock:
            self._callback = None
            self._is_running = False
            self._buttons_enabled = False
            self._last_key_code = None
            self._last_repeat_time = None
        self._update_status("IR receiver stopped")

    def _stop_process(self) -> None:
        if not self._process:
            return
        try:
            logger.info("Terminating IR tool process...")
            self._process.terminate()
            try:
                self._process.wait(timeout=2.0)
                logger.info("IR tool process terminated gracefully")
            except subprocess.TimeoutExpired:
                logger.warning("Process did not terminate gracefully, killing...")
                self._process.kill()
                self._process.wait()
                logger.info("IR tool process killed")
        except Exception as e:
            logger.error("Error terminating IR tool process: %s", e)
        finally:
            self._process = None

    def _stop_thread(self) -> None:
        if not self._ir_monitor_thread or not self._ir_monitor_thread.is_alive():
            return
        try:
            logger.info("Waiting for monitor thread to finish...")
            self._ir_monitor_thread.join(timeout=5.0)
            if self._ir_monitor_thread.is_alive():
                logger.warning("IR monitoring thread did not stop gracefully within timeout")
            else:
                logger.info("Monitor thread stopped successfully")
        except Exception as e:
            logger.error("Error stopping monitor thread: %s", e)

    def _monitor_tool_output(self) -> None:
        """Monitor IR tool output in loop until stop event is set. Uses blocking readline for all platforms."""
        try:
            logger.info("Starting IR tool output monitoring...")
            while not self._stop_event.is_set():
                if self._process is None or self._process.stdout is None:
                    logger.warning("Process or stdout is None, stopping monitoring")
                    break
                line = self._process.stdout.readline()
                current_time = time.time()
                if line:
                    self._process_line(line, current_time)
                else:
                    time.sleep(0.05)
                self._check_timeout(current_time)
            logger.info("IR tool output monitoring stopped")
        except Exception as e:
            logger.error("Error monitoring tool output: %s", e)

    def _process_line(self, line: str, current_time: float) -> None:
        logger.debug("Processing line: %s", line.strip())
        scancode = self._get_scancode(line)
        if scancode:
            logger.debug("Found scancode: %s", scancode)
            key_code = self._scan_code_to_key(scancode)
            logger.debug("Converted to key_code: %s", key_code)
            self._key_change(True, key_code, current_time)
        else:
            with self._lock:
                last_key = self._last_key_code
            if self._is_repeat_code(line) and last_key is not None:
                logger.debug("Repeat code for key: %s", last_key)
                self._key_change(True, last_key, current_time)
            else:
                logger.debug("Line ignored (no scancode or repeat): %s", line.strip())

    def _check_timeout(self, current_time: float) -> None:
        with self._lock:
            last_repeat_time = self._last_repeat_time
            last_key = self._last_key_code
        if last_repeat_time and (current_time - last_repeat_time > 0.2):
            if last_key is not None:
                self._key_change(False, last_key, current_time)
            with self._lock:
                self._last_key_code = None

    def _key_change(self, pressed: bool, key_code: int, current_time: float) -> None:
        """Handle key state changes and notify callback.

        Args:
            pressed: True if key is pressed, False if released
            key_code: Key code that changed state
            current_time: Current timestamp for release events
        """
        with self._lock:
            callback = self._callback

        logger.debug("_key_change: pressed=%s, key_code=%s, callback=%s", pressed, key_code, callback is not None)
        if callback and key_code is not None and key_code > 0:
            logger.debug("Calling callback with pressed=%s, key_code=%s", pressed, key_code)
            callback(pressed, key_code)
            logger.debug("Callback completed")
        else:
            logger.debug("No callback registered or invalid key_code!")

        with self._lock:
            if pressed:
                self._last_key_code = key_code
                self._last_repeat_time = current_time
            else:
                self._last_key_code = None
                self._last_repeat_time = None

    def _get_scancode(self, line: str) -> Optional[str]:
        match = SCANCODE_REGEX.search(line)
        return match.group(1) if match else None

    def _is_repeat_code(self, line: str) -> bool:
        return REPEAT_REGEX.search(line) is not None

    def _scan_code_to_key(self, scancode: str) -> int:
        """Convert scancode to key code.

        Args:
            scancode: Hex string scancode from IR tool

        Returns:
            Key code as integer, or 0 if invalid
        """
        if scancode.startswith("0x"):
            try:
                value = int(scancode, 16)
                swapped = int.from_bytes(value.to_bytes(4, byteorder='big'), byteorder='little')
                return (swapped & 0xFF00) >> 8
            except (ValueError, OverflowError) as e:
                logger.warning("Invalid scancode %s: %s", scancode, e)
                return 0
        return 0
