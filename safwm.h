#pragma once

#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <stdbool.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
    int x, y;
    unsigned w, h;
} Rect;



// ################
// # WindowClient #
// ################

typedef struct WindowClient {
    Window window;
    bool fullscreen;
    Rect rect; // the rectangle where the window is contained
    Rect prev; // the previous status, useful for fullscreen toggle
} WindowClient;

// construct a new WindowClient out of a window (also gets all the needed info)
WindowClient client_from_window(Window window);

// focus a window
void client_focus(const WindowClient* client);

// close a window
void client_quit(const WindowClient* client);

// centers a window
// INFO: not const, because it changes ->rect
void client_center(WindowClient* client);

// resizes and moves the window to the current `client->rect`
void client_update_rect(const WindowClient* client);

// maximizes a window
// INFO: not const, because it changes ->w and ->h
void client_maximize(WindowClient* client);

// sets the client to fullscreen
// INFO: not const, because it changes ->prev, ->rect
void client_fullscreen(WindowClient* client);

// reverts the client from fullscreen-mode to windowed mode
// INFO: not const, because it changes ->rect
void client_unfullscreen(WindowClient* client);

typedef struct Arg {
    const char* com;
    int i;
} Arg;

typedef void (*callback_t)(Arg arg);

typedef struct Keymap {
    unsigned int mod;
    KeySym key;
    callback_t callback;
    Arg arg;
} Keymap;

typedef struct WM {
    Display* display;
    Window root;
    bool keep_alive;
    XButtonEvent mouse;
    WindowClient current;

    struct {
        int id;
        unsigned w, h;
    } screen;
} WM;



// #####################
// # General functions #
// #####################

// grabs the input globally
void grab_global_input(void);

// grabs the input only in a specified window
void grab_window_input(Window window);



// ##################
// # Event handling #
// ##################

void event_button_press(XEvent* event);
void event_button_release(XEvent* event);
void event_configure_request(XEvent* event);
void event_key_press(XEvent* event);
void event_map_request(XEvent* event);
void event_mapping(XEvent* event);
void event_destroy(XEvent* event);
void event_enter(XEvent* event);
void event_motion(XEvent* event);

// handle x11 errors
int error_handler(Display* display, XErrorEvent* err);


// ###############################
// # Helper functions            #
// # mostly used for keybindings #
// ###############################

// quit the window manager
void quit_wm(Arg arg);

// executes a shell command
void execute_cmd(Arg arg);

// close the current window
void close_win(Arg arg);

// center the current window
void center_win(Arg arg);

// maximizes the current window
void maximize_win(Arg arg);

// sets the current window to fullscreen
void fullscreen_win(Arg arg);
