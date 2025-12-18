import tkinter as tk
from tkinter import ttk
from ui.ui import EventHandlers
from ir.receiver_flirc_irtool import IrReceiverFlircIrTool

class IRPane(ttk.Frame):
    """Tkinter-based pane for IR functionality management. Provides start/stop buttons."""

    def __init__(self, parent, handlers: EventHandlers, button_highlight_callback=None):
        super().__init__(parent, padding=10)
        self.handlers = handlers
        self.button_highlight_callback = button_highlight_callback

        ir_frame = ttk.LabelFrame(self, text="IR functionality")
        ir_frame.pack(fill=tk.BOTH, expand=True)

        button_frame = ttk.Frame(ir_frame)
        button_frame.pack(fill=tk.X, padx=8, pady=8)

        self.start_button = ttk.Button(
            button_frame,
            text="Start IR",
            command=self._on_start_ir
        )
        self.start_button.pack(side=tk.LEFT, padx=(0, 5))

        self.stop_button = ttk.Button(
            button_frame,
            text="Stop IR",
            command=self._on_stop_ir
        )
        self.stop_button.pack(side=tk.LEFT, padx=(5, 0))

        self.ir_receiver = None

        self._update_button_states()

    def _on_start_ir(self):
        if self.handlers.ir_start:
            success = self.handlers.ir_start()
            if success:
                self._schedule_ui_update(self._update_button_states)

    def _on_stop_ir(self):
        if self.handlers.ir_stop:
            success = self.handlers.ir_stop()
            if success:
                self._schedule_ui_update(self._update_button_states)

    def _schedule_ui_update(self, callback):
        self.after(0, callback)

    def _update_button_states(self):
        if not self.ir_receiver:
            self.start_button.configure(state="disabled")
            self.stop_button.configure(state="disabled")
        elif self.ir_receiver.is_running:
            self.start_button.configure(state="disabled")
            self.stop_button.configure(state="normal")
        else:
            self.start_button.configure(state="normal")
            self.stop_button.configure(state="disabled")

    def simulate_button_press(self, button_id: int):
        if self.ir_receiver and hasattr(self.ir_receiver, '_callback') and self.ir_receiver._callback:
            self.ir_receiver._callback(True, button_id)
            self.after(100, lambda: self.ir_receiver._callback(False, button_id))

    def get_ir_receiver(self):
        return self.ir_receiver

    def set_ir_handler(self, ir_receiver):
        self.ir_receiver = ir_receiver
        self._update_button_states()

    def update_ir_buttons_enabled(self, enabled: bool, is_running: bool):
        if not enabled:
            self.start_button.configure(state="disabled")
            self.stop_button.configure(state="disabled")
        elif is_running:
            self.start_button.configure(state="disabled")
            self.stop_button.configure(state="normal")
        else:
            self.start_button.configure(state="normal")
            self.stop_button.configure(state="disabled")
