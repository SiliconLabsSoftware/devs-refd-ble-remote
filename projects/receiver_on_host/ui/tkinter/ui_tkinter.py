import os
import queue
import time
import threading

import tkinter as tk
from tkinter import ttk

from ui.ui import Ui, EventHandlers, EventType
from ui.event_bus import EventBus
from typing import Optional

from ui.tkinter.feedback_pane import FeedbackPane
from ui.tkinter.audio_pane import AudioPane
from ui.tkinter.ir_pane import IRPane
from ui.tkinter.remote_pane import RemotePane
from ui.tkinter.controlbar_pane import ControlBarPane
from ui.tkinter.ui_style import apply_silabs_light_theme, SILABS_LIGHT

from log import get_logger

logger = get_logger(__name__)
class UiTkinter(Ui, tk.Tk):
    """ Tkinter-based user interface for the BLE Remote Receiver application. """

    def __init__(self, handlers: EventHandlers, event_bus: Optional[EventBus] = None):

        Ui.__init__(self, handlers)
        tk.Tk.__init__(self)

        self.event_bus = event_bus

        self._set_icon()
        apply_silabs_light_theme(self)

        self.title("BLE Remote Receiver")
        screen_width = self.winfo_screenwidth()
        screen_height = self.winfo_screenheight()
        default_width = int(screen_width * 0.8)
        default_height = int(screen_height * 0.9)
        self.geometry(f"{default_width}x{default_height}")

        self.minsize(500, 700)

        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=2)
        self.grid_columnconfigure(1, weight=1)
        main_container = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        main_container.grid(row=0, column=0, columnspan=2, sticky="nsew")

        left_container = ttk.Frame(main_container)
        self.remote_pane = RemotePane(main_container, self.handlers)
        self.feedback_pane = FeedbackPane(left_container, self.handlers,
                                         button_highlight_callback=self.remote_pane.highlight_button)
        self.feedback_pane.pack(fill=tk.X, padx=10, pady=(10, 5))
        self.audio_pane = AudioPane(left_container, self.handlers)
        self.audio_pane.pack(fill=tk.X, padx=10, pady=5)
        self.ir_pane = IRPane(left_container, self.handlers,
                             button_highlight_callback=self.remote_pane.highlight_button)
        self.ir_pane.pack(fill=tk.X, padx=10, pady=(5, 10))

        main_container.add(left_container, weight=1)
        main_container.add(self.remote_pane, weight=2)

        self.control_bar = ControlBarPane(self, self.handlers)
        self.control_bar.grid(row=1, column=0, sticky="ew")

        self.ui_update_queue = queue.Queue()
        self._start_queue_processor()

        # Initialize button tracking state
        self._last_button = 0xFF  # 0xFF means no button is currently highlighted

        if self.event_bus:
            logger.debug("Subscribing to EventBus events...")
            self.event_bus.subscribe(EventType.CONNECTED, self._on_connected)
            self.event_bus.subscribe(EventType.DISCONNECTED, self._on_disconnected)
            self.event_bus.subscribe(EventType.BATTERY_VOLTAGE, self._on_battery_voltage)
            self.event_bus.subscribe(EventType.AUDIO_STATUS, self._on_audio_status)
            self.event_bus.subscribe(EventType.AUDIO_FRAMES_RECEIVED, self._on_audio_frames_received)
            self.event_bus.subscribe(EventType.AUDIO_FRAMES_LOST, self._on_audio_frames_lost)
            self.event_bus.subscribe(EventType.KEYS_ID, self._on_keys_id)
            self.event_bus.subscribe(EventType.KEYS_STATUS, self._on_keys_status)
            self.event_bus.subscribe(EventType.KEYS_BITMAP, self._on_keys_bitmap)
            self.event_bus.subscribe(EventType.AUDIO_RECORDING_STATUS, self._on_audio_recording_status)
            self.event_bus.subscribe(EventType.IR_BUTTONS_ENABLED, self._on_ir_buttons_enabled)
            self.event_bus.subscribe(EventType.IR_KEY_EVENT, self._on_ir_key_event)
            self.event_bus.subscribe(EventType.AI_ANALYSIS, self._on_ai_analysis)
            self.event_bus.subscribe(EventType.AI_ERROR, self._on_ai_error)
            self.event_bus.subscribe(EventType.AI_BUTTON_STATE_CHANGED, self._on_ai_button_state_changed)
            self.event_bus.subscribe(EventType.AUDIO_PLAYING_STATE, self._on_audio_playing_state)
            self.event_bus.subscribe(EventType.AUDIO_STOPPED_STATE, self._on_audio_stopped_state)
            self.event_bus.subscribe(EventType.IR_HANDLER_READY, self._on_ir_handler_ready)
            logger.debug("EventBus subscriptions complete")

        self._subscribe(EventType.BATTERY_VOLTAGE, lambda value: self._schedule_ui_update(lambda v=value: self.feedback_pane.update_battery(v)))
        self._subscribe(EventType.KEYS_STATUS, lambda status: self._schedule_ui_update(lambda s=status: self.feedback_pane.update_key_status(s)))
        self._subscribe(EventType.AUDIO_STATUS, lambda status: self._schedule_ui_update(lambda s=status: self.audio_pane.update_audio_status(s)))
        self._subscribe(EventType.AUDIO_FRAMES_RECEIVED, lambda received: self._schedule_ui_update(lambda r=received: self.audio_pane.update_audio_frames_received(r)))
        self._subscribe(EventType.AUDIO_FRAMES_LOST, lambda lost: self._schedule_ui_update(lambda l=lost: self.audio_pane.update_audio_frames_lost(l)))
        self._subscribe(EventType.AUDIO_RECORDING_STATUS, lambda status: self._schedule_ui_update(lambda s=status: self.audio_pane.update_audio_recording_status(s)))
        self._subscribe(EventType.IR_BUTTONS_ENABLED, lambda data: self._schedule_ui_update(lambda d=data: self.ir_pane.update_ir_buttons_enabled(d.get('enabled', False), d.get('is_running', False))))
        self._subscribe(EventType.IR_KEY_EVENT, lambda data: self._handle_ir_key_event(data))
        self._subscribe(EventType.AI_ANALYSIS, lambda results: self._schedule_ui_update(lambda r=results: self._handle_ai_analysis_complete(r)))
        self._subscribe(EventType.AI_ERROR, lambda error_data: self._schedule_ui_update(lambda ed=error_data: self._handle_ai_error(ed)))
        self._subscribe(EventType.DISCONNECTED, lambda addr: self._schedule_ui_update(lambda a=addr: (self.control_bar.handle_device_disconnected(), self.audio_pane.reset_audio_state())))

    def _set_icon(self):
        try:
            base_dir = os.path.dirname(os.path.abspath(__file__))
            icon_path = os.path.join(base_dir, "..", "silabsicon.png")  # Use a .png file
            icon_path = os.path.normpath(icon_path)

            from tkinter import PhotoImage
            self.iconphoto(False, PhotoImage(file=icon_path))  # Works on all platforms
        except Exception as e:
            logger.warning("Could not set icon: %s", e)
            logger.info("Using default icon.")

    def _start_queue_processor(self):
        def process_queue():
            processed = 0
            while True:
                try:
                    callback = self.ui_update_queue.get_nowait()
                    logger.debug1("process_queue: executing callback %s", callback)
                    callback()
                    self.ui_update_queue.task_done()
                    processed += 1
                except queue.Empty:
                    break
                except Exception as e:
                    logger.error("Exception in queue processor callback: %s", e)
                    import traceback
                    traceback.print_exc()
            if processed > 0:
                logger.debug1("process_queue: processed %d callbacks", processed)
            self.after(50, process_queue)

        self.after(50, process_queue)

    def _schedule_ui_update(self, callback):
        logger.debug1("_schedule_ui_update: queuing callback %s", callback)
        self.ui_update_queue.put(callback)

    def _handle_ir_key_event(self, data):
        if isinstance(data, dict) and 'button_id' in data:
            button_id = data['button_id']
            pressed = data.get('pressed', True)
            source = data.get('source', 'ir')

            logger.debug1("IR key event: button_id=%s, pressed=%s, source=%s", button_id, pressed, source)

            if pressed:
                self._schedule_ui_update(lambda bid=button_id: self._button_callback_single(bid))

                if source == 'ir':
                    def delayed_unhighlight():
                        time.sleep(0.5)
                        self._schedule_ui_update(lambda: self._unhighlight_all_buttons())
                    threading.Thread(target=delayed_unhighlight, daemon=True).start()

    def _unhighlight_all_buttons(self):
        if hasattr(self, '_last_button') and self._last_button != 0xFF:
            self.remote_pane.highlight_button(self._last_button, False)
            self._last_button = 0xFF

    def run(self):
        self.mainloop()

    def _button_callback_single(self, button_id: int):
        """ Handles a single button press event.

        This method highlights the button corresponding to the given button ID and unhighlights the previously
        highlighted button.

        Args:
            button_id (int): The ID of the button that was pressed.
        """
        logger.debug1("_button_callback_single called with button_id=%s", button_id)
        try:
            if hasattr(self, '_last_button') and self._last_button != 0xFF:
                logger.debug1("Un-highlighting previous button %s", self._last_button)
                self.remote_pane.highlight_button(self._last_button, False)
            logger.debug1("Highlighting button %s", button_id)
            self.remote_pane.highlight_button(button_id , True)
            self._last_button = button_id
            logger.debug1("_button_callback_single completed")
        except Exception as e:
            logger.error("Exception in _button_callback_single: %s", e)
            import traceback
            traceback.print_exc()

    def _button_callback_bitmap(self, button_bitmap: int):
        """ Handles a bitmap of button press events.

        This method highlights buttons corresponding to the bits set in the bitmap and unhighlights buttons
        corresponding to bits cleared in the bitmap.

        Args:
            button_bitmap (int): A bitmap representing the state of all buttons.

        Behavior:
            - Iterates through each bit in the bitmap to determine the state of each button.
            - Highlights buttons for bits set to 1 and unhighlights buttons for bits set to 0.
        """
        logger.debug1("_button_callback_bitmap called with bitmap=0x%016x", button_bitmap)
        try:
            if not hasattr(self, '_last_button_bitmap'):
                self._last_button_bitmap = 0
            for i in range(64):
                if button_bitmap & (1 << i):
                    logger.debug1("Highlighting button %d", i)
                    self.remote_pane.highlight_button(i, True)
                elif self._last_button_bitmap & (1 << i):
                    logger.debug1("Un-highlighting button %d", i)
                    self.remote_pane.highlight_button(i, False)
            self._last_button_bitmap = button_bitmap
            logger.debug1("_button_callback_bitmap completed")
        except Exception as e:
            logger.error("Exception in _button_callback_bitmap: %s", e)
            import traceback
            traceback.print_exc()

    def set_audio_playing_state(self):
        self._schedule_ui_update(lambda: self.audio_pane.set_audio_playing_state())

    def set_audio_stopped_state(self):
        self._schedule_ui_update(lambda: self.audio_pane.set_audio_stopped_state())

    def _handle_ai_analysis_complete(self, results):
        self.audio_pane.set_ai_button_to_ready()

        if results:
            if isinstance(results, dict):
                ai_response = results.get('response', str(results))
            else:
                ai_response = str(results)

            self.audio_pane.show_ai_response_popup(ai_response)

    def _handle_ai_error(self, error_data):
        self.audio_pane.set_ai_button_to_error_state()

    def _on_connected(self, device_address):
        """Handle CONNECTED event from EventBus."""
        logger.debug("_on_connected called with device_address: %s", device_address)
        self._schedule_ui_update(lambda: self.control_bar.set_connected_state(True))

    def _on_disconnected(self, device_address):
        """Handle DISCONNECTED event from EventBus."""
        logger.debug("_on_disconnected called with device_address: %s", device_address)
        self._schedule_ui_update(lambda addr=device_address: (
            self.control_bar.handle_device_disconnected(),
            self.audio_pane.reset_audio_state()
        ))

    def _on_battery_voltage(self, voltage):
        """Handle BATTERY_VOLTAGE event from EventBus."""
        logger.debug("_on_battery_voltage called with voltage: %s", voltage)
        self._schedule_ui_update(lambda v=voltage: self.feedback_pane.update_battery(v))

    def _on_audio_status(self, status):
        """Handle AUDIO_STATUS event from EventBus."""
        logger.debug("_on_audio_status called with status: %s", status)
        self._schedule_ui_update(lambda s=status: self.audio_pane.update_audio_status(s))

    def _on_audio_frames_received(self, count):
        """Handle AUDIO_FRAMES_RECEIVED event from EventBus."""
        logger.debug("_on_audio_frames_received called with count: %s", count)
        self._schedule_ui_update(lambda c=count: self.audio_pane.update_audio_frames_received(c))

    def _on_audio_frames_lost(self, count):
        """Handle AUDIO_FRAMES_LOST event from EventBus."""
        logger.debug("_on_audio_frames_lost called with count: %s", count)
        self._schedule_ui_update(lambda c=count: self.audio_pane.update_audio_frames_lost(c))

    def _on_keys_id(self, button_id):
        """Handle KEYS_ID event from EventBus."""
        logger.debug1("_on_keys_id called with button_id: %s", button_id)
        self._schedule_ui_update(lambda bid=button_id: self._button_callback_single(bid))

    def _on_keys_status(self, status):
        """Handle KEYS_STATUS event from EventBus."""
        logger.debug("_on_keys_status called with status: %s", status)
        self._schedule_ui_update(lambda s=status: self.feedback_pane.update_key_status(s))

    def _on_keys_bitmap(self, bitmap):
        """Handle KEYS_BITMAP event from EventBus."""
        logger.debug1("_on_keys_bitmap called with bitmap: 0x%016x", bitmap)
        self._schedule_ui_update(lambda bm=bitmap: self._button_callback_bitmap(bm))

    def _on_audio_recording_status(self, status):
        """Handle AUDIO_RECORDING_STATUS event from EventBus."""
        logger.debug("_on_audio_recording_status called with status: %s", status)
        self._schedule_ui_update(lambda s=status: self.audio_pane.update_audio_recording_status(s))

    def _on_ir_buttons_enabled(self, data):
        """Handle IR_BUTTONS_ENABLED event from EventBus."""
        logger.debug("_on_ir_buttons_enabled called with data: %s", data)
        if isinstance(data, dict):
            enabled = data.get('enabled', False)
            is_running = data.get('is_running', False)
        else:
            enabled = bool(data)
            is_running = False
        self._schedule_ui_update(lambda e=enabled, r=is_running: self.ir_pane.update_ir_buttons_enabled(e, r))

    def _on_ir_key_event(self, data):
        """Handle IR_KEY_EVENT from EventBus."""
        logger.debug("_on_ir_key_event called with data: %s", data)
        self._handle_ir_key_event(data)

    def _on_ai_analysis(self, results):
        """Handle AI_ANALYSIS event from EventBus."""
        logger.debug("_on_ai_analysis called with results: %s", results)
        self._schedule_ui_update(lambda r=results: self._handle_ai_analysis_complete(r))

    def _on_ai_error(self, error_data):
        """Handle AI_ERROR event from EventBus."""
        logger.debug("_on_ai_error called with error_data: %s", error_data)
        self._schedule_ui_update(lambda ed=error_data: self._handle_ai_error(ed))

    def _on_ai_button_state_changed(self, data):
        """Handle AI_BUTTON_STATE_CHANGED event from EventBus."""
        logger.debug("_on_ai_button_state_changed called with data: %s", data)
        if isinstance(data, dict):
            enabled = data.get('enabled', False)
        else:
            enabled = bool(data)
        self._schedule_ui_update(lambda e=enabled: self.audio_pane.update_ai_button_state(e))

    def _on_audio_playing_state(self, data=None):
        """Handle AUDIO_PLAYING_STATE event from EventBus."""
        logger.debug("_on_audio_playing_state called")
        self.set_audio_playing_state()

    def _on_audio_stopped_state(self, data=None):
        """Handle AUDIO_STOPPED_STATE event from EventBus."""
        logger.debug("_on_audio_stopped_state called")
        self.set_audio_stopped_state()

    def _on_ir_handler_ready(self, data):
        """Handle IR_HANDLER_READY event from EventBus."""
        logger.debug("_on_ir_handler_ready called with data: %s", data)
        if isinstance(data, dict) and 'ir_receiver' in data:
            ir_receiver = data['ir_receiver']
            self._schedule_ui_update(lambda ir=ir_receiver: self.ir_pane.set_ir_handler(ir))
