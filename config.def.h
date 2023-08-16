#pragma once

#include <X11/XF86keysym.h>

#define MOD Mod4Mask

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

#define BORDER_NORMAL 0x7a8478
#define BORDER_FOCUS  0xd3c6aa
#define BORDER_NONE   0x000000
#define BORDER_WIDTH  3
#define WINDOW_GAP    10

#define SAVE_SCREENSHOT "tee $HOME/Pictures/Screenshots/$(date +'Screenshot-from-%Y-%m-%d-%H-%M-%S').png | xclip -selection clipboard -t image/png"
#define SCREENSHOT_CMD           "maim | "    SAVE_SCREENSHOT
#define SELECTION_SCREENSHOT_CMD "maim -s | " SAVE_SCREENSHOT

#define MENU_CMD    "rofi -show drun -theme gruvbox-dark"
#define TERM_CMD    "alacritty"
#define BROWSER_CMD "firefox"

#define BRIUP_CMD   "brightnessctl set 2%+ -n"
#define BRIDOWN_CMD "brightnessctl set 2%- -n"

#define VOLUP_CMD   "amixer sset Master 5%+"
#define VOLDOWN_CMD "amixer sset Master 5%-"
#define VOLMUTE_CMD "amixer sset Master toggle"

#define MEDIA_NEXT_CMD   "playerctl next"
#define MEDIA_PREV_CMD   "playerctl previous"
#define MEDIA_TOGGLE_CMD "playerctl play-pause"
