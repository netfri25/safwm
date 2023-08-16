// TODO:
// [*] make it actually work (lowest priority)
// [*] error handler
// [*] maximize key (win + m)
// [*] fullscreen toggle option (win + f)
// [*] workspaces
// [*] add all of the keybindings from my old wm
// [*] Alt+Tab window switching in the current workspace
// [*] simple pseudo-tiling support
// [ ] "undo" for the pseudo-tiling
// [ ] make maximize toggleable (maybe?)
// [ ] middle click on a window to set the input focus only to him ("pinning")
// [ ] support for external bars (simple padding at the top)
// [ ] add an array of window names to not center on creation (e.g. "neovide")

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/X.h>

#include "safwm.h"
#include "config.h"
#include "keymaps.h"

#define MOD_MASK(mask) (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define MAPPING_COUNT (sizeof(keymaps) / sizeof(*keymaps))

// btw I really like to steal from https://github.com/dylanaraps/sowm

static void update_numlock_mask(void);
static size_t next_ws(void);
static size_t prev_ws(void);

static unsigned numlockmask = 0;

static WM wm = {0};

static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = event_button_press,
    [ButtonRelease]    = event_button_release,
    [ConfigureRequest] = event_configure_request,
    [KeyPress]         = event_key_press,
    [MapRequest]       = event_map_request,
    [MappingNotify]    = event_mapping,
    [DestroyNotify]    = event_destroy,
    [EnterNotify]      = event_enter,
    [MotionNotify]     = event_motion
};

int main(void) {
    wm = (WM){0};
    wm.display = XOpenDisplay(NULL);
    if (wm.display == NULL) return 1;
    wm.root = DefaultRootWindow(wm.display);
    wm.screen = DefaultScreen(wm.display);
    wm.mouse.subwindow = None;

    Cursor cursor = XCreateFontCursor(wm.display, 68);
    XDefineCursor(wm.display, wm.root, cursor);
    XSetErrorHandler(error_handler);

    XSelectInput(wm.display, wm.root, SubstructureRedirectMask);
    grab_global_input();

    wm.keep_alive = true;
    while (wm.keep_alive) {
        XEvent event;
        XNextEvent(wm.display, &event);
        if (events[event.type]) events[event.type](&event);
    }

    XCloseDisplay(wm.display);
}

WindowClient client_from_window(Window window) {
    WindowClient client = {0};
    client.window = window;
    client.fullscreen = false;

    Window root_return;
    unsigned _border_width_return;
    unsigned _depth_return;
    XGetGeometry(
        wm.display,
        window,
        &root_return,
        &client.rect.x,
        &client.rect.y,
        &client.rect.w,
        &client.rect.h,
        &_border_width_return,
        &_depth_return
    );

    client.prev = client.rect;
    return client;
}

void client_focus(const WindowClient* client) {
    XSetInputFocus(wm.display, client->window, RevertToParent, CurrentTime);
    if (wm_client()->window != None) XSetWindowBorder(wm.display, wm_client()->window, BORDER_NORMAL);
    Workspace* ws = wm_workspace();
    ws->focused_index = client - ws->client;

    // TODO: handle fullscreen
    // TODO: add green border for windows that are "pinned"
    XSetWindowBorder(wm.display, client->window, BORDER_FOCUS);
    XConfigureWindow(wm.display, client->window, CWBorderWidth, &(XWindowChanges) { .border_width = BORDER_WIDTH });
}

void client_center(WindowClient* client) {
    // TODO: add support for external bars
    client->rect.x = (SCREEN_WIDTH - client->rect.w) / 2 - BORDER_WIDTH;
    client->rect.y = (SCREEN_HEIGHT - client->rect.h) / 2 - BORDER_WIDTH;
    client_update_rect(client);
}

void client_update_rect(const WindowClient* client) {
    XMoveResizeWindow(
        wm.display,
        client->window,
        client->rect.x,
        client->rect.y,
        client->rect.w,
        client->rect.h
    );
}

