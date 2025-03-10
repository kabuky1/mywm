#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <linux/limits.h>

/* Function declarations */
void cleanup(void);

/* Type definitions */
typedef struct Client {
    Window win;
    int x, y, w, h;
    struct Client *next;
    int isfloating;
    int workspace;
    int isfullscreen;
} Client;

/* Forward declarations */
static void maprequest(XEvent *e);
static void keypress(XEvent *e);
static void destroynotify(XEvent *e);
static void configurerequest(XEvent *e);
static void focusnext(const char **arg);
static void focusprev(const char **arg);
static void spawn(const char **arg);
static void setmfact(const char **arg);
static void togglefloating(const char **arg);
static void quit(const char **arg);
static void focus(Client *c);
static void arrange(void);
static int xerror(Display *dpy, XErrorEvent *ee);
static void scan(void);
static void swapmaster(const char **arg);
static void enternotify(XEvent *e);
static void sendtoworkspace(const char **arg);
static void switchworkspace(const char **arg);
static void togglefullscreen(const char **arg);
static void killclient(const char **arg);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void motionnotify(XEvent *e);
static void clearworkspace(const char **arg);
static void showworkspace(const char **arg);
static void reload(const char **arg);

#include "config.h"

/* Global log file */
static FILE *logfile = NULL;

/* Log message with timestamp */
static void
wm_log(const char *fmt, ...) {
    if (!logfile)
        return;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    fprintf(logfile, "[%02d:%02d:%02d] ", tm->tm_hour, tm->tm_min, tm->tm_sec);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(logfile, fmt, ap);
    va_end(ap);

    fflush(logfile);
}

/* Error handler */
static int
xerror(Display *dpy __attribute__((unused)), XErrorEvent *ee) {
    if (ee->error_code == BadAccess &&
        ee->request_code == X_ChangeWindowAttributes) {
        wm_log("Error: Another window manager is already running\n");
        exit(1);
    }
    wm_log("X error: request code=%d, error code=%d\n",
            ee->request_code, ee->error_code);
    return 0;
}

/* Window Manager Global State */
static Display *dpy;                // X11 display connection
static Window root;                 // Root window handle
static int screen;                 // Current screen number
static XWindowAttributes attr;      // Root window attributes
static int running = 1;            // Main loop control flag
static Client *clients = NULL;      // Linked list of managed windows
static Client *sel = NULL;         // Currently selected window
static float mfact = MASTER_SIZE;  // Master area size ratio (0.1-0.9)
static int current_workspace = 1;  /* Current workspace (1-based index) */
// Add these new globals
static Atom clipboard;
static Atom primary_selection;
static int dragx, dragy;           // Initial cursor position
static Client *dragclient = NULL;  // Window being dragged
static int drag_started = 0;       // Track if drag has started

/* Event handler mapping table */
static void (*handler[LASTEvent]) (XEvent *) = {
    [MapRequest] = maprequest,      // Handle new window creation
    [KeyPress] = keypress,          // Handle keyboard input
    [DestroyNotify] = destroynotify, // Handle window destruction
    [ConfigureRequest] = configurerequest, // Handle window resize/move requests
    [EnterNotify] = enternotify,    // Handle mouse enter events
    [ButtonPress] = buttonpress,
    [ButtonRelease] = buttonrelease,
    [MotionNotify] = motionnotify,
};

/* Helper function to count windows in a workspace */
static int
count_windows_in_workspace(int workspace)
{
    Client *c;
    int count = 0;
    for (c = clients; c; c = c->next)
        if (c->workspace == workspace)
            count++;
    return count;
}

/* Add this helper function after the other static declarations */
static Client *
find_fullscreen(void)
{
    Client *c;
    for (c = clients; c; c = c->next)
        if (c->isfullscreen && c->workspace == current_workspace)
            return c;
    return NULL;
}

/* Add these validation functions near the top */
static int
validate_window_size(int w, int h) {
    return (w >= MIN_WIN_SIZE && h >= MIN_WIN_SIZE &&
            w <= attr.width && h <= attr.height);
}

