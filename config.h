#ifndef CONFIG_H
#define CONFIG_H

#include <X11/keysym.h>
#include <X11/XF86keysym.h>

/* Colors */
#define ACTIVE_BORDER   0xffbd93f9    /* Blue border for active window */
#define INACTIVE_BORDER 0x333333    

/* Key modifiers */
#define MODKEY Mod4Mask         /* Super (Windows) key */
#define SMODKEY Mod4Mask|ShiftMask  /* Super + Shift */
#define CSMODKEY Mod4Mask|ControlMask /* Super + Control */

/* External commands */
static const char *roficmd[] = {"rofi", "-theme", "~/.config/rofi/config.rasi", "-show", "drun", "-location", "0", "-xoffset", "0", "-yoffset", "0", NULL};
static const char *firefoxcmd[] = {"firefox", NULL};  // Web browser
static const char *wifi_choosercmd[] = {"~/.local/bin/wifi_chooser.sh", NULL};  // WiFi network selector 
static const char *digital_clockcmd[] = {"digital_clock", NULL};  // Digital clock
static const char *alacrittycmd[] = {"alacritty", NULL};  // Terminal emulator
static const char *powermenucmd[] = {"~/.local/bin/powermenu.sh", NULL};  // Power menu s
static const char *volupcmd[] = {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%", NULL};  // Volume up
static const char *voldowncmd[] = {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%", NULL};  // Volume down
static const char *volmutecmd[] = {"pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle", NULL};  // Mute volume
static const char *brightnessupcmd[] = {"brightnessctl", "set", "+5%", NULL};  // Brightness up
static const char *brightnessdowncmd[] = {"brightnessctl", "set", "5%-", NULL};  // Brightness down

/* Static arguments for commands */
static const char *const setmfact_dec[] = {"-0.05", NULL};
static const char *const setmfact_inc[] = {"+0.05", NULL};
static const char *const workspace1[] = {"1", NULL};
static const char *const workspace2[] = {"2", NULL};
static const char *const workspace3[] = {"3", NULL};
static const char *const workspace4[] = {"4", NULL};
static const char *const workspace5[] = {"5", NULL};
static const char *const workspace6[] = {"6", NULL};
static const char *const workspace7[] = {"7", NULL};
static const char *const workspace8[] = {"8", NULL};
static const char *const workspace9[] = {"9", NULL};

/* Key bindings structure */
static const struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(const char **);
    const char *const *arg;  // Updated to maintain const correctness
} keys[] = {
    /* Launchers */
    { MODKEY, XK_p, spawn, roficmd },         // Open application launcher
    { MODKEY, XK_w, spawn, firefoxcmd },      // Launch Firefox
    { SMODKEY, XK_w, spawn, wifi_choosercmd }, // Open WiFi selector
    { MODKEY, XK_c, spawn, digital_clockcmd }, // Launch digital clock
    { MODKEY, XK_Return, spawn, alacrittycmd }, // Open terminal emulator
    { SMODKEY, XK_s, spawn, powermenucmd },   // Open power menu
    { CSMODKEY, XK_r, reload, NULL },          // Reload window manager
    { SMODKEY, XK_r, reload_keys, NULL },    // Reload just key bindings
    
    /* Window control */
    { MODKEY, XK_j, focusnext, NULL },        // Focus next window
    { MODKEY, XK_k, focusprev, NULL },        // Focus previous window
    { SMODKEY, XK_f, togglefullscreen, NULL }, // Toggle fullscreen
    { SMODKEY, XK_c, killclient, NULL },      // Kill focused window
    { SMODKEY, XK_q, quit, NULL },            // Quit window manager
    
    /* Layout control */
    { MODKEY, XK_h, setmfact, setmfact_dec },  // Decrease master area
    { MODKEY, XK_l, setmfact, setmfact_inc },  // Increase master area
    { MODKEY, XK_space, togglefloating, NULL }, // Toggle floating layout
    { SMODKEY, XK_Return, swapmaster, NULL }, // Swap focused window with master
    
    /* System controls */
    { 0, XF86XK_AudioRaiseVolume, spawn, volupcmd },        // Volume up
    { 0, XF86XK_AudioLowerVolume, spawn, voldowncmd },      // Volume down
    { 0, XF86XK_AudioMute, spawn, volmutecmd },             // Mute volume
    { 0, XF86XK_MonBrightnessUp, spawn, brightnessupcmd },  // Brightness up
    { 0, XF86XK_MonBrightnessDown, spawn, brightnessdowncmd }, // Brightness down
    
    /* Workspace control */
    { SMODKEY, XK_1, sendtoworkspace, workspace1 },      /* Move window to workspace 1 */
    { MODKEY, XK_1, switchworkspace, workspace1 },     /* Switch to workspace 1 */
    { CSMODKEY, XK_1, clearworkspace, workspace1 },     /* Clear workspace 1 */
    { SMODKEY, XK_2, sendtoworkspace, workspace2 },      /* Move window to workspace 2 */
    { MODKEY, XK_2, switchworkspace, workspace2 },     /* Switch to workspace 2 */
    { CSMODKEY, XK_2, clearworkspace, workspace2 },     /* Clear workspace 2 */
    { SMODKEY, XK_3, sendtoworkspace, workspace3 },      /* Move window to workspace 3 */
    { MODKEY, XK_3, switchworkspace, workspace3 },     /* Switch to workspace 3 */
    { CSMODKEY, XK_3, clearworkspace, workspace3 },      /* Move window to workspace 4 */
    { SMODKEY, XK_4, sendtoworkspace, workspace4 },      /* Move window to workspace 4 */
    { MODKEY, XK_4, switchworkspace, workspace4 },     /* Switch to workspace 4 */
    { CSMODKEY, XK_4, clearworkspace, workspace4 },     /* Clear workspace 4 */
    { SMODKEY, XK_5, sendtoworkspace, workspace5 },      /* Move window to workspace 5 */
    { MODKEY, XK_5, switchworkspace, workspace5 },     /* Switch to workspace 5 */
    { CSMODKEY, XK_5, clearworkspace, workspace5 },     /* Clear workspace 5 */
    { SMODKEY, XK_6, sendtoworkspace, workspace6 },      /* Move window to workspace 6 */
    { MODKEY, XK_6, switchworkspace, workspace6 },     /* Switch to workspace 6 */
    { CSMODKEY, XK_6, clearworkspace, workspace6 },     /* Clear workspace 6 */
    { SMODKEY, XK_7, sendtoworkspace, workspace7 },      /* Move window to workspace 7 */
    { MODKEY, XK_7, switchworkspace, workspace7 },     /* Switch to workspace 7 */
    { CSMODKEY, XK_7, clearworkspace, workspace7 },     /* Clear workspace 7 */
    { SMODKEY, XK_8, sendtoworkspace, workspace8 },      /* Move window to workspace 8 */
    { MODKEY, XK_8, switchworkspace, workspace8 },     /* Switch to workspace 8 */
    { CSMODKEY, XK_8, clearworkspace, workspace8 },     /* Clear workspace 8 */
    { SMODKEY, XK_9, sendtoworkspace, workspace9 },      /* Move window to workspace 9 */
    { MODKEY, XK_9, switchworkspace, workspace9 },     /* Switch to workspace 9 */    
    { CSMODKEY, XK_9, clearworkspace, workspace9 },     /* Clear workspace 9 */
    { MODKEY, XK_Escape, showworkspace, NULL },  /* Show current workspace number */
};

/* Window manager settings */
#define BORDER_WIDTH  5     /* Window border size */
#define MASTER_SIZE   0.45  /* Master area ratio */
#define MIN_WIN_SIZE  45    /* Minimum window size */
#define MAX_WINDOWS   5     /* Window limit per workspace*/
#define GAP_WIDTH     5    /* Gap between windows */

#endif /* CONFIG_H */
