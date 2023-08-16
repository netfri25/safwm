#pragma once

#include <X11/XF86keysym.h>

#include "safwm.h"

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

#define MAPPING_COUNT (sizeof(keymaps) / sizeof(*keymaps))
static const Keymap keymaps[] = {
    // general window manager keys
    { MOD|ShiftMask, XK_q, quit_wm, { 0 }},


    // window controls
    { MOD, XK_c, center_win,     { 0 }},
    { MOD, XK_m, maximize_win,   { 0 }},
    { MOD, XK_f, fullscreen_win, { 0 }},
    { MOD, XK_q, close_win,      { 0 }},


    // workspace controls
    { MOD, XK_1, goto_ws, { .i = 0 }},
    { MOD, XK_2, goto_ws, { .i = 1 }},
    { MOD, XK_3, goto_ws, { .i = 2 }},
    { MOD, XK_4, goto_ws, { .i = 3 }},

    { MOD|ShiftMask, XK_1, move_win_to_ws, { .i = 0 }},
    { MOD|ShiftMask, XK_2, move_win_to_ws, { .i = 1 }},
    { MOD|ShiftMask, XK_3, move_win_to_ws, { .i = 2 }},
    { MOD|ShiftMask, XK_4, move_win_to_ws, { .i = 3 }},

    { MOD, XK_d, goto_next_ws, {0}},
    { MOD, XK_a, goto_prev_ws, {0}},
    { MOD|ShiftMask, XK_d, move_win_to_next_ws, {0}},
    { MOD|ShiftMask, XK_a, move_win_to_prev_ws, {0}},


    // applications
    { MOD, XK_s,      execute_cmd, { .com = MENU_CMD }},
    { MOD, XK_Return, execute_cmd, { .com = TERM_CMD }},
    { MOD, XK_b,      execute_cmd, { .com = BROWSER_CMD }},

    { ShiftMask, XK_Print, execute_cmd, { .com = SCREENSHOT_CMD }},
    { 0,         XK_Print, execute_cmd, { .com = SELECTION_SCREENSHOT_CMD }},

    { 0, XF86XK_AudioRaiseVolume, execute_cmd, { .com = VOLUP_CMD }},
    { 0, XF86XK_AudioLowerVolume, execute_cmd, { .com = VOLDOWN_CMD }},
    { 0, XF86XK_AudioMute,        execute_cmd, { .com = VOLMUTE_CMD }},
    { MOD, XK_F2, execute_cmd, { .com = VOLUP_CMD }},
    { MOD, XK_F1, execute_cmd, { .com = VOLDOWN_CMD }},

    { Mod1Mask, XK_F3, execute_cmd, { .com = MEDIA_NEXT_CMD }},
    { Mod1Mask, XK_F2, execute_cmd, { .com = MEDIA_PREV_CMD }},
    { Mod1Mask, XK_F1, execute_cmd, { .com = MEDIA_TOGGLE_CMD }},

    { 0, XF86XK_MonBrightnessUp,   execute_cmd, { .com = BRIUP_CMD }},
    { 0, XF86XK_MonBrightnessDown, execute_cmd, { .com = BRIDOWN_CMD }},
    { MOD, XK_F4, execute_cmd, { .com = BRIUP_CMD }},
    { MOD, XK_F3, execute_cmd, { .com = BRIDOWN_CMD }},
};