static int
validate_window_position(int x, int y) {
    return (x >= -BORDER_WIDTH && y >= -BORDER_WIDTH &&
            x <= attr.width && y <= attr.height);
}

/* Add these validation functions after the other validation functions */
static int
validate_spawn_args(const char **arg) {
    if (!arg || !arg[0]) {
        wm_log("Invalid spawn arguments: NULL\n");
        return 0;
    }

    /* Check for command injection attempts */
    for (int i = 0; arg[i]; i++) {
        if (strchr(arg[i], ';') || strchr(arg[i], '|') ||
            strchr(arg[i], '&') || strchr(arg[i], '`')) {
            wm_log("Rejected spawn command with invalid characters: %s\n", arg[i]);
            return 0;
        }
    }

    /* Validate executable path */
    char *path = getenv("PATH");
    if (!path) {
        wm_log("Cannot get PATH environment\n");
        return 0;
    }

    char *path_copy = strdup(path);
    if (!path_copy) {
        wm_log("Failed to allocate memory for path validation\n");
        return 0;
    }

    char *dir = strtok(path_copy, ":");
    char full_path[PATH_MAX];
    int found = 0;

    while (dir) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, arg[0]);
        if (access(full_path, X_OK) == 0) {
            found = 1;
            break;
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);

    if (!found) {
        wm_log("Command not found in PATH: %s\n", arg[0]);
        return 0;
    }

    return 1;
}

/* Add this helper function before maprequest */
static int
is_child_of_fullscreen(Window win) {
    Window root, parent, *children;
    unsigned int nchildren;
    Client *fs = find_fullscreen();

    if (!fs) return 0;

    if (!XQueryTree(dpy, win, &root, &parent, &children, &nchildren))
        return 0;

    if (children)
        XFree(children);

    return (parent == fs->win);
}

void
maprequest(XEvent *e)
{
    if (!e) return;
    XMapRequestEvent *ev = &e->xmaprequest;
    Client *c;
    XWindowAttributes wa;

    /* Validate the window exists and can be accessed */
    if (!XGetWindowAttributes(dpy, ev->window, &wa)) {
        wm_log("Invalid window in maprequest\n");
        return;
    }

    /* Validate window size */
    if (!validate_window_size(wa.width, wa.height)) {
        wm_log("Invalid window size: %dx%d\n", wa.width, wa.height);
        return;
    }

    /* Validate window position */
    if (!validate_window_position(wa.x, wa.y)) {
        wm_log("Invalid window position: %d,%d\n", wa.x, wa.y);
        return;
    }

    /* Check if we've reached the maximum for current workspace */
    if (count_windows_in_workspace(current_workspace) >= MAX_WINDOWS) {
        wm_log("Maximum number of windows (%d) reached in workspace %d\n",
                MAX_WINDOWS, current_workspace);
        return;
    }

    /* Add the window to our list */
    c = malloc(sizeof(Client));
    if (!c) {
        wm_log("Fatal: failed to allocate memory for new client\n");
        return;
    }
    c->win = ev->window;
    c->next = clients;
    c->isfloating = 0;
    c->isfullscreen = 0;
    c->workspace = current_workspace;
    clients = c;
    sel = c;

    /* Set up window isolation */
    XSetWindowAttributes swa;  // Changed variable name to swa
    swa.event_mask = EnterWindowMask | KeyPressMask;
    swa.override_redirect = True;  // Prevent direct window communication
    swa.border_pixel = INACTIVE_BORDER;
    XChangeWindowAttributes(dpy, ev->window,
                          CWEventMask | CWOverrideRedirect | CWBorderPixel,
                          &swa);

    /* Set initial border width */
    XSetWindowBorderWidth(dpy, ev->window, BORDER_WIDTH);

    /* Isolate window from others except via clipboard */
    XChangeProperty(dpy, ev->window,
                   XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False),
                   XA_ATOM, 32, PropModeReplace,
                   (unsigned char *) &clipboard, 1);

    /* Add mouse button grabs */
    XGrabButton(dpy, Button1, MODKEY, ev->window, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None);

    XMapWindow(dpy, ev->window);

    /* Handle child windows of fullscreen windows */
    Client *fs = find_fullscreen();
    if (fs) {
        if (is_child_of_fullscreen(ev->window)) {
            /* Child window of fullscreen window - keep on top */
            XRaiseWindow(dpy, ev->window);
        } else {
            /* Not a child - keep fullscreen window on top */
            XRaiseWindow(dpy, fs->win);
        }
    }

    arrange();
    focus(sel);
}

