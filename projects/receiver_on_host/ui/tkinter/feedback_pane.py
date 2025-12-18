import tkinter as tk
from tkinter import ttk
import queue
from log import get_logger

logger = get_logger(__name__)
from ui.ui import EventHandlers, EventType

class FeedbackPane(ttk.Frame):
    """Pane for displaying feedback. Shows battery status and key status."""

    def __init__(self, parent, handlers: EventHandlers, button_highlight_callback=None):
        super().__init__(parent, padding=10)
        self.handlers = handlers
        self.button_highlight_callback = button_highlight_callback

        self.ui_update_queue = queue.Queue()
        self._start_queue_processor()

        self._build_ui()

    def _build_ui(self):
        feedback_frame = ttk.LabelFrame(self, text="Feedback")
        feedback_frame.pack(fill=tk.X, expand=False)

        self.batt_var = tk.StringVar(value="Battery: --")
        self.key_status_var = tk.StringVar(value="Key: --")

        ttk.Label(feedback_frame, textvariable=self.batt_var).pack(anchor="w", padx=8, pady=2)
        ttk.Label(feedback_frame, textvariable=self.key_status_var).pack(anchor="w", padx=8, pady=2)

    def _start_queue_processor(self):
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

    def update_battery(self, volts: float | int):
        if isinstance(volts, (int, float)):
            self.batt_var.set(f"Battery: {volts:.2f} V" if isinstance(volts, float) else f"Battery: {volts}")
        else:
            self.batt_var.set("Battery: --")

    def update_key_status(self, status: int):
        self.key_status_var.set(f"Key: {self._map_key_status(status)}")

    def _map_key_status(self, status: int) -> str:
        return {0: "No Key Pressed",
                1: "New Key Pressed",
                2: "1 Button Pressed (repeat)",
                3: "Button Released",
                4: "2 Button Pressed",
                5: "Multiple Press (can be invalid...)"}.get(min(status, 5), "Unknown")
