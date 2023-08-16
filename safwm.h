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

// Window client is the WindowID (in x11 it's called just Window), with some additional information
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



// #############
// # Workspace #
// #############

// this can be changed based on your need, but it probably shouldn't
// for me, even 3 is enough because I'm mostly using every workspace for
// one application only and I use fullscreen
#define WORKSPACE_CLIENTS_CAPACITY 64

// I really wanted to put it in "config.h", but the c includes can't
// allow mutually recursive modules, and it sucks.
#define WORKSPACE_COUNT 4

typedef struct {
    WindowClient client[WORKSPACE_CLIENTS_CAPACITY]; // might have holes
    size_t focused_index; // the index of the focused_client
} Workspace;

// focus all the windows in the current workspace
void ws_focus(const Workspace* ws);

// unfocus all the windows in the current workspace
void ws_unfocus(const Workspace* ws);

// returns the index of the next empty client slot in the workspace
// sets `is_full` to true in case the workspace is full
size_t ws_next_empty(const Workspace* ws);

// find the given WindowClient inside the workspace, and return it
// WARN: returns NULL when the window hasn't been found
// INFO: not const, because returning a pointer from the workspace forces
//       the pointer to be const if the workspace is const
WindowClient* ws_find(Workspace* ws, Window window);

// sets the client at the given index to the given client
// returns the previous window client at that index (useful when wanting to do swaps)
WindowClient ws_set_client(Workspace* ws, size_t index, WindowClient client);

// removes the client from the workspace at the given index
// returns the previous window client at that index (useful when wanting to do swaps)
WindowClient ws_remove_client(Workspace* ws, size_t index);

// move a window from one workspace to another workspace
void ws_move_client(size_t from, size_t to, Window window);


// #################################
// # General structres + functions #
// #################################

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

    Workspace workspace[WORKSPACE_COUNT];
    size_t current_ws; // the currently visible workspace

    int screen; // the screen identifier
} WM;


// return the currently focused WindowClient
WindowClient* wm_client(void);

// return the currently visible workspace
Workspace* wm_workspace(void);

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

// go to the given workspace
void goto_ws(Arg arg);

// go to the next workspace
void goto_next_ws(Arg arg);

// go to the previous workspace
void goto_prev_ws(Arg arg);

// move the currently focused window to the given workspace
void move_win_to_ws(Arg arg);

// move the currently focused window to the next workspace
void move_win_to_next_ws(Arg arg);

// move the currently focused window to the previous workspace
void move_win_to_prev_ws(Arg arg);