void
keypress(XEvent *e)
{
    XKeyEvent *ev = &e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(dpy, ev->keycode, 0, 0);
    long unsigned int i;
    const char **args;

    for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        if (keysym == keys[i].keysym && keys[i].mod == ev->state && keys[i].func) {
            args = keys[i].arg;  // Assign the array of strings directly
            keys[i].func(args);
        }
    }
}

void
destroynotify(XEvent *e)
{
    Client *c, *prev = NULL;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    for (c = clients; c; prev = c, c = c->next) {
        if (c->win == ev->window) {
            if (prev)
                prev->next = c->next;
            else
                clients = c->next;
            if (sel == c)
                sel = clients;
            free(c);
            break;
        }
    }
    arrange();
}

void
configurerequest(XEvent *e)
{
    if (!e) return;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    /* Validate requested dimensions */
    if (!validate_window_size(ev->width, ev->height)) {
        wm_log("Invalid configure request size: %dx%d\n", ev->width, ev->height);
        return;
    }

    /* Validate requested position */
    if (!validate_window_position(ev->x, ev->y)) {
        wm_log("Invalid configure request position: %d,%d\n", ev->x, ev->y);
        return;
    }

    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = BORDER_WIDTH;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
}

void
focus(Client *c)
{
    Client *i;
    int visible = 0;
    if (!c)
        return;

    /* Count visible windows in workspace */
    for (i = clients; i; i = i->next)
        if (i->workspace == current_workspace)
            visible++;

    /* Set borders only if more than one window and not fullscreen */
    if (visible > 1 && !c->isfullscreen) {
        /* Set all windows to inactive border */
        for (i = clients; i; i = i->next) {
            if (i->workspace == current_workspace) {
                XSetWindowBorderWidth(dpy, i->win, BORDER_WIDTH);
                XSetWindowBorder(dpy, i->win, INACTIVE_BORDER);
            }
        }
        /* Set active window border */
        XSetWindowBorderWidth(dpy, c->win, BORDER_WIDTH);
        XSetWindowBorder(dpy, c->win, ACTIVE_BORDER);
    } else {
        /* No borders needed */
        XSetWindowBorderWidth(dpy, c->win, 0);
    }

    sel = c;
    XSetInputFocus(dpy, c->win, RevertToParent, CurrentTime);
    XRaiseWindow(dpy, c->win);
}