void client_maximize(WindowClient* client) {
    if (client->fullscreen) client_unfullscreen(client);
    // TODO: add support for external bars
    client->rect = (Rect) {
        .w = SCREEN_WIDTH - 2 * BORDER_WIDTH - 2 * WINDOW_GAP,
        .h = SCREEN_HEIGHT - 2 * BORDER_WIDTH - 2 * WINDOW_GAP,
    };

    client_center(client);
}

void client_fullscreen(WindowClient* client) {
    if (client->fullscreen) return;
    client->fullscreen = true;
    client->prev = client->rect;
    client->rect = (Rect) {
        .w = SCREEN_WIDTH,
        .h = SCREEN_HEIGHT,
        .x = -BORDER_WIDTH,
        .y = -BORDER_WIDTH,
    };

    client_update_rect(client);
}

void client_unfullscreen(WindowClient* client) {
    if (!client->fullscreen) return;
    client->fullscreen = false;
    client->rect = client->prev;
    client_update_rect(client);
}

void ws_focus(const Workspace* ws) {
    for (size_t i = 0; i < WORKSPACE_CLIENTS_CAPACITY; i++) {
        // this two lines make sure that the currently focused client is the
        // last window to get mapped
        size_t index_from_back = WORKSPACE_CLIENTS_CAPACITY - i - 1;
        size_t index = (index_from_back + ws->focused_index) % WORKSPACE_CLIENTS_CAPACITY;

        const WindowClient* client = ws->client + index;
        if (client->window == None) continue;
        XMapWindow(wm.display, client->window);
    }
}

void ws_unfocus(const Workspace* ws) {
    for (size_t i = 0; i < WORKSPACE_CLIENTS_CAPACITY; i++) {
        const WindowClient* client = ws->client + i;
        if (client->window == None) continue;
        XUnmapWindow(wm.display, client->window);
    }
}

size_t ws_next_empty(const Workspace* ws) {
    size_t i;
    for (i = 0; i < WORKSPACE_CLIENTS_CAPACITY; i++)
        if (ws->client[i].window == None) break;

    bool is_full = i == WORKSPACE_CLIENTS_CAPACITY;
    if (is_full) {
        fprintf(stderr, "ERROR: workspace is full and can't fit more windows.\n");
        exit(1);
    }

    return i;
}

WindowClient* ws_next_window(Workspace* ws) {
    size_t off = ws->focused_index;
    for (size_t i = 0; i < WORKSPACE_CLIENTS_CAPACITY; i++) {
        size_t index = (i + off + 1) % WORKSPACE_CLIENTS_CAPACITY;
        WindowClient* client = ws->client + index;
        if (client->window == None) continue;
        return client;
    }

    return NULL;
}

WindowClient* ws_prev_window(Workspace* ws) {
    size_t off = ws->focused_index;
    for (size_t i = 0; i < WORKSPACE_CLIENTS_CAPACITY; i++) {
        size_t index_from_back = WORKSPACE_CLIENTS_CAPACITY - i - 1;
        size_t index = (index_from_back + off) % WORKSPACE_CLIENTS_CAPACITY;
        WindowClient* client = ws->client + index;
        if (client->window == None) continue;
        return client;
    }

    return NULL;
}

WindowClient* ws_find(Workspace* ws, Window window) {
    for (size_t i = 0; i < WORKSPACE_CLIENTS_CAPACITY; i++) {
        WindowClient* client = ws->client + i;
        if (window == client->window)
            return client;
    }

    return NULL;
}

WindowClient ws_set_client(Workspace* ws, size_t index, WindowClient client) {
    assert(index < WORKSPACE_CLIENTS_CAPACITY && "out of bounds");

    WindowClient prev = ws->client[index];
    ws->client[index] = client;
    return prev;
}

