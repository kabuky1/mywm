#ifndef CONFIG_H
#define CONFIG_H

#include <X11/keysym.h>
#include <X11/XF86keysym.h>

/* Key modifiers */
#define MODKEY Mod4Mask         /* Super (Windows) key */
#define SMODKEY Mod4Mask|ShiftMask  /* Super + Shift */

/* External commands */
static const char *roficmd[] = {"rofi", "-theme", "~/.config/rofi/config.rasi", "-show", "drun", "-location", "0", "-xoffset", "0", "-yoffset", "0", NULL};
static const char *firefoxcmd[] = {"firefox", NULL};  // Web browser
static const char *wifi_choosercmd[] = {"wifi_chooser.sh", NULL};  // WiFi network selector
static const char *digital_clockcmd[] = {"digital_clock", NULL};  // Digital clock
static const char *alacrittycmd[] = {"alacritty", NULL};  // Terminal emulator
static const char *powermenucmd[] = {"powermenu.sh", NULL};  // Power menu
static const char *volupcmd[] = {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%", NULL};  // Volume up
static const char *voldowncmd[] = {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%", NULL};  // Volume down
static const char *volmutecmd[] = {"pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle", NULL};  // Mute volume
static const char *brightnessupcmd[] = {"brightnessctl", "set", "+5%", NULL};  // Brightness up
static const char *brightnessdowncmd[] = {"brightnessctl", "set", "5%-", NULL};  // Brightness down

/* Key bindings
 * mod: Modifier key combination
 * key: X11 key symbol
 * func: Function to execute
 * arg: Optional arguments */
static const struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(const char **);
    const char **arg;
} keys[] = {
    /* Launchers */
    { MODKEY, XK_p, spawn, roficmd },         // Open application launcher
    { MODKEY, XK_w, spawn, firefoxcmd },      // Launch Firefox
    { SMODKEY, XK_w, spawn, wifi_choosercmd }, // Open WiFi selector
    { MODKEY, XK_c, spawn, digital_clockcmd }, // Launch digital clock
    { MODKEY, XK_Return, spawn, alacrittycmd }, // Open terminal emulator
    { SMODKEY, XK_s, spawn, powermenucmd },   // Open power menu
    
    /* Window control */
    { MODKEY, XK_j, focusnext, NULL },        // Focus next window
    { MODKEY, XK_k, focusprev, NULL },        // Focus previous window
    { SMODKEY, XK_q, quit, NULL },            // Quit window manager
    
    /* Layout control */
    { MODKEY, XK_h, setmfact, (const char *[]){"-0.05", NULL} },  // Decrease master area
    { MODKEY, XK_l, setmfact, (const char *[]){"+0.05", NULL} },  // Increase master area
    { MODKEY, XK_space, togglefloating, NULL }, // Toggle floating layout
    { SMODKEY, XK_Return, swapmaster, NULL }, // Swap focused window with master
    
    /* System controls */
    { 0, XF86XK_AudioRaiseVolume, spawn, volupcmd },        // Volume up
    { 0, XF86XK_AudioLowerVolume, spawn, voldowncmd },      // Volume down
    { 0, XF86XK_AudioMute, spawn, volmutecmd },             // Mute volume
    { 0, XF86XK_MonBrightnessUp, spawn, brightnessupcmd },  // Brightness up
    { 0, XF86XK_MonBrightnessDown, spawn, brightnessdowncmd }, // Brightness down
    
    /* Workspace control */
    { SMODKEY, XK_1, sendtoworkspace, (const char *[]){"1", NULL} },      /* Move window to workspace 1 */
    { MODKEY, XK_1, switchworkspace, (const char *[]){"1", NULL} },     /* Switch to workspace 1 */
    { SMODKEY, XK_2, sendtoworkspace, (const char *[]){"2", NULL} },      /* Move window to workspace 2 */
    { MODKEY, XK_2, switchworkspace, (const char *[]){"2", NULL} },     /* Switch to workspace 2 */
    { SMODKEY, XK_3, sendtoworkspace, (const char *[]){"3", NULL} },      /* Move window to workspace 3 */
    { MODKEY, XK_3, switchworkspace, (const char *[]){"3", NULL} },     /* Switch to workspace 3 */
};

/* Window manager settings */
#define BORDER_WIDTH  5     /* Window border size */
#define MASTER_SIZE   0.45  /* Master area ratio */
#define MIN_WIN_SIZE  45    /* Minimum window size */
#define MAX_WINDOWS   5     /* Window limit per workspace*/

#endif /* CONFIG_H */