void
arrange(void)
{
    Client *c, *master = NULL;
    int n = 0, visible = 0;

    /* Handle fullscreen windows first */
    Client *fs = find_fullscreen();
    if (fs) {
        XSetWindowBorderWidth(dpy, fs->win, 0);
        XMoveResizeWindow(dpy, fs->win, 0, 0, attr.width, attr.height);
        XRaiseWindow(dpy, fs->win);
        return;
    }

    /* Count non-floating windows in current workspace and visible windows */
    for (c = clients; c; c = c->next) {
        if (!c->isfloating && !c->isfullscreen && c->workspace == current_workspace)
            n++;
        if (c->workspace == current_workspace && !c->isfullscreen)
            visible++;
    }

    /* Position floating windows first */
    for (c = clients; c; c = c->next) {
        if (c->isfloating && c->workspace == current_workspace) {
            XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
            continue;
        }
    }

    /* If only one visible window in workspace, make it fullscreen without borders */
    if (visible == 1) {
        for (c = clients; c; c = c->next) {
            if (c->workspace == current_workspace) {
                XSetWindowBorderWidth(dpy, c->win, 0);
                XMoveResizeWindow(dpy, c->win, 0, 0, attr.width, attr.height);
                break;
            }
        }
        return;
    }

    /* Rest of tiling logic */
    if (n == 0)
        return;

    /* If only one non-floating window, make it fullscreen */
    if (n == 1) {
        for (c = clients; c; c = c->next) {
            if (!c->isfloating) {
                XMoveResizeWindow(dpy, c->win, 0, 0, attr.width, attr.height);
                break;
            }
        }
        return;
    }

    /* Find the first non-floating window as master */
    for (c = clients; c; c = c->next) {
        if (!c->isfloating) {
            master = c;
            break;
        }
    }

    /* Master */
    if (master) {
        int master_width = (attr.width * mfact) - (GAP_WIDTH * 1.5) - (BORDER_WIDTH * 2);
        XMoveResizeWindow(dpy, master->win,
                        GAP_WIDTH,                    // x position
                        GAP_WIDTH,                    // y position
                        master_width,                 // width
                        attr.height - (GAP_WIDTH * 2) - (BORDER_WIDTH * 2)); // height
        XRaiseWindow(dpy, master->win);
    }

    /* Stack */
    if (n > 1) {
        int stack_width = (attr.width * (1 - mfact)) - (GAP_WIDTH * 1.5) - (BORDER_WIDTH * 2);
        int x = (attr.width * mfact) + (GAP_WIDTH * 0.5);
        int i = 0;
        for (c = clients; c; c = c->next) {
            if (!c->isfloating && c != master) {
                int height = (attr.height / (n - 1)) - (GAP_WIDTH * 2) - (BORDER_WIDTH * 2);
                XMoveResizeWindow(dpy, c->win,
                                x,                                         // x position
                                (i * (height + GAP_WIDTH)) + GAP_WIDTH,   // y position with gap
                                stack_width,                              // width
                                height);                                  // height
                XRaiseWindow(dpy, c->win);
                i++;
            }
        }
    }
}

void
focusnext(const char **arg __attribute__((unused)))
{
    if (!sel || !sel->next)
        return;
    focus(sel->next);
}

void
focusprev(const char **arg __attribute__((unused)))
{
    Client *c;
    if (!sel || !clients)
        return;
    for (c = clients; c->next && c->next != sel; c = c->next);
    focus(c);
}

void
spawn(const char **arg)
{
    if (!validate_spawn_args(arg))
        return;

    /* Fork and execute command */
    pid_t pid = fork();
    if (pid == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));

        /* Set clean environment variables */
        static char path[] = "PATH=/usr/local/bin:/usr/bin:/bin";
        static char home[] = "HOME=/home/kabuky";
        putenv(path);
        putenv(home);

        setsid();
        execvp(((char **)arg)[0], (char **)arg);
        wm_log("Failed to execute: %s\n", arg[0]);
        exit(1);
    } else if (pid < 0) {
        wm_log("Fork failed for spawn command: %s\n", arg[0]);
    }
}

void
setmfact(const char **arg)
{
    float f;
    if (!arg || !arg[0])
        return;
    f = mfact + atof(arg[0]);
    if (f < 0.1 || f > 0.9)
        return;
    mfact = f;
    arrange();
}

void
togglefloating(const char **arg __attribute__((unused)))
{
    if (!sel)
        return;
    if (!sel->isfloating) {
        XWindowAttributes wa;
        XGetWindowAttributes(dpy, sel->win, &wa);
        sel->x = wa.x;
        sel->y = wa.y;
        sel->w = wa.width;
        sel->h = wa.height;
    }
    sel->isfloating = !sel->isfloating;
    arrange();
}

void
togglefullscreen(const char **arg __attribute__((unused)))
{
    if (!sel)
        return;

    sel->isfullscreen = !sel->isfullscreen;

    if (sel->isfullscreen) {
        /* Save window dimensions before going fullscreen */
        XWindowAttributes wa;
        XGetWindowAttributes(dpy, sel->win, &wa);
        sel->x = wa.x;
        sel->y = wa.y;
        sel->w = wa.width;
        sel->h = wa.height;

        /* Remove borders and go full screen */
        XSetWindowBorderWidth(dpy, sel->win, 0);
        XMoveResizeWindow(dpy, sel->win, 0, 0, attr.width, attr.height);
        XRaiseWindow(dpy, sel->win);
    } else {
        /* Restore borders and previous size */
        XSetWindowBorderWidth(dpy, sel->win, BORDER_WIDTH);
        XSetWindowBorder(dpy, sel->win, ACTIVE_BORDER);
        XMoveResizeWindow(dpy, sel->win, sel->x, sel->y, sel->w, sel->h);
    }

    arrange();
}

