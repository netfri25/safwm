// TODO:
// [*] make it actually work (lowest priority)
// [*] error handler
// [*] maximize key (win + m)
// [*] fullscreen toggle option (win + f)
// [*] workspaces
// [*] add all of the keybindings from my old wm
// [*] Alt+Tab window switching in the current workspace
// [*] simple pseudo-tiling support
// [*] support for toggling all the windows in the current desktop (hiding)
// [ ] "undo" for the pseudo-tiling
// [ ] make maximize toggleable (maybe?)
// [ ] middle click on a window to set the input focus only to him ("pinning")
// [ ] support for external bars (simple padding at the top)
// [ ] add an array of window names to not center on creation (e.g. "neovide")

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

static void (*events[LASTEvent])(XEvent* e) = {
    [ButtonPress]      = event_button_press,
    [ButtonRelease]    = event_button_release,
    [ConfigureRequest] = event_configure,
    [KeyPress]         = event_key_press,
    [MapRequest]       = event_map_request,
    [MappingNotify]    = event_mapping,
    [DestroyNotify]    = event_destroy,
    [EnterNotify]      = event_enter,
    [MotionNotify]     = event_motion,
    [ResizeRequest]    = event_resize,
};

int main(void) {
    wm.display = XOpenDisplay(NULL);
    if (wm.display == NULL) return 1;
    wm.root = DefaultRootWindow(wm.display);
    wm.screen = DefaultScreen(wm.display);
    wm.mouse.subwindow = None;

    Cursor cursor = XCreateFontCursor(wm.display, 68);
    XDefineCursor(wm.display, wm.root, cursor);
    XSetErrorHandler(error_handler);

    XSelectInput(wm.display, wm.root, SubstructureRedirectMask|SubstructureNotifyMask);
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
    ssize_t index = ws_find(ws, client->window);
    if (index == -1) return;
    ws->focused_index = index;

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
    ssize_t i = ws_find(ws, None);

    if (i == -1) {
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

WindowClient* ws_get(Workspace* ws, Window window) {
    ssize_t index = ws_find(ws, window);
    return index >= 0 ? ws->client + index : NULL;
}

ssize_t ws_find(const Workspace* ws, Window window) {
    for (size_t i = 0; i < WORKSPACE_CLIENTS_CAPACITY; i++) {
        const WindowClient* client = ws->client + i;
        if (window == client->window)
            return i;
    }

    return -1;
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

void ws_insert(Workspace* ws, WindowClient client) {
    size_t index = ws_next_empty(ws);
    ws_set_client(ws, index, client);
}

void ws_move_client(size_t from, size_t to, Window window) {
    assert(from < WORKSPACE_CLIENTS_CAPACITY && "out of bounds");
    assert(to   < WORKSPACE_CLIENTS_CAPACITY && "out of bounds");
    Workspace* from_ws = wm.workspace + from;
    Workspace* to_ws   = wm.workspace + to;

    WindowClient* client = ws_get(from_ws, window);
    assert(client && "client should always be in the `from` workspace");
    ws_insert(to_ws, *client);
    client->window = None; // set the client as removed
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

WindowClient* wm_get_client(Window window) {
    for (size_t i = 0; i < WORKSPACE_COUNT; i++) {
        Workspace* ws = wm.workspace + i;
        WindowClient* client = ws_get(ws, window);
        if (client) return client;
    }

    return NULL;
}

void grab_global_input(void) {
    grab_window_input(wm.root);
}

void grab_window_input(Window window) {
    XUngrabKey(wm.display, AnyKey, AnyModifier, window);
    XUngrabButton(wm.display, AnyButton, AnyModifier, window);

    unsigned int mask = ButtonMotionMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
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

bool is_out(const WindowClient* client) {
    int x = client->rect.x;
    int y = client->rect.y;
    unsigned w = client->rect.w;
    unsigned h = client->rect.h;
    return x - BORDER_WIDTH <= 0
        || y - BORDER_WIDTH <= 0
        || x + w + 2 * BORDER_WIDTH >= SCREEN_WIDTH
        || y + h + 2 * BORDER_WIDTH >= SCREEN_HEIGHT;
}

bool is_really_big(const WindowClient* client) {
    unsigned w = client->rect.w;
    unsigned h = client->rect.h;
    return w > SCREEN_WIDTH || h > SCREEN_HEIGHT;
}

void event_button_press(XEvent* event) {
    Window window = event->xbutton.subwindow;

    // I don't care if there is a click on nothing
    if (window == None) return;
    Workspace* ws = wm_workspace();
    if (ws->hidden) return;

    ssize_t client_index = ws_find(ws, window);
    if (client_index < 0) return;

    wm.mouse = event->xbutton;
    wm_workspace()->focused_index = client_index;
    XRaiseWindow(wm.display, window);
}

void event_button_release(XEvent* event) {
    (void) event;
    wm.mouse.subwindow = None;
}

void event_configure(XEvent* event) {
    XConfigureRequestEvent *ev = &event->xconfigurerequest;

    WindowClient* client = wm_get_client(ev->window);
    if (!client) return;

    if (is_really_big(client)) {
        client_maximize(client);
    } else if (is_out(client)) {
        client_center(client);
    } else {
        client->rect.x = ev->x - BORDER_WIDTH,
        client->rect.y = ev->y - BORDER_WIDTH,
        client->rect.w = ev->width;
        client->rect.h = ev->height;
        client_update_rect(client);
    }

    XConfigureWindow(wm.display, ev->window, ev->value_mask, &(XWindowChanges) {
        .x          = client->rect.x,
        .y          = client->rect.y,
        .width      = client->rect.w,
        .height     = client->rect.h,
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
    Workspace* ws = wm_workspace();
    XSelectInput(wm.display, window, StructureNotifyMask|EnterWindowMask);
    if (!ws->hidden) XMapWindow(wm.display, window);

    // remove if found in a differnet workspace
    WindowClient* found_client = wm_get_client(window);
    if (found_client) found_client->window = None;

    // insert to workspace
    WindowClient this_client = client_from_window(window);
    size_t client_index = ws_next_empty(ws);
    ws_set_client(ws, client_index, this_client);
    WindowClient* client = ws->client + client_index;
    client_focus(client);
    client_maximize(client);
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
        ssize_t client_index = ws_find(workspace, event->xdestroywindow.window);
        if (client_index >= 0) {
            ws_remove_client(workspace, client_index);
            break;
        }
    }
}

void event_enter(XEvent* event) {
    while (XCheckTypedEvent(wm.display, EnterNotify, event));
    while (XCheckTypedWindowEvent(wm.display, wm.mouse.subwindow, MotionNotify, event));

    Workspace* ws = wm_workspace();
    WindowClient* client = ws_get(ws, event->xcrossing.window);
    if (client == NULL) return;
    client_focus(client);
}

void event_motion(XEvent* event) {
    Workspace* ws = wm_workspace();
    WindowClient* client = ws_get(ws, wm.mouse.subwindow);

    while (XCheckTypedEvent(wm.display, MotionNotify, event));
    while (XCheckTypedWindowEvent(wm.display, client->window, MotionNotify, event));

    bool is_pressed = wm.mouse.button == Button1 || wm.mouse.button == Button3;
    if (ws->hidden || client == NULL || client->window == None || client->fullscreen || !is_pressed) return;

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

void event_resize(XEvent* event) {
    XResizeRequestEvent req = event->xresizerequest;
    XResizeWindow(req.display, req.window, req.width, req.height);

    WindowClient* client = wm_get_client(req.window);
    if (client) {
        client->rect.w = req.width;
        client->rect.h = req.height;
        client_update_rect(client);
    }
}

int error_handler(Display* display, XErrorEvent* err) {
    char err_msg[1024];
    XGetErrorText(display, err->error_code, err_msg, sizeof(err_msg));
    fprintf(stderr, "X-Error(req: %u, code: %u, res_id: %lu): %.*s\n",
            err->request_code,
            err->error_code,
            err->resourceid,
            (int) sizeof(err_msg),
            err_msg);
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
    if (wm_workspace()->hidden) return;
    (void) arg;
    XEvent msg = {0};
    msg.xclient.type = ClientMessage;
    msg.xclient.message_type = XInternAtom(wm.display, "WM_PROTOCOLS", false);
    msg.xclient.window = wm_client()->window;
    msg.xclient.format = 32;
    msg.xclient.data.l[0] = XInternAtom(wm.display, "WM_DELETE_WINDOW", false);
    msg.xclient.data.l[1] = CurrentTime;

    if (!XSendEvent(wm.display, wm_client()->window, false, NoEventMask, &msg)) {
        XGrabServer(wm.display);
        XSetCloseDownMode(wm.display, DestroyAll);
        XKillClient(wm.display, wm_client()->window);
        XSync(wm.display, False);
        XUngrabServer(wm.display);
    }

    // WARN: pointer arithmetics
    size_t client_index = wm_client() - wm_workspace()->client;
    ws_remove_client(wm_workspace(), client_index);
}

void center_win(Arg arg) {
    (void) arg;
    if (wm_workspace()->hidden) return;
    client_center(wm_client());
}

void maximize_win(Arg arg) {
    (void) arg;
    if (wm_workspace()->hidden) return;
    client_maximize(wm_client());
}

void fullscreen_win(Arg arg) {
    (void) arg;

    if (wm_workspace()->hidden) return;
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
    if (!new_ws->hidden) ws_focus(new_ws);
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
    Workspace* ws = wm_workspace();
    ws_unfocus(ws);
    ws_move_client(from, to, window);
    if (!ws->hidden) ws_focus(ws);
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
    if (wm_workspace()->hidden) return;
    WindowClient* client = ws_next_window(wm_workspace());
    if (client == NULL) return;
    client_focus(client);
    XRaiseWindow(wm.display, client->window);
}

void win_prev(Arg arg) {
    (void) arg;
    if (wm_workspace()->hidden) return;
    WindowClient* client = ws_prev_window(wm_workspace());
    if (client == NULL) return;
    client_focus(client);
    XRaiseWindow(wm.display, client->window);
}

// coolest fucking code I've ever written
void win_slice(Arg arg) {
    WindowClient* client = wm_client();
    if (wm_workspace()->hidden || !client || client->window == None || client->fullscreen) return;

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

void ws_toggle_visibility(Arg arg) {
    (void) arg;
    Workspace* ws = wm_workspace();
    if (ws->hidden) {
        ws_focus(ws); // unhide
    } else {
        ws_unfocus(ws); // hide
    }

    ws->hidden = !ws->hidden;
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
