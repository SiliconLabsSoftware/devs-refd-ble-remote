import tkinter as tk
from tkinter import ttk
import queue
from log import get_logger

logger = get_logger(__name__)

class AudioPane(ttk.Frame):
    """Pane for audio controls and status display. Handles recording status, frames statistics, playback, and AI analysis."""

    def __init__(self, parent, handlers=None):
        super().__init__(parent, padding=10)
        self.handlers = handlers
        self.audio_received = False
        self.is_playing = False

        self.ui_update_queue = queue.Queue()
        self._start_queue_processor()

        self._build_ui()

    def _build_ui(self):
        stats_frame = ttk.LabelFrame(self, text="Audio Statistics")
        stats_frame.pack(fill=tk.X, expand=False, pady=(0, 5))

        self.audio_status_var = tk.StringVar(value="Audio: --")
        self.audio_frames_received_var = tk.StringVar(value="Audio frames received: --")
        self.audio_frames_lost_var = tk.StringVar(value="Audio frames lost: --")

        ttk.Label(stats_frame, textvariable=self.audio_status_var).pack(anchor="w", padx=8, pady=2)
        ttk.Label(stats_frame, textvariable=self.audio_frames_received_var).pack(anchor="w", padx=8, pady=2)
        ttk.Label(stats_frame, textvariable=self.audio_frames_lost_var).pack(anchor="w", padx=8, pady=2)

        controls_frame = ttk.LabelFrame(self, text="Audio Controls")
        controls_frame.pack(fill=tk.X, expand=False, pady=(5, 0))

        self.audio_recording_status_var = tk.StringVar(value="waiting for audio transmission")
        status_label = ttk.Label(controls_frame, textvariable=self.audio_recording_status_var)
        status_label.pack(anchor="w", padx=8, pady=4)

        button_frame = ttk.Frame(controls_frame)
        button_frame.pack(fill=tk.X, padx=8, pady=4)

        self.play_button = ttk.Button(button_frame, text="Play", command=self._on_play_audio)
        self.play_button.pack(side=tk.LEFT, padx=(0, 5))
        self.play_button.config(state=tk.DISABLED)

        self.ai_button = ttk.Button(button_frame, text="Ask AI", command=self._on_ask_ai)
        self.ai_button.pack(side=tk.LEFT, padx=(5, 0))
        self.ai_button.config(state=tk.DISABLED)

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

    def update_audio_status(self, status):
        if isinstance(status, int):
            self.audio_status_var.set(f"Audio: {'Recording...' if status else 'Stopped'}")
        else:
            self.audio_status_var.set(f"Audio: {status}")

    def update_audio_frames_received(self, received: int):
        self.audio_frames_received_var.set(f"Audio frames received: {received}")

        if received > 0 and not self.audio_received:
            self.audio_received = True
            self.play_button.config(state=tk.NORMAL)

    def update_audio_frames_lost(self, lost: int):
        self.audio_frames_lost_var.set(f"Audio frames lost: {lost}")

    def update_audio_recording_status(self, status: str):
        self.audio_recording_status_var.set(status)

    def reset_audio_state(self):
        self.audio_received = False
        self.play_button.config(state=tk.DISABLED)
        if self.is_playing:
            if self.handlers and hasattr(self.handlers, 'audio_stop'):
                self.handlers.audio_stop()
            self._set_audio_stopped()

    def _on_play_audio(self):
        if self.is_playing:
            if self.handlers and hasattr(self.handlers, 'audio_stop'):
                self.handlers.audio_stop()
            self._set_audio_stopped()
        else:
            if self.handlers and hasattr(self.handlers, 'audio_play'):
                self.handlers.audio_play()
            self._set_audio_playing()

    def _set_audio_playing(self):
        self.is_playing = True
        self.play_button.config(text="Stop")

    def _set_audio_stopped(self):
        self.is_playing = False
        self.play_button.config(text="Play")

    def set_audio_playing_state(self):
        self.ui_update_queue.put(self._set_audio_playing)

    def set_audio_stopped_state(self):
        self.ui_update_queue.put(self._set_audio_stopped)

    def _on_ask_ai(self):
        if self.handlers and hasattr(self.handlers, 'ai_analyze'):
            self.ai_button.config(text="Thinking...", state=tk.DISABLED)
            self.handlers.ai_analyze()

    def _on_ask_ai_error(self):
        self.ai_button.config(text="Ask AI")

        self.ai_button.config(state=tk.DISABLED)

    def set_ai_button_to_ready(self):
        self.ai_button.config(text="Ask AI", state=tk.NORMAL)

    def set_ai_button_to_error_state(self):
        self.ui_update_queue.put(self._on_ask_ai_error)

    def update_ai_button_state(self, has_audio_recording: bool):
        if has_audio_recording:
            self.ai_button.config(state=tk.NORMAL)
        else:
            self.ai_button.config(state=tk.DISABLED)

    def show_ai_response_popup(self, ai_response: str):
        popup = tk.Toplevel(self)
        popup.title("AI Response")
        popup.geometry("600x600")  # Set the size of the popup window

        try:
            from ui.tkinter.ui_style import apply_silabs_light_theme, SILABS_LIGHT
            apply_silabs_light_theme(popup)
        except ImportError:
            pass

        main_frame = ttk.Frame(popup)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        response_frame = ttk.LabelFrame(main_frame, text="AI Response")
        response_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 10))

        text_frame = ttk.Frame(response_frame)
        text_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        text_scrollbar = ttk.Scrollbar(text_frame, orient=tk.VERTICAL)
        text_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        text_field = tk.Text(
            text_frame,
            wrap=tk.WORD,
            yscrollcommand=text_scrollbar.set,
            font=("Segoe UI", 10),
            padx=8,
            pady=8,
            borderwidth=1,
            relief="solid"
        )
        text_field.pack(fill=tk.BOTH, expand=True)
        text_scrollbar.config(command=text_field.yview)

        text_field.insert(tk.END, ai_response)
        text_field.config(state=tk.DISABLED)

        close_button = ttk.Button(main_frame, text="Close", command=popup.destroy)
        close_button.pack(pady=(0, 5))

        popup.transient(self.winfo_toplevel())
        popup.grab_set()

        popup.update_idletasks()
        x = (popup.winfo_reqwidth() // 2) + self.winfo_toplevel().winfo_rootx()
        y = (popup.winfo_reqheight() // 2) + self.winfo_toplevel().winfo_rooty()
        popup.geometry(f"+{x}+{y}")
        popup.focus_set()