void
killclient(const char **arg __attribute__((unused)))
{
    if (!sel)
        return;
    XKillClient(dpy, sel->win);
}

void
cleanup(void) {
    Client *c, *tmp;

    // Clean up all managed windows
    c = clients;
    while (c) {
        tmp = c->next;
        if (!running) // Only unmap windows if we're actually quitting
            XUnmapWindow(dpy, c->win);
        free(c);
        c = tmp;
    }

    // Remove all keyboard shortcuts
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
}

void
quit(const char **arg __attribute__((unused))) {
    cleanup();
    running = 0;
}

void
swapmaster(const char **arg __attribute__((unused)))
{
    Client *c;

    if (!clients || !clients->next)
        return;

    c = clients;
    clients = clients->next;
    c->next = clients->next;
    clients->next = c;

    if (sel == c)
        sel = clients;
    else if (sel == clients)
        sel = c;

    arrange();
}

void
enternotify(XEvent *e)
{
    if (!e) return;
    XCrossingEvent *ev = &e->xcrossing;
    Client *c;

    /* Validate crossing event */
    if (ev->mode != NotifyNormal || ev->detail == NotifyInferior) {
        return;
    }

    for (c = clients; c; c = c->next) {
        if (c->win == ev->window) {
            focus(c);
            break;
        }
    }
}

void
scan(void)
{
    Window root_return, parent_return, *children;
    unsigned int nchildren;
    Client *c;

    if (!XQueryTree(dpy, root, &root_return, &parent_return, &children, &nchildren))
        return;

    for (unsigned int i = 0; i < nchildren; i++) {
        if (children[i] != root) {
            c = malloc(sizeof(Client));
            if (!c) {
                wm_log("Fatal: failed to allocate memory for client during scan\n");
                if (children) XFree(children);
                return;
            }
            c->win = children[i];
            c->next = clients;
            c->isfloating = 0;
            c->isfullscreen = 0;
            clients = c;
            XMapWindow(dpy, children[i]);
        }
    }
    if (children)
        XFree(children);
    arrange();
}

/* Workspace Management */
void
sendtoworkspace(const char **arg)
{
    if (!sel || !arg || !arg[0])
        return;

    int workspace = atoi(arg[0]);
    if (workspace < 1 || workspace > 9)
        return;

    /* Check if target workspace has room */
    if (count_windows_in_workspace(workspace) >= MAX_WINDOWS) {
        wm_log("Cannot move window: workspace %d is full (max %d windows)\n",
                workspace, MAX_WINDOWS);
        return;
    }

    sel->workspace = workspace;
    if (workspace != current_workspace)
        XUnmapWindow(dpy, sel->win);
    focus(NULL);
    arrange();
}

void
switchworkspace(const char **arg)
{
    if (!arg || !arg[0])
        return;

    int workspace = atoi(arg[0]);
    if (workspace < 1 || workspace > 9 || workspace == current_workspace)
        return;

    /* Hide current workspace windows */
    for (Client *c = clients; c; c = c->next) {
        if (c->workspace == current_workspace)
            XUnmapWindow(dpy, c->win);
        else if (c->workspace == workspace)
            XMapWindow(dpy, c->win);
    }

    current_workspace = workspace;
    focus(NULL);
    arrange();
}