WindowClient ws_remove_client(Workspace* ws, size_t index) {
    return ws_set_client(ws, index, (WindowClient){ .window = None });
}

void ws_move_client(size_t from, size_t to, Window window) {
    assert(from < WORKSPACE_CLIENTS_CAPACITY && "out of bounds");
    assert(to   < WORKSPACE_CLIENTS_CAPACITY && "out of bounds");
    Workspace* from_ws = wm.workspace + from;
    Workspace* to_ws   = wm.workspace + to;

    WindowClient* client = ws_find(from_ws, window);
    assert(client && "client should always be in the `from` workspace");
    // WARN: pointer arithmetics
    size_t client_original_index = client - from_ws->client;
    size_t client_new_index = ws_next_empty(to_ws);
    ws_set_client(to_ws, client_new_index, *client);
    ws_remove_client(from_ws, client_original_index);
}

WindowClient* wm_client(void) {
    // using a pointer, because Workspace is a large struct (700+ bytes)
    // and I don't wanna copy it to the stack
    Workspace* current_ws = wm_workspace();
    return current_ws->client + current_ws->focused_index;
}

Workspace* wm_workspace(void) {
    return wm.workspace + wm.current_ws;
}

void grab_global_input(void) {
    grab_window_input(wm.root);
}

void grab_window_input(Window window) {
    XUngrabKey(wm.display, AnyKey, AnyModifier, window);
    XUngrabButton(wm.display, AnyButton, AnyModifier, window);

    unsigned int mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    int mode = GrabModeAsync;

    // mouse buttons with main modifier key
    XGrabButton(wm.display, Button1, MOD, window, false, mask, mode, mode, None, None); // win + LMB
    XGrabButton(wm.display, Button3, MOD, window, false, mask, mode, mode, None, None); // win + RMB

    // all the user defined keymappings
    for (size_t i = 0; i < MAPPING_COUNT; i++) {
        Keymap keymap = keymaps[i];
        KeyCode code = XKeysymToKeycode(wm.display, keymap.key);
        XGrabKey(wm.display, code, keymap.mod, window, false, mode, mode);
    }
}

void event_button_press(XEvent* event) {
    Window window = event->xbutton.subwindow;

    // I don't care if there is a click on nothing
    if (window == None) return;

    WindowClient* client = ws_find(wm_workspace(), window);
    assert(client && "how did you click a client that doesn't exist in the visible workspace?");

    // WARN: pointer arithmetics
    size_t client_index = client - wm_workspace()->client;
    wm_workspace()->focused_index = client_index;
    wm.mouse = event->xbutton;
    XRaiseWindow(wm.display, window);
}

void event_button_release(XEvent* event) {
    (void) event;
    wm.mouse.subwindow = None;
}

void event_configure_request(XEvent* event) {
    XConfigureRequestEvent *ev = &event->xconfigurerequest;

    XConfigureWindow(wm.display, ev->window, ev->value_mask, &(XWindowChanges) {
        .x          = ev->x,
        .y          = ev->y,
        .width      = ev->width,
        .height     = ev->height,
        .sibling    = ev->above,
        .stack_mode = ev->detail
    });
}

void event_key_press(XEvent* event) {
    update_numlock_mask();

    XKeyEvent xkey = event->xkey;
    for (size_t i = 0; i < MAPPING_COUNT; i++) {
        Keymap keymap = keymaps[i];
        KeyCode code = XKeysymToKeycode(wm.display, keymap.key);

        if (code != xkey.keycode) continue;
        if (MOD_MASK(keymap.mod) != MOD_MASK(xkey.state)) continue;
        keymap.callback(keymap.arg);
    }
}

