import tkinter as tk
from tkinter import ttk

# Silicon Labs Light Theme Color Palette
SILABS_LIGHT = {
    "bg": "#F6F7F9",       # app background
    "surface": "#FFFFFF",  # card/sheet surfaces
    "fg": "#1F2937",       # primary text
    "fg_muted": "#4B5563", # secondary text
    "border": "#E5E7EB",   # hairline separators / outlines
    "select": "#E8F0FE",   # selection background for lists/trees
    "accent": "#D71920",   # Silicon Labs red
    "accent_hover": "#B31218",
    "focus": "#3B82F6"     # subtle focus ring
}

def apply_default_fonts(root):
    root.option_add("*Font", ("Segoe UI", 10))
    root.option_add("*Treeview.Font", ("Segoe UI", 10))
    root.option_add("*TEntry*Font", ("Segoe UI", 10))
    root.option_add("*TButton*Font", ("Segoe UI", 10))
    root.option_add("*Text.Font", ("Consolas", 10))

def create_silabs_light_theme(style: ttk.Style, palette: dict = SILABS_LIGHT):
    """Create Silicon Labs branded light theme for ttk widgets. Features light backgrounds and red accent color. """

    if "SilabsLight" in style.theme_names():
        return

    p = palette

    style.theme_create("SilabsLight", parent="clam", settings={
        # Base configuration for all widgets
        ".": {"configure": {
            "background": p["bg"],
            "foreground": p["fg"],
            "fieldbackground": p["surface"],
            "troughcolor": p["surface"],
            "bordercolor": p["border"],
            "lightcolor": p["border"],
            "darkcolor": p["border"],
            "focuscolor": p["focus"],
            "padding": 4
        }},

        # Frame widgets
        "TFrame": {
            "configure": {"background": p["bg"]}
        },
        "TLabelframe": {
            "configure": {
                "background": p["bg"],
                "borderwidth": 1,
                "relief": "solid"
            }
        },
        "TLabelframe.Label": {
            "configure": {
                "background": p["bg"],
                "foreground": p["fg_muted"]
            }
        },

        # Text labels
        "TLabel": {
            "configure": {
                "background": p["bg"],
                "foreground": p["fg"]
            }
        },
        "Header.TLabel": {
            "configure": {
                "font": ("Segoe UI", 11, "bold"),
                "foreground": p["fg"]
            }
        },

        # Separator
        "TSeparator": {
            "configure": {"background": p["border"]}
        },

        # Primary button: Silicon Labs red
        "TButton": {
            "configure": {
                "padding": (10, 6),
                "foreground": "#FFFFFF",
                "background": p["accent"],
                "borderwidth": 1,
                "relief": "flat"
            },
            "map": {
                "background": [("active", p["accent_hover"]), ("disabled", p["border"])],
                "foreground": [("disabled", p["fg_muted"])],
                "relief": [("pressed", "flat")]
            }
        },

        # Secondary button (use style='Secondary.TButton' where needed)
        "Secondary.TButton": {
            "configure": {
                "padding": (10, 6),
                "foreground": p["fg"],
                "background": p["surface"],
                "borderwidth": 1,
                "relief": "solid"
            },
            "map": {
                "background": [("active", p["select"]), ("disabled", p["bg"])],
                "foreground": [("disabled", p["fg_muted"])]
            }
        },

        # Input widgets
        "TEntry": {
            "configure": {
                "fieldbackground": p["surface"],
                "foreground": p["fg"],
                "padding": (8, 6),
                "borderwidth": 1,
                "relief": "solid"
            }
        },
        "TCombobox": {
            "configure": {
                "fieldbackground": p["surface"],
                "foreground": p["fg"],
                "padding": (8, 6),
                "borderwidth": 1,
                "relief": "solid"
            }
        },

        # Notebook (tabs)
        "TNotebook": {
            "configure": {
                "background": p["bg"],
                "tabmargins": [6, 4, 0, 0]
            }
        },
        "TNotebook.Tab": {
            "configure": {
                "padding": (14, 8),
                "background": p["bg"],
                "foreground": p["fg_muted"],
                "borderwidth": 0
            },
            "map": {
                "background": [("selected", p["surface"]), ("active", p["surface"])],
                "foreground": [("selected", p["fg"])]
            }
        },

        # Treeview
        "Treeview": {
            "configure": {
                "background": p["surface"],
                "fieldbackground": p["surface"],
                "foreground": p["fg"],
                "borderwidth": 1
            }
        },
        "Treeview.Heading": {
            "configure": {
                "background": p["bg"],
                "foreground": p["fg_muted"],
                "borderwidth": 0
            }
        },

        # Scrollbars
        "Vertical.TScrollbar": {
            "configure": {
                "background": p["surface"],
                "troughcolor": p["bg"]
            }
        },
        "Horizontal.TScrollbar": {
            "configure": {
                "background": p["surface"],
                "troughcolor": p["bg"]
            }
        },

        # Panedwindow
        "TPanedwindow": {
            "configure": {"background": p["bg"]}
        },
    })

def apply_silabs_light_theme(root):
    """Apply Silicon Labs light theme to Tkinter application. Applies fonts and custom theme.

    Args:
        root: Tkinter root window
    """
    # Apply fonts first
    apply_default_fonts(root)

    # Create and apply the theme
    style = ttk.Style(root)
    create_silabs_light_theme(style)
    style.theme_use("SilabsLight")

    # Ensure Treeview selected row uses the theme's selection color
    style.map(
        "Treeview",
        background=[("selected", SILABS_LIGHT["select"])],
        foreground=[("selected", SILABS_LIGHT["fg"])]
    )

    # Set root background to match theme
    root.configure(bg=SILABS_LIGHT["bg"])
