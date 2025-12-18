import threading
import tkinter as tk
from tkinter import ttk
import queue
from ui.ui import EventHandlers
from log import get_logger

logger = get_logger(__name__)

class ControlBarPane(ttk.Frame):
    """Tkinter-based control bar for managing BLE device connections. Provides device selection dropdown and scan/connect buttons."""

    def __init__(self, parent, handlers: EventHandlers):
        super().__init__(parent, padding=10)
        self.handlers = handlers
        self.addresses = []

        self.ui_update_queue = queue.Queue()
        self._start_queue_processor()

        self.grid_columnconfigure(1, weight=1)
        self.selected_device_var = tk.StringVar(value="")

        self.scan_button = ttk.Button(self, text="Scan", command=self.start_scan_thread)
        self.scan_button.grid(row=0, column=0, padx=(0, 5))

        self.device_dropdown = ttk.Combobox(
            self,
            state="readonly",
            width=34,
            textvariable=self.selected_device_var,
            values=["(no devices)"]
        )
        self.device_dropdown.grid(row=0, column=1, sticky="ew", padx=5)
        self.device_dropdown.bind("<<ComboboxSelected>>", self.on_device_select)

        self.connect_button = ttk.Button(self, text="Connect", command=self.connect_to_device, state=tk.DISABLED)
        self.connect_button.grid(row=0, column=2, padx=(5, 0))
        self.update_dropdown()

    def _start_queue_processor(self):
        """Start the queue processor for macOS thread-safe UI updates."""
        def process_queue():
            while True:
                try:
                    callback = self.ui_update_queue.get_nowait()
                    callback()
                    self.ui_update_queue.task_done()
                except queue.Empty:
                    break
            self.after(50, process_queue)

        self.after(50, process_queue)

    def on_device_select(self, event=None):
        sel = self.selected_device_var.get()
        if sel and sel not in ("Scanning...", "(no devices)", "(no devices found)"):
            self.connect_button.config(state=tk.NORMAL)
        else:
            self.connect_button.config(state=tk.DISABLED)

    def start_scan_thread(self):
        self._set_scan_state(True)
        threading.Thread(target=self.run_scan, daemon=True).start()

    def run_scan(self):
        try:
            logger.debug1("Starting BLE scan...")
            addresses = list(self.handlers.scan())
            logger.debug1("Scan completed, found %d addresses: %s", len(addresses), addresses)
            self._scan_addresses = addresses

            logger.debug1("Using queue-based UI update method")
            self.ui_update_queue.put(self.update_ui_after_scan)
            logger.debug1("UI update scheduled")
        except Exception as e:
            logger.error("Scan failed: %s", e)
            import traceback
            traceback.print_exc()
            try:
                self.ui_update_queue.put(lambda: self._set_scan_state(False))
            except Exception as ui_error:
                logger.error("Failed to schedule UI reset: %s", ui_error)

    def update_ui_after_scan(self):
        logger.debug1("update_ui_after_scan called - starting UI update")
        try:
            self._set_scan_state(False)
            addresses = getattr(self, '_scan_addresses', [])
            logger.debug1(" Updating UI with %d addresses: %s", len(addresses), addresses)
            self.addresses = addresses

            if not addresses:
                logger.debug1(" No addresses found, setting 'no devices found'")
                self.device_dropdown['values'] = ["(no devices found)"]
                self.selected_device_var.set("(no devices found)")
                self.connect_button.config(state=tk.DISABLED)
            else:
                logger.debug1(" Addresses found, populating dropdown")
                self.device_dropdown['values'] = addresses
                # Auto-select first address if nothing valid is selected
                current_selection = self.selected_device_var.get()
                logger.debug1(" Current selection: %s", current_selection)
                if current_selection not in addresses:
                    logger.debug1(" Selecting first address: %s", addresses[0])
                    self.selected_device_var.set(addresses[0])
                self.connect_button.config(state=tk.NORMAL)
                logger.debug1(" Connect button enabled")
            logger.debug1(" UI update completed successfully")
        except Exception as e:
            logger.error(" Exception in update_ui_after_scan: %s", e)
            import traceback
            traceback.print_exc()
            try:
                self._set_scan_state(False)
            except Exception as reset_error:
                logger.error(" Failed to reset scan state: %s", reset_error)

    def update_dropdown(self):
        if not self.addresses:
            self.device_dropdown['values'] = ["(no devices)"]
            self.selected_device_var.set("(no devices)")
        else:
            self.device_dropdown['values'] = self.addresses
            self.selected_device_var.set(self.device_dropdown['values'][0])

    def _set_scan_state(self, scanning: bool):
        if scanning:
            self.scan_button.config(state=tk.DISABLED, text="Scanning...")
            self.device_dropdown['values'] = ["Scanning..."]
            self.selected_device_var.set("Scanning...")
            self.device_dropdown.config(state="disabled")
            self.connect_button.config(state=tk.DISABLED)
        else:
            self.scan_button.config(state=tk.NORMAL, text="Scan")
            self.device_dropdown.config(state="readonly")

    def set_connected_state(self, connected: bool):
        logger.debug1(" set_connected_state called with connected=%s", connected)
        if connected:
            self.connect_button.config(text="Disconnect", command=self.disconnect_from_device)
            self.scan_button.config(state=tk.DISABLED)
            logger.debug1(" UI set to connected state")
        else:
            self.connect_button.config(text="Connect", command=self.connect_to_device)
            self.scan_button.config(state=tk.NORMAL)
            logger.debug1(" UI set to disconnected state")

    def connect_to_device(self):
        selected_address = self.selected_device_var.get()
        if selected_address:
            self._set_busy_state(True, label="Connecting...")
            addr = selected_address  # capture in local var for thread
            threading.Thread(target=self._run_connect, args=(addr,), daemon=True).start()

    def _run_connect(self, address: str):
        logger.debug1(" Starting connection to %s...", address)
        try:
            self.handlers.connect(address)
            logger.debug1(" Connection successful")
            logger.debug1(" Using queue-based connect UI update")
            self.ui_update_queue.put(lambda: self._after_connect(True))
        except Exception as e:
            logger.debug1(" Connection failed: %s", e)
            logger.debug1(" Using queue-based connect UI update (failed)")
            self.ui_update_queue.put(lambda: self._after_connect(False))

    def _after_connect(self, success: bool):
        logger.debug1(" _after_connect called with success=%s", success)
        self.set_connected_state(success)
        self._set_busy_state(False)
        logger.debug1("UI state updated after connection")

    def disconnect_from_device(self):
        logger.debug1(" User-initiated disconnect")
        self._set_busy_state(True, label="Disconnecting...")
        threading.Thread(target=self._run_disconnect, daemon=True).start()

    def handle_device_disconnected(self):
        try:
            logger.debug1(" Device disconnected unexpectedly - updating UI state only")
            self.set_connected_state(False)
            self._set_busy_state(False)
        except Exception as e:
            logger.error(" Failed to handle device disconnection: %s", e)
            import traceback
            traceback.print_exc()

    def _run_disconnect(self):
        try:
            self.handlers.disconnect()
            logger.debug1(" Disconnection completed")

            self.ui_update_queue.put(lambda: self._after_disconnect())
        except Exception as e:
            logger.error(" Disconnect failed: %s", e)
            import traceback
            traceback.print_exc()
            try:
                self.ui_update_queue.put(lambda: self._after_disconnect())
            except Exception as ui_error:
                logger.error(" Failed to schedule UI update after disconnect error: %s", ui_error)

    def _after_disconnect(self):
        self.set_connected_state(False)
        self._set_busy_state(False)

    def _set_busy_state(self, busy: bool, label: str | None = None):
        logger.debug1(" _set_busy_state called with busy=%s, label='%s'", busy, label)
        if busy:
            if label:
                self.connect_button.config(text=label)
            self.connect_button.config(state=tk.DISABLED)
            self.scan_button.config(state=tk.DISABLED)
            self.device_dropdown.state(["disabled"])
            logger.debug1(" UI set to busy state")
        else:
            self.device_dropdown.state(["!disabled"])
            self.on_device_select()
            logger.debug1(" UI busy state cleared")