void
buttonpress(XEvent *e)
{
    if (!e) return;
    XButtonEvent *ev = &e->xbutton;
    Client *c;

    /* Validate button press coordinates */
    if (!validate_window_position(ev->x_root, ev->y_root)) {
        wm_log("Invalid button press coordinates: %d,%d\n", ev->x_root, ev->y_root);
        return;
    }

    for (c = clients; c; c = c->next) {
        if (c->win == ev->window) {
            if (ev->button == Button1 && ev->state & MODKEY) {
                dragx = ev->x_root;
                dragy = ev->y_root;
                dragclient = c;
                drag_started = 1;
                XSetWindowBorderWidth(dpy, c->win, BORDER_WIDTH * 2);
                focus(c);
                XGrabPointer(dpy, root, True,
                            PointerMotionMask | ButtonReleaseMask,
                            GrabModeAsync, GrabModeAsync,
                            root, None, CurrentTime);
            }
            return;
        }
    }
}

void
buttonrelease(XEvent *e __attribute__((unused)))
{
    Client *c;
    Window dummy;
    int rx, ry, x, y;
    unsigned int mask;

    if (!drag_started)
        return;

    /* Find window under pointer */
    if (XQueryPointer(dpy, root, &dummy, &dummy, &rx, &ry, &x, &y, &mask)) {
        Window win_under;
        int win_x, win_y;
        XTranslateCoordinates(dpy, root, root, rx, ry, &win_x, &win_y, &win_under);

        /* Find client under pointer */
        for (c = clients; c; c = c->next) {
            if (c->win == win_under && c != dragclient &&
                c->workspace == current_workspace) {
                /* Swap windows in linked list */
                Client *prev_drag = NULL, *prev_c = NULL;
                Client *t;

                /* Find nodes */
                for (t = clients; t && t != dragclient; t = t->next)
                    prev_drag = t;
                for (t = clients; t && t != c; t = t->next)
                    prev_c = t;

                /* Swap positions */
                if (prev_drag)
                    prev_drag->next = c;
                else
                    clients = c;

                if (prev_c)
                    prev_c->next = dragclient;
                else
                    clients = dragclient;

                t = dragclient->next;
                dragclient->next = c->next;
                c->next = t;

                break;
            }
        }
    }

    XUngrabPointer(dpy, CurrentTime);
    XSetWindowBorderWidth(dpy, dragclient->win, BORDER_WIDTH);
    dragclient = NULL;
    drag_started = 0;
    arrange();
}

void
motionnotify(XEvent *e)
{
    if (!e || !dragclient || !drag_started) return;
    XMotionEvent *ev = &e->xmotion;

    /* Validate motion coordinates */
    if (!validate_window_position(ev->x_root, ev->y_root)) {
        wm_log("Invalid motion coordinates: %d,%d\n", ev->x_root, ev->y_root);
        return;
    }

    int dx = ev->x_root - dragx;
    int dy = ev->y_root - dragy;

    /* Validate new window position */
    int new_x = dragclient->x + dx;
    int new_y = dragclient->y + dy;

    if (validate_window_position(new_x, new_y)) {
        XMoveWindow(dpy, dragclient->win, new_x, new_y);
    }
}

void
clearworkspace(const char **arg)
{
    if (!arg || !arg[0])
        return;

    int workspace = atoi(arg[0]);
    if (workspace < 1 || workspace > 9)
        return;

    Client *c, *next;
    for (c = clients; c; c = next) {
        next = c->next;
        if (c->workspace == workspace) {
            XKillClient(dpy, c->win);
        }
    }
}