void event_map_request(XEvent* event) {
    // gets called when a new window is created, and it wants to "map" to the screen
    Window window = event->xmaprequest.window;
    XSelectInput(wm.display, window, StructureNotifyMask|EnterWindowMask);
    XMapWindow(wm.display, window);
    WindowClient this_client = client_from_window(window);

    // TODO: proper error handling
    Workspace* current_ws = wm_workspace();
    size_t client_index = ws_next_empty(current_ws);

    ws_set_client(current_ws, client_index, this_client);

    WindowClient* current_client = wm_workspace()->client + client_index;
    client_focus(current_client);
    int x = current_client->rect.x;
    int y = current_client->rect.y;
    if (x + y == 0) client_center(current_client);
}

void event_mapping(XEvent* event) {
    XMappingEvent* ev = &event->xmapping;
    if (ev->request == MappingKeyboard || ev->request == MappingModifier) {
        XRefreshKeyboardMapping(ev);
        grab_global_input();
    }
}

void event_destroy(XEvent* event) {
    // I can prioritize the current workspace when searching for the window,
    // but WORKSPACE_COUNT * WORKSPACE_CLIENTS_CAPACITY is a really small
    // number of windows to loop through (for a computer ofc), so I don't
    // think it should be optimized
    for (size_t i = 0; i < WORKSPACE_COUNT; i++) {
        Workspace* workspace = wm.workspace + i;
        WindowClient* client = ws_find(workspace, event->xdestroywindow.window);
        if (client == NULL) continue;
        // WARN: pointer arithmetics for finding the client index
        size_t client_index = client - workspace->client;
        ws_remove_client(workspace, client_index);
        break;
    }
}

void event_enter(XEvent* event) {
    while (XCheckTypedEvent(wm.display, EnterNotify, event));
    while (XCheckTypedWindowEvent(wm.display, wm.mouse.subwindow, MotionNotify, event));

    WindowClient* client = ws_find(wm_workspace(), event->xcrossing.window);
    if (client == NULL) return;
    client_focus(client);
}

void event_motion(XEvent* event) {
    bool is_pressed = wm.mouse.button == Button1 || wm.mouse.button == Button3;
    if (!is_pressed) return;

    WindowClient* client = ws_find(wm_workspace(), wm.mouse.subwindow);
    if (client == NULL || client->window == None || client->fullscreen) return;

    while (XCheckTypedEvent(wm.display, MotionNotify, event));
    while (XCheckTypedWindowEvent(wm.display, client->window, MotionNotify, event));

    int dx = event->xmotion.x_root - wm.mouse.x_root;
    int dy = event->xmotion.y_root - wm.mouse.y_root;

    if (wm.mouse.button == Button1) {
        client->rect.x += dx;
        client->rect.y += dy;
        client_update_rect(client);
    }

    if (wm.mouse.button == Button3) {
        client->rect.w = MAX(1, client->rect.w + dx);
        client->rect.h = MAX(1, client->rect.h + dy);
        client_update_rect(client);
    }

    wm.mouse.x_root = event->xmotion.x_root;
    wm.mouse.y_root = event->xmotion.y_root;
}

int error_handler(Display* display, XErrorEvent* err) {
    char err_msg[1024];
    XGetErrorText(display, err->error_code, err_msg, sizeof(err_msg));
    fprintf(stderr, "X-Error(req: %u, code: %u, res_id: %lu)\n", err->request_code, err->error_code, err->resourceid);
    return 0;
}

void quit_wm(Arg arg) {
    (void) arg;
    wm.keep_alive = false;
}

void execute_cmd(Arg arg) {
    if (fork()) return;
    if (wm.display) close(ConnectionNumber(wm.display));

    setsid();
    (void) !system(arg.com);
}

void close_win(Arg arg) {
    // I have no clue what this does
    (void) arg;
    XEvent msg = {0};
    msg.xclient.type = ClientMessage;
    msg.xclient.message_type = XInternAtom(wm.display, "WM_PROTOCOLS", false);
    msg.xclient.window = wm_client()->window;
    msg.xclient.format = 32;
    msg.xclient.data.l[0] = XInternAtom(wm.display, "WM_DELETE_WINDOW", false);
    XSendEvent(wm.display, wm_client()->window, false, 0, &msg);

    // WARN: pointer arithmetics
    size_t client_index = wm_client() - wm_workspace()->client;
    ws_remove_client(wm_workspace(), client_index);
}

