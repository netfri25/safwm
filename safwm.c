// TODO:
// [*] make it actually work (lowest priority)
// [*] error handler
// [*] maximize key (win + m)
// [*] fullscreen toggle option (win + f)
// [ ] workspaces
// [ ] Alt+Tab window switching in the current workspace
// [ ] make maximize toggleable (maybe?)
// [ ] middle click on a window to set the input focus only to him ("pinning")
// [ ] support for external bars (simple padding at the top)
// [ ] add an array of window names to not center on creation (e.g. "neovide")

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/X.h>

#include "safwm.h"
#include "config.h"

#define MOD_MASK(mask) (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

// btw I really like to steal from https://github.com/dylanaraps/sowm

static void update_numlock_mask(void);
static unsigned long get_color(const char* color);

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
    wm.display = XOpenDisplay(NULL);
    if (wm.display == NULL) return 1;
    wm.root = DefaultRootWindow(wm.display);
    wm.current = (WindowClient){ .window = None };
    wm.mouse.subwindow = None;
    wm.screen.id = XDefaultScreen(wm.display);

#ifdef SCREEN_WIDTH
    wm.screen.w = SCREEN_WIDTH;
#else
    wm.screen.w = XDisplayWidth(wm.display, wm.screen.id);
#endif

#ifdef SCREEN_HEIGHT
    wm.screen.h = SCREEN_HEIGHT;
#else
    wm.screen.h = XDisplayHeight(wm.display, wm.screen.id);
#endif

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
    WindowClient client = {
        .window = window,
        .fullscreen = false,
    };

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
    if (wm.current.window != None) XSetWindowBorder(wm.display, wm.current.window, get_color(BORDER_NORMAL));
    wm.current = *client;

    // TODO: handle fullscreen
    // TODO: add green border for windows that are "pinned"
    XSetWindowBorder(wm.display, wm.current.window, get_color(BORDER_FOCUS));
    XConfigureWindow(wm.display, wm.current.window, CWBorderWidth, &(XWindowChanges) { .border_width = BORDER_WIDTH });
}

void client_center(WindowClient* client) {
    // TODO: add support for external bars
    client->rect.x = (wm.screen.w - client->rect.w) / 2 - BORDER_WIDTH;
    client->rect.y = (wm.screen.h - client->rect.h) / 2 - BORDER_WIDTH;
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
        .w = wm.screen.w - 2 * BORDER_WIDTH - 2 * WINDOW_GAP,
        .h = wm.screen.h - 2 * BORDER_WIDTH - 2 * WINDOW_GAP,
    };

    client_center(client);
}

void client_fullscreen(WindowClient* client) {
    if (client->fullscreen) return;
    client->fullscreen = true;
    client->prev = client->rect;
    client->rect = (Rect) {
        .w = wm.screen.w,
        .h = wm.screen.h,
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
    // I don't care if there is a click on nothing
    if (event->xbutton.subwindow == None) return;

    Window subwindow = event->xbutton.subwindow;
    WindowClient client = client_from_window(subwindow);
    wm.current = client;
    wm.mouse = event->xbutton;
    XRaiseWindow(wm.display, subwindow);
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
    wm.current = this_client;
    client_focus(&wm.current);
    client_center(&wm.current);
}

void event_mapping(XEvent* event) {
    XMappingEvent* ev = &event->xmapping;
    if (ev->request == MappingKeyboard || ev->request == MappingModifier) {
        XRefreshKeyboardMapping(ev);
        grab_global_input();
    }
}

void event_destroy(XEvent* event) {
    (void) event;
    // TODO: this funciton tells the window manager when a window has been destroyed
    //       so I might not have to implement it
}

void event_enter(XEvent* event) {
    while (XCheckTypedEvent(wm.display, EnterNotify, event));
    while (XCheckTypedWindowEvent(wm.display, wm.mouse.subwindow, MotionNotify, event));

    WindowClient this_window = client_from_window(event->xcrossing.window);
    client_focus(&this_window);
}

void event_motion(XEvent* event) {
    bool is_pressed = wm.mouse.button == Button1 || wm.mouse.button == Button3;
    if (wm.mouse.subwindow == None || !is_pressed || wm.current.fullscreen) return;
    // TODO: find the subwindow, and if it's in fullscreen then don't allow it to be moved

    while (XCheckTypedEvent(wm.display, MotionNotify, event));

    int dx = event->xbutton.x_root - wm.mouse.x_root;
    int dy = event->xbutton.y_root - wm.mouse.y_root;

    if (wm.mouse.button == Button1) {
        int x = wm.current.rect.x + dx;
        int y = wm.current.rect.y + dy;
        XMoveWindow(wm.display, wm.mouse.subwindow, x, y);
    }

    if (wm.mouse.button == Button3) {
        int w = wm.current.rect.w + dx;
        int h = wm.current.rect.h + dy;
        XResizeWindow(wm.display, wm.mouse.subwindow, w, h);
    }
}

int error_handler(Display* display, XErrorEvent* err) {
    char err_msg[1024];
    XGetErrorText(display, err->error_code, err_msg, sizeof(err_msg));
    printf("X-Error(req: %u, code: %u, res_id: %lu)\n", err->request_code, err->error_code, err->resourceid);
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
    msg.xclient.window = wm.current.window;
    msg.xclient.format = 32;
    msg.xclient.data.l[0] = XInternAtom(wm.display, "WM_DELETE_WINDOW", false);
    XSendEvent(wm.display, wm.current.window, false, 0, &msg);
}

void center_win(Arg arg) {
    (void) arg;
    client_center(&wm.current);
}

void maximize_win(Arg arg) {
    (void) arg;
    client_maximize(&wm.current);
}

void fullscreen_win(Arg arg) {
    (void) arg;

    if (wm.current.fullscreen) {
        client_unfullscreen(&wm.current);
    } else {
        client_fullscreen(&wm.current);
    }
}

static void update_numlock_mask(void)
{
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

static unsigned long get_color(const char* color) {
    Colormap colormap = DefaultColormap(wm.display, wm.screen.id);
    XColor c;
    return XAllocNamedColor(wm.display, colormap, color, &c, &c) ? c.pixel : 0;
}
