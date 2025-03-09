#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Function declarations */
void cleanup(void);  // Add this before any function uses it

/* Type definitions */
typedef struct Client {
    Window win;
    int x, y, w, h;
    struct Client *next;
    int isfloating;
    int workspace;  /* Workspace ID for this window */
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

#include "config.h"

/* Error handler */
static int
xerror(Display *dpy __attribute__((unused)), XErrorEvent *ee) {
    if (ee->error_code == BadAccess &&
        ee->request_code == X_ChangeWindowAttributes) {
        fprintf(stderr, "Error: Another window manager is already running\n");
        exit(1);
    }
    fprintf(stderr, "X error: request code=%d, error code=%d\n",
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

/* Event handler mapping table */
static void (*handler[LASTEvent]) (XEvent *) = {
    [MapRequest] = maprequest,      // Handle new window creation
    [KeyPress] = keypress,          // Handle keyboard input
    [DestroyNotify] = destroynotify, // Handle window destruction
    [ConfigureRequest] = configurerequest, // Handle window resize/move requests
    [EnterNotify] = enternotify,    // Handle mouse enter events
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

void
maprequest(XEvent *e)
{
    XMapRequestEvent *ev = &e->xmaprequest;
    Client *c;

    /* Check if we've reached the maximum for current workspace */
    if (count_windows_in_workspace(current_workspace) >= MAX_WINDOWS) {
        fprintf(stderr, "Maximum number of windows (%d) reached in workspace %d\n", 
                MAX_WINDOWS, current_workspace);
        return;
    }

    /* Add the window to our list */
    c = malloc(sizeof(Client));
    c->win = ev->window;
    c->next = clients;
    c->isfloating = 0;
    c->workspace = current_workspace;
    clients = c;
    sel = c;

    /* Set up window isolation */
    XSetWindowAttributes wa;
    wa.event_mask = EnterWindowMask | KeyPressMask;
    wa.override_redirect = True;  // Prevent direct window communication
    XChangeWindowAttributes(dpy, ev->window, CWEventMask | CWOverrideRedirect, &wa);

    /* Isolate window from others except via clipboard */
    XChangeProperty(dpy, ev->window, 
                   XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False),
                   XA_ATOM, 32, PropModeReplace,
                   (unsigned char *) &clipboard, 1);

    XMapWindow(dpy, ev->window);
    arrange();
    focus(sel);
}

void
keypress(XEvent *e)
{
    XKeyEvent *ev = &e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(dpy, ev->keycode, 0, 0);
    long unsigned int i;
    const char **args;  // Changed from const char * to const char **

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
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = BORDER_WIDTH;  /* Ensure border width is explicitly set */
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
}

void
focus(Client *c)
{
    if (!c)
        return;

    sel = c;
    XSetInputFocus(dpy, c->win, RevertToParent, CurrentTime);
    XRaiseWindow(dpy, c->win);
}

void
arrange(void)
{
    Client *c, *master = NULL;
    int n = 0, visible = 0;

    /* Count non-floating windows in current workspace and visible windows */
    for (c = clients; c; c = c->next) {
        if (!c->isfloating && c->workspace == current_workspace)
            n++;
        if (c->workspace == current_workspace)
            visible++;
    }

    /* Position floating windows first */
    for (c = clients; c; c = c->next) {
        if (c->isfloating && c->workspace == current_workspace) {
            XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
            continue;
        }
    }

    /* If only one visible window in workspace, make it fullscreen */
    if (visible == 1) {
        for (c = clients; c; c = c->next) {
            if (c->workspace == current_workspace) {
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
        int master_width = attr.width * mfact;
        XMoveResizeWindow(dpy, master->win, 0, 0,
                        master_width,
                        attr.height);
        XRaiseWindow(dpy, master->win);
    }

    /* Stack */
    if (n > 1) {
        int stack_width = attr.width * (1 - mfact);
        int x = attr.width * mfact;
        int i = 0;
        for (c = clients; c; c = c->next) {
            if (!c->isfloating && c != master) {
                int height = attr.height / (n - 1);
                XMoveResizeWindow(dpy, c->win, x, i * height,
                                stack_width, height);
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
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));
        setsid();
        execvp(((char **)arg)[0], (char **)arg);
        exit(0);
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
cleanup(void) {  // Non-static definition
    Client *c, *tmp;

    // Clean up all managed windows
    c = clients;
    while (c) {
        tmp = c->next;
        XUnmapWindow(dpy, c->win);
        free(c);
        c = tmp;
    }

    // Remove all keyboard shortcuts
    for (long unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        XUngrabKey(dpy, XKeysymToKeycode(dpy, keys[i].keysym),
                  keys[i].mod, root);
    }
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
    XCrossingEvent *ev = &e->xcrossing;
    Client *c;

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
            c->win = children[i];
            c->next = clients;
            c->isfloating = 0;
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
        fprintf(stderr, "Cannot move window: workspace %d is full (max %d windows)\n",
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

int
main(void)
{
    XEvent ev;

    fprintf(stderr, "Starting window manager\n");

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    /* Initialize clipboard atoms */
    clipboard = XInternAtom(dpy, "CLIPBOARD", False);
    primary_selection = XInternAtom(dpy, "PRIMARY", False);

    fprintf(stderr, "Display opened successfully\n");

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
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | EnterWindowMask);
    XSync(dpy, False);

    fprintf(stderr, "Entering event loop\n");

    scan();

    /* Main event loop */
    while (running && !XNextEvent(dpy, &ev)) {
        fprintf(stderr, "Processing event: %d\n", ev.type);
        if (handler[ev.type])
            handler[ev.type](&ev);
    }

    fprintf(stderr, "Exiting event loop\n");

    /* Clean up */
    XCloseDisplay(dpy);
    return 0;
}