void
showworkspace(const char **arg __attribute__((unused)))
{
    char buf[32];
    Window w;
    XSetWindowAttributes wa;
    GC gc;
    XGCValues gcv;
    int x, y, width = 200, height = 50;
    XFontStruct *font;

    snprintf(buf, sizeof(buf), "Workspace: %d", current_workspace);

    /* Create centered notification window */
    wa.override_redirect = True;
    wa.background_pixel = 0x282a36;
    wa.border_pixel = ACTIVE_BORDER;

    x = (attr.width - width) / 2;
    y = (attr.height - height) / 2;

    w = XCreateWindow(dpy, root, x, y, width, height, 2,
                     DefaultDepth(dpy, screen), CopyFromParent,
                     DefaultVisual(dpy, screen),
                     CWOverrideRedirect | CWBackPixel | CWBorderPixel, &wa);

    /* Load a font */
    font = XLoadQueryFont(dpy, "fixed");
    if (!font) font = XLoadQueryFont(dpy, "9x15");

    /* Create GC for text */
    gcv.foreground = 0xf8f8f2;
    gcv.font = font->fid;
    gc = XCreateGC(dpy, w, GCForeground | GCFont, &gcv);

    /* Calculate text position */
    int text_width = XTextWidth(font, buf, strlen(buf));
    int text_x = (width - text_width) / 2;
    int text_y = height / 2 + font->ascent / 2;

    XMapRaised(dpy, w);
    XDrawString(dpy, w, gc, text_x, text_y, buf, strlen(buf));
    XFlush(dpy);

    /* Auto-destroy after 1 second */
    usleep(1000000);

    /* Cleanup */
    XFreeGC(dpy, gc);
    XFreeFont(dpy, font);
    XDestroyWindow(dpy, w);
    XFlush(dpy);
}

void
reload(const char **arg __attribute__((unused)))
{
    char path[PATH_MAX];
    ssize_t len;

    /* Get path to current executable */
    len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len < 0) {
        wm_log("Failed to get executable path\n");
        return;
    }
    path[len] = '\0';

    /* Ungrab all inputs */
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    XUngrabButton(dpy, AnyButton, AnyModifier, root);
    XSync(dpy, False);

    /* Keep track of existing windows */
    Window *wins = NULL;
    int num_wins = 0;
    for (Client *c = clients; c; c = c->next) {
        num_wins++;
        wins = realloc(wins, sizeof(Window) * num_wins);
        wins[num_wins - 1] = c->win;
        XUnmapWindow(dpy, c->win);
    }

    /* Free client list */
    while (clients) {
        Client *tmp = clients->next;
        free(clients);
        clients = tmp;
    }
    clients = NULL;
    sel = NULL;

    /* Reexec self */
    execl(path, path, NULL);
    
    /* If execl fails, restore windows */
    for (int i = 0; i < num_wins; i++) {
        XMapWindow(dpy, wins[i]);
    }
    free(wins);
    
    /* Regrab keys */
    for (long unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].keysym),
                keys[i].mod, root, True,
                GrabModeAsync, GrabModeAsync);
    }
    XSync(dpy, True);
    wm_log("Failed to reload window manager\n");
}

int
main(void)
{
    XEvent ev;

    /* Set up logging */
    char logpath[256];
    snprintf(logpath, sizeof(logpath), "%s/.local/share/wm", getenv("HOME"));
    mkdir(logpath, 0755);  // Create directory if it doesn't exist

    char logfile_path[512];
    snprintf(logfile_path, sizeof(logfile_path), "%s/wm.log", logpath);
    logfile = fopen(logfile_path, "a");
    if (!logfile) {
        fprintf(stderr, "Cannot open log file: %s\n", strerror(errno));
        exit(1);
    }

    wm_log("Starting window manager\n");

    if (!(dpy = XOpenDisplay(NULL))) {
        wm_log("Cannot open display\n");
        exit(1);
    }

    /* Initialize clipboard atoms */
    clipboard = XInternAtom(dpy, "CLIPBOARD", False);
    primary_selection = XInternAtom(dpy, "PRIMARY", False);

    wm_log("Display opened successfully\n");

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    XGetWindowAttributes(dpy, root, &attr);

    /* Set up key bindings */
    for (long unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
        XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].keysym),
                keys[i].mod, root, True,
                GrabModeAsync, GrabModeAsync);

    /* Select events */
    XSetErrorHandler(xerror);
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask |
                           ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                           EnterWindowMask);
    XSync(dpy, False);

    wm_log("Entering event loop\n");

    scan();

    /* Main event loop */
    while (running && !XNextEvent(dpy, &ev)) {
        wm_log("Processing event: %d\n", ev.type);
        if (handler[ev.type])
            handler[ev.type](&ev);
    }

    wm_log("Exiting event loop\n");
    /* Clean up */
    XCloseDisplay(dpy);
    if (logfile)
        fclose(logfile);
    return 0;
}
