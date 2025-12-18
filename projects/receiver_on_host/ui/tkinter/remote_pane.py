import tkinter as tk
from tkinter import ttk
import queue
from ui.ui import EventHandlers, ButtonId
from log import get_logger

logger = get_logger(__name__)

class RemotePane(ttk.Frame):
    """Tkinter-based pane for rendering virtual remote control. Buttons can be highlighted to indicate interaction."""

    def __init__(self, parent, handlers: EventHandlers):
        super().__init__(parent, padding=10)
        self.handlers = handlers

        self.ui_update_queue = queue.Queue()
        self._start_queue_processor()

        self.buttons = {}

        self.canvas = tk.Canvas(self, bg='white', width=400, height=800)
        self.canvas.pack(expand=True, fill="both")

        self._create_remote_layout()
        self.canvas.bind('<Configure>', self._on_canvas_resize)

    def _create_remote_layout(self, remote_x=None):
        """Create graphical layout of remote control. Draws body, buttons, and registers buttons for interaction."""
        remote_width, remote_height = 220, 850
        remote_y = 20

        if remote_x is None:
            try:
                canvas_width = self.canvas.winfo_width()
                if canvas_width <= 1:
                    canvas_width = int(self.canvas['width'])
            except Exception:
                canvas_width = 400
            remote_x = (canvas_width - remote_width) // 2

        self.canvas.create_rectangle(
            remote_x, remote_y,
            remote_x + remote_width, remote_y + remote_height,
            fill="#8F8F8F", outline='#202020', width=3
        )

        button_size = 35
        button_spacing = 45
        num_start_x = remote_x + remote_width // 2 - (button_size + button_spacing / 4)

        button_3_x = num_start_x + 2 * button_spacing

        power_x, power_y = button_3_x, remote_y + 50
        power_btn = self.canvas.create_oval(
          power_x - 20, power_y - 15,
          power_x + 20, power_y + 15,
          fill='#00cc00', outline='#008800', width=2
        )
        self.canvas.create_text(power_x, power_y, text="⏻", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(power_btn, ButtonId.POWER.value, '#00cc00')

        button_size = 35
        button_spacing = 45
        num_start_x, num_start_y = remote_x + remote_width//2 - (button_size + button_spacing / 4), remote_y + 110
        numbers = [
          ['1', '2', '3'],
          ['4', '5', '6'],
          ['7', '8', '9'],
          ['.', '0', 'APPS']
        ]

        button_map = {
            '1': ButtonId.NUM_1, '2': ButtonId.NUM_2, '3': ButtonId.NUM_3,
            '4': ButtonId.NUM_4, '5': ButtonId.NUM_5, '6': ButtonId.NUM_6,
            '7': ButtonId.NUM_7, '8': ButtonId.NUM_8, '9': ButtonId.NUM_9,
            '.': ButtonId.DOT, '0': ButtonId.NUM_0, 'APPS': ButtonId.APPS
        }

        for row_idx, row in enumerate(numbers):
          for col_idx, text in enumerate(row):
            x = num_start_x + col_idx * button_spacing
            y = num_start_y + row_idx * button_spacing

            btn = self.canvas.create_rectangle(
              x - button_size//2, y - button_size//2,
              x + button_size//2, y + button_size//2,
              fill='#707070', outline='#505050', width=1
            )

            if text == '.':
                font_size = 20
            elif text == 'APPS':
                font_size = 8
            else:
                font_size = 12
            self.canvas.create_text(x, y, text=text, fill='white', font=('Arial', font_size, 'bold'))

            self._register_button(btn, button_map[text].value, '#707070')

        stream_y = num_start_y + 4 * button_spacing + 30
        stream_center_x = remote_x + remote_width//2

        # Google Play button (blue rectangle)
        gplay_btn = self.canvas.create_rectangle(
          stream_center_x - 80, stream_y - 15,
          stream_center_x - 10, stream_y + 15,
          fill='#4285f4', outline='#1a73e8', width=2
        )
        self.canvas.create_text(stream_center_x - 45, stream_y, text="Google Play", fill='white', font=('Arial', 9, 'bold'))
        self._register_button(gplay_btn, ButtonId.GPLAY.value, '#4285f4')

        # Netflix button (red rectangle)
        netflix_btn = self.canvas.create_rectangle(
          stream_center_x + 10, stream_y - 15,
          stream_center_x + 80, stream_y + 15,
          fill='#e50914', outline='#b20710', width=2
        )
        self.canvas.create_text(stream_center_x + 45, stream_y, text="NETFLIX", fill='white', font=('Arial', 9, 'bold'))
        self._register_button(netflix_btn, ButtonId.NETFLIX.value, '#e50914')

        # Color function buttons (circles) - centered
        color_y = stream_y + 50
        colors = [
            ('#ffcc00', ButtonId.YELLOW.value),
            ('#0066cc', ButtonId.BLUE.value),
            ('#cc0000', ButtonId.RED.value),
            ('#00aa00', ButtonId.GREEN.value)
        ]

        for i, (color, btn_id) in enumerate(colors):
          x = stream_center_x - 52 + i * 35
          btn = self.canvas.create_oval(
            x - 12, color_y - 12,
            x + 12, color_y + 12,
            fill=color, outline='#000000', width=1
          )
          self._register_button(btn, btn_id, color)

        # Guide and DVR buttons
        guide_dvr_y = color_y + 40
        guide_btn = self.canvas.create_rectangle(
            stream_center_x - 70, guide_dvr_y - 12,
            stream_center_x - 10, guide_dvr_y + 12,
            fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x - 40, guide_dvr_y, text="GUIDE", fill='white', font=('Arial', 9, 'bold'))
        self._register_button(guide_btn, ButtonId.GUIDE.value, '#606060')

        dvr_btn = self.canvas.create_rectangle(
            stream_center_x + 10, guide_dvr_y - 12,
            stream_center_x + 70, guide_dvr_y + 12,
            fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x + 40, guide_dvr_y, text="DVR", fill='white', font=('Arial', 9, 'bold'))
        self._register_button(dvr_btn, ButtonId.DVR.value, '#606060')

        # INPUT, Microphone and Settings buttons - aligned with BACK, TV, and HOME buttons
        func_y = guide_dvr_y + 50

        # Align INPUT button with BACK button
        input_btn = self.canvas.create_oval(
            stream_center_x - 90, func_y - 15,
            stream_center_x - 40, func_y + 15,
            fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x - 65, func_y, text="IN", fill='white', font=('Arial', 10, 'bold'))
        self._register_button(input_btn, ButtonId.INPUT.value, '#606060')

        # Align Microphone button with TV button
        mic_btn = self.canvas.create_oval(
            stream_center_x - 20, func_y - 15,
            stream_center_x + 20, func_y + 15,
            fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x, func_y, text="🎤", fill='white', font=('Arial', 12, 'bold'))
        self._register_button(mic_btn, ButtonId.MIC.value, '#606060')

        # Align Settings button with HOME button
        settings_btn = self.canvas.create_oval(
            stream_center_x + 40, func_y - 15,
            stream_center_x + 90, func_y + 15,
            fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x + 65, func_y, text="⚙", fill='white', font=('Arial', 12, 'bold'))
        self._register_button(settings_btn, ButtonId.SETTINGS.value, '#606060')

        # Navigation cluster (directional pad + OK) - centered
        nav_center_x, nav_center_y = stream_center_x, func_y + 70

        # Up arrow
        up_btn = self.canvas.create_polygon(
          nav_center_x, nav_center_y - 40,
          nav_center_x - 15, nav_center_y - 20,
          nav_center_x + 15, nav_center_y - 20,
          fill='#808080', outline='#606060', width=1
        )
        self.canvas.create_text(nav_center_x, nav_center_y - 30, text="▲", fill='white', font=('Arial', 12, 'bold'))
        self._register_button(up_btn, ButtonId.UP.value, '#808080')

        # Left arrow
        left_btn = self.canvas.create_polygon(
          nav_center_x - 40, nav_center_y,
          nav_center_x - 20, nav_center_y - 15,
          nav_center_x - 20, nav_center_y + 15,
          fill='#808080', outline='#606060', width=1
        )
        self.canvas.create_text(nav_center_x - 30, nav_center_y, text="◄", fill='white', font=('Arial', 12, 'bold'))
        self._register_button(left_btn, ButtonId.LEFT.value, '#808080')

        # OK button (center circle)
        ok_btn = self.canvas.create_oval(
          nav_center_x - 20, nav_center_y - 20,
          nav_center_x + 20, nav_center_y + 20,
          fill='#0080ff', outline='#0060cc', width=2
        )
        self.canvas.create_text(nav_center_x, nav_center_y, text="OK", fill='white', font=('Arial', 11, 'bold'))
        self._register_button(ok_btn, ButtonId.OK.value, '#0080ff')

        # Right arrow
        right_btn = self.canvas.create_polygon(
          nav_center_x + 40, nav_center_y,
          nav_center_x + 20, nav_center_y - 15,
          nav_center_x + 20, nav_center_y + 15,
          fill='#808080', outline='#606060', width=1
        )
        self.canvas.create_text(nav_center_x + 30, nav_center_y, text="►", fill='white', font=('Arial', 12, 'bold'))
        self._register_button(right_btn, ButtonId.RIGHT.value, '#808080')

        # Down arrow
        down_btn = self.canvas.create_polygon(
          nav_center_x, nav_center_y + 40,
          nav_center_x - 15, nav_center_y + 20,
          nav_center_x + 15, nav_center_y + 20,
          fill='#808080', outline='#606060', width=1
        )
        self.canvas.create_text(nav_center_x, nav_center_y + 30, text="▼", fill='white', font=('Arial', 12, 'bold'))
        self._register_button(down_btn, ButtonId.DOWN.value, '#808080')

        # Control buttons (BACK, TV, HOME) - centered
        control_y = nav_center_y + 70

        back_btn = self.canvas.create_rectangle(
            stream_center_x - 90, control_y - 12,
            stream_center_x - 40, control_y + 12,
            fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x - 65, control_y, text="BACK", fill='white', font=('Arial', 8, 'bold'))
        self._register_button(back_btn, ButtonId.BACK.value, '#606060')

        tv_btn = self.canvas.create_rectangle(
            stream_center_x - 20, control_y - 12,
            stream_center_x + 20, control_y + 12,
            fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x, control_y, text="TV", fill='white', font=('Arial', 10, 'bold'))
        self._register_button(tv_btn, ButtonId.TV.value, '#606060')

        home_btn = self.canvas.create_rectangle(
            stream_center_x + 40, control_y - 12,
            stream_center_x + 90, control_y + 12,
            fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x + 65, control_y, text="HOME", fill='white', font=('Arial', 8, 'bold'))
        self._register_button(home_btn, ButtonId.HOME.value, '#606060')

        # Updated Volume and Channel controls - aligned with BACK and HOME buttons
        vol_ch_y = control_y + 60

        # Volume controls (left side)
        vol_up_btn = self.canvas.create_rectangle(
            stream_center_x - 90, vol_ch_y - 25,
            stream_center_x - 40, vol_ch_y - 10,
            fill='#707070', outline='#505050', width=1
        )
        self.canvas.create_text(stream_center_x - 65, vol_ch_y - 17, text="+", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(vol_up_btn, ButtonId.VOL_UP.value, '#707070')

        self.canvas.create_text(stream_center_x - 65, vol_ch_y, text="VOL", fill='white', font=('Arial', 8, 'bold'))

        vol_down_btn = self.canvas.create_rectangle(
            stream_center_x - 90, vol_ch_y + 10,
            stream_center_x - 40, vol_ch_y + 25,
            fill='#707070', outline='#505050', width=1
        )
        self.canvas.create_text(stream_center_x - 65, vol_ch_y + 17, text="−", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(vol_down_btn, ButtonId.VOL_DOWN.value, '#707070')

        # JUMP button (center)
        jump_btn = self.canvas.create_rectangle(
          stream_center_x - 30, vol_ch_y - 20,
          stream_center_x + 30, vol_ch_y - 5,
          fill='#666666', outline='#444444', width=1
        )
        self.canvas.create_text(stream_center_x, vol_ch_y - 12, text="JUMP", fill='white', font=('Arial', 9, 'bold'))
        self._register_button(jump_btn, ButtonId.JUMP.value, '#666666')

        # MUTE button (center)
        mute_btn = self.canvas.create_rectangle(
          stream_center_x - 30, vol_ch_y + 5,
          stream_center_x + 30, vol_ch_y + 20,
          fill='#666666', outline='#444444', width=1
        )
        self.canvas.create_text(stream_center_x, vol_ch_y + 12, text="MUTE", fill='white', font=('Arial', 9, 'bold'))
        self._register_button(mute_btn, ButtonId.MUTE.value, '#666666')

        # Channel controls (right side)
        ch_up_btn = self.canvas.create_rectangle(
            stream_center_x + 40, vol_ch_y - 25,
            stream_center_x + 90, vol_ch_y - 10,
            fill='#707070', outline='#505050', width=1
        )
        self.canvas.create_text(stream_center_x + 65, vol_ch_y - 17, text="+", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(ch_up_btn, ButtonId.CH_UP.value, '#707070')

        self.canvas.create_text(stream_center_x + 65, vol_ch_y, text="CH", fill='white', font=('Arial', 8, 'bold'))

        ch_down_btn = self.canvas.create_rectangle(
            stream_center_x + 40, vol_ch_y + 10,
            stream_center_x + 90, vol_ch_y + 25,
            fill='#707070', outline='#505050', width=1
        )
        self.canvas.create_text(stream_center_x + 65, vol_ch_y + 17, text="−", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(ch_down_btn, ButtonId.CH_DOWN.value, '#707070')

        # Audio, CC, Help buttons
        audio_cc_help_y = vol_ch_y + 45

        audio_btn = self.canvas.create_rectangle(
          stream_center_x - 90, audio_cc_help_y - 12,
          stream_center_x - 40, audio_cc_help_y + 12,
          fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x - 65, audio_cc_help_y, text="AUDIO", fill='white', font=('Arial', 8, 'bold'))
        self._register_button(audio_btn, ButtonId.AUDIO.value, '#606060')

        cc_btn = self.canvas.create_rectangle(
          stream_center_x - 20, audio_cc_help_y - 12,
          stream_center_x + 20, audio_cc_help_y + 12,
          fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x, audio_cc_help_y, text="CC", fill='white', font=('Arial', 10, 'bold'))
        self._register_button(cc_btn, ButtonId.CC.value, '#606060')

        help_btn = self.canvas.create_rectangle(
          stream_center_x + 40, audio_cc_help_y - 12,
          stream_center_x + 90, audio_cc_help_y + 12,
          fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x + 65, audio_cc_help_y, text="HELP", fill='white', font=('Arial', 8, 'bold'))
        self._register_button(help_btn, ButtonId.HELP.value, '#606060')

        # Media controls row - RR, Play, FF
        media_y = audio_cc_help_y + 45

        rr_btn = self.canvas.create_rectangle(
          stream_center_x - 90, media_y - 12,
          stream_center_x - 40, media_y + 12,
          fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x - 65, media_y, text="⏪", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(rr_btn, ButtonId.RW.value, '#606060')

        play_btn = self.canvas.create_rectangle(
          stream_center_x - 20, media_y - 12,
          stream_center_x + 20, media_y + 12,
          fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x, media_y, text="▶", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(play_btn, ButtonId.PLAY.value, '#606060')

        ff_btn = self.canvas.create_rectangle(
          stream_center_x + 40, media_y - 12,
          stream_center_x + 90, media_y + 12,
          fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x + 65, media_y, text="⏩", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(ff_btn, ButtonId.FF.value, '#606060')

        # Final media controls row - Rec, Pause, Display
        final_y = media_y + 50

        rec_btn = self.canvas.create_rectangle(
          stream_center_x - 90, final_y - 12,
          stream_center_x - 40, final_y + 12,
          fill='#aa0000', outline='#880000', width=1
        )
        self.canvas.create_text(stream_center_x - 65, final_y, text="●", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(rec_btn, ButtonId.REC.value, '#aa0000')

        pause_btn = self.canvas.create_rectangle(
          stream_center_x - 20, final_y - 12,
          stream_center_x + 20, final_y + 12,
          fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x, final_y, text="⏸", fill='white', font=('Arial', 14, 'bold'))
        self._register_button(pause_btn, ButtonId.PAUSE.value, '#606060')

        display_btn = self.canvas.create_rectangle(
          stream_center_x + 40, final_y - 12,
          stream_center_x + 90, final_y + 12,
          fill='#606060', outline='#404040', width=1
        )
        self.canvas.create_text(stream_center_x + 65, final_y, text="DISPLAY", fill='white', font=('Arial', 7, 'bold'))
        self._register_button(display_btn, ButtonId.DISPLAY.value, '#606060')

    def _register_button(self, canvas_item, button_id, original_color):
        self.buttons[button_id] = {
            'item': canvas_item,
            'original_color': original_color
        }

    def highlight_button(self, button_id, highlight=True):
        logger.debug1("highlight_button called with button_id=%s, highlight=%s", button_id, highlight)

        def _do_highlight():
            logger.debug1("_do_highlight executing for button_id=%s, highlight=%s", button_id, highlight)
            if button_id in self.buttons:
                btn_info = self.buttons[button_id]
                canvas_item = btn_info['item']

                if highlight:
                    # Bright yellow highlight
                    logger.debug1("Setting button %s to highlight color", button_id)
                    self.canvas.itemconfig(canvas_item, fill='#ffff00', outline='#ffcc00')
                else:
                    # Restore original color
                    original_color = btn_info['original_color']
                    # Calculate darker outline color
                    outline_color = self._darken_color(original_color)
                    logger.debug1("Restoring button %s to original color %s", button_id, original_color)
                    self.canvas.itemconfig(canvas_item, fill=original_color, outline=outline_color)
                logger.debug1("Canvas update completed for button %s", button_id)
            else:
                logger.debug1("Button %s not found in buttons dictionary", button_id)
                logger.debug1("Available buttons: %s", list(self.buttons.keys()))

        if hasattr(self, 'ui_update_queue'):
            logger.debug1("Queuing update (button_id=%s)", button_id)
            self.ui_update_queue.put(_do_highlight)
        else:
            # Fallback to direct scheduling if no queue available
            logger.debug1("Scheduling update via after() (button_id=%s)", button_id)
            self.after(0, _do_highlight)

    def _darken_color(self, hex_color):
        # Remove # and convert to RGB
        hex_color = hex_color.lstrip('#')
        rgb = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
        # Darken by reducing each component by 30%
        darkened = tuple(max(0, int(c * 0.7)) for c in rgb)
        # Convert back to hex
        return f"#{darkened[0]:02x}{darkened[1]:02x}{darkened[2]:02x}"

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

    def _on_canvas_resize(self, event):
        self.canvas.delete('all')
        remote_x = (event.width - 300) // 2
        self._create_remote_layout(remote_x=remote_x)