void center_win(Arg arg) {
    (void) arg;
    client_center(wm_client());
}

void maximize_win(Arg arg) {
    (void) arg;
    client_maximize(wm_client());
}

void fullscreen_win(Arg arg) {
    (void) arg;

    if (wm_client()->fullscreen) {
        client_unfullscreen(wm_client());
    } else {
        client_fullscreen(wm_client());
    }
}

static size_t next_ws(void) {
    size_t ws = wm.current_ws;
    ws += 1;
    ws %= WORKSPACE_COUNT;
    return ws;
}

static size_t prev_ws(void) {
    size_t ws = wm.current_ws;
    ws += WORKSPACE_COUNT - 1;
    ws %= WORKSPACE_COUNT;
    return ws;
}

void goto_ws(Arg arg) {
    size_t new_ws_index = arg.i;
    Workspace* new_ws = wm.workspace + new_ws_index;
    Workspace* old_ws = wm_workspace();

    ws_unfocus(old_ws);
    ws_focus(new_ws);
    wm.current_ws = new_ws_index;
}

void goto_next_ws(Arg arg) {
    (void) arg;
    size_t next_ws_index = next_ws();
    goto_ws((Arg){ .i = next_ws_index });
}

void goto_prev_ws(Arg arg) {
    (void) arg;
    size_t prev_ws_index = prev_ws();
    goto_ws((Arg){ .i = prev_ws_index });
}

void move_win_to_ws(Arg arg) {
    Window window = wm_client()->window;
    size_t from = wm.current_ws;
    size_t to   = arg.i;
    ws_unfocus(wm.workspace + wm.current_ws);
    ws_move_client(from, to, window);
    ws_focus(wm.workspace + wm.current_ws);
}

void move_win_to_next_ws(Arg arg) {
    size_t next_ws_index = next_ws();
    arg.i = next_ws_index;
    move_win_to_ws(arg);
    goto_ws(arg);
}

void move_win_to_prev_ws(Arg arg) {
    size_t prev_ws_index = prev_ws();
    arg.i = prev_ws_index;
    move_win_to_ws(arg);
    goto_ws(arg);
}

void win_next(Arg arg) {
    (void) arg;
    WindowClient* client = ws_next_window(wm_workspace());
    if (client == NULL) return;
    client_focus(client);
    XRaiseWindow(wm.display, client->window);
}

void win_prev(Arg arg) {
    (void) arg;
    WindowClient* client = ws_prev_window(wm_workspace());
    if (client == NULL) return;
    client_focus(client);
    XRaiseWindow(wm.display, client->window);
}

// coolest fucking code I've ever written
void win_slice(Arg arg) {
    WindowClient* client = wm_client();
    if (!client || client->window == None || client->fullscreen) return;

    enum Direction direction = arg.i;
    bool x_axis = direction & 1;
    bool opposite = (direction >> 1) & 1;
    float ratio = opposite ? 1 - SNAP_RATIO : SNAP_RATIO;

    int*      pos  = (int*      [2]) { &client->rect.y, &client->rect.x }[x_axis];
    unsigned* size = (unsigned* [2]) { &client->rect.h, &client->rect.w }[x_axis];

    // size = size0 * ratio - gap
    float new_size = *size * ratio - WINDOW_GAP;

    // delta = size0 * (1 - ratio) + gap
    float delta = *size * (1 - ratio) + WINDOW_GAP;

    *pos += opposite * delta;
    *size = MAX(1, new_size);

    client_update_rect(client);
}

static void update_numlock_mask(void) {
	int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(wm.display);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(wm.display, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}
