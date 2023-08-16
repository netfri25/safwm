#pragma once

#include "safwm.h"

#define MOD Mod4Mask

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

#define BORDER_NORMAL "#7a8478"
#define BORDER_FOCUS  "#d3c6aa"
#define BORDER_NONE   "#000000"
#define BORDER_WIDTH  3
#define WINDOW_GAP    10

#define APPLICATION_RUNNER_CMD "rofi -show drun -theme gruvbox-dark"
#define TERMINAL_CMD           "alacritty"

#define MAPPING_COUNT (sizeof(keymaps) / sizeof(*keymaps))
static const Keymap keymaps[] = {
    // general window manager keys
    { MOD|ShiftMask, XK_q, quit_wm, { 0 }},


    // applications
    { MOD, XK_s,      execute_cmd, { .com = APPLICATION_RUNNER_CMD }},
    { MOD, XK_Return, execute_cmd, { .com = TERMINAL_CMD }},


    // window controls
    { MOD, XK_c, center_win,     { 0 }},
    { MOD, XK_m, maximize_win,   { 0 }},
    { MOD, XK_f, fullscreen_win, { 0 }},
    { MOD, XK_q, close_win,      { 0 }},


    // workspace related
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
};
