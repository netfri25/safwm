#include <X11/XF86keysym.h>

#include "safwm.h"
#include "config.h"

const Keymap keymaps[] = {
    // general window manager keys
    { MOD|ShiftMask, XK_q, quit_wm, {0}},


    // window controls
    { MOD, XK_c, center_win,     {0}},
    { MOD, XK_m, maximize_win,   {0}},
    { MOD, XK_f, fullscreen_win, {0}},
    { MOD, XK_q, close_win,      {0}},

    { MOD, XK_k, win_shrink, { .i = D_UP }},
    { MOD, XK_j, win_shrink, { .i = D_DOWN }},
    { MOD, XK_l, win_shrink, { .i = D_RIGHT }},
    { MOD, XK_h, win_shrink, { .i = D_LEFT }},

    { MOD|ShiftMask, XK_k, win_move, { .i = D_UP }},
    { MOD|ShiftMask, XK_j, win_move, { .i = D_DOWN }},
    { MOD|ShiftMask, XK_l, win_move, { .i = D_RIGHT }},
    { MOD|ShiftMask, XK_h, win_move, { .i = D_LEFT }},

    { MOD, XK_Left,  win_swap_prev, {0}},
    { MOD, XK_Right, win_swap_next, {0}},

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

    { Mod1Mask,           XK_Tab, win_next, {0}},
    { Mod1Mask|ShiftMask, XK_Tab, win_prev, {0}},

    { MOD, XK_v, ws_toggle_visibility, {0}},


    // applications
    { MOD, XK_s,      execute_cmd, { .com = MENU_CMD }},
    { MOD, XK_Return, execute_cmd, { .com = TERM_CMD }},
    { MOD, XK_b,      toggle_bar,  {0}},

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
