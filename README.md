# Minimalist Window Manager

A lightweight X11 window manager with workspaces and window isolation.

## Features

- Tiling window management
- Multiple workspaces (1-9)
- Window isolation (windows can only communicate via clipboard)
- Smart borders (hidden for single/fullscreen windows)
- Master-stack layout with gaps
- Floating window support
- Per-workspace window limits
- Automatic fullscreen for single windows
- Colored borders for active/inactive windows
- Drag and drop window swapping

## Installation Prerequisites

### X Server
You'll need a running Xorg server. Most Linux distributions include this by default, but if not:

### Debian/Ubuntu
```bash
sudo apt install xorg build-essential libx11-dev libxft-dev libxinerama-dev
```

### Fedora
```bash
sudo dnf install xorg-x11-server-Xorg gcc libX11-devel libXft-devel libXinerama-devel
```

### Arch Linux
```bash
sudo pacman -S xorg-server base-devel libx11 libxft libxinerama
```

## Default Keybindings

### Window Management
- `Super + j` - Focus next window
- `Super + k` - Focus previous window
- `Super + Space` - Toggle floating mode
- `Super + Shift + Return` - Swap focused window with master
- `Super + Shift + c` - Kill focused window
- `Super + Shift + f` - Toggle fullscreen mode
- `Super + Left Click + Drag` - Drag window to swap position with another window

### Workspace Control
- `Super + [1-9]` - Switch to workspace 1-9
- `Super + Shift + [1-9]` - Move focused window to workspace 1-9

### Layout Control
- `Super + h` - Decrease master area
- `Super + l` - Increase master area

### Applications
- `Super + p` - Launch application menu (rofi)
- `Super + w` - Launch Firefox
- `Super + Shift + w` - Launch WiFi selector
- `Super + c` - Launch digital clock
- `Super + Return` - Launch terminal (alacritty)
- `Super + Shift + s` - Launch power menu

### System Controls
- `XF86AudioRaiseVolume` - Volume up
- `XF86AudioLowerVolume` - Volume down
- `XF86AudioMute` - Toggle mute
- `XF86MonBrightnessUp` - Brightness up
- `XF86MonBrightnessDown` - Brightness down

### Session
- `Super + Shift + q` - Quit window manager

## Building

Requirements:
- GCC or compatible C compiler
- X11 development libraries
- make

To build:
```bash
make
```

## Running

1. Make sure you have a running X server
2. Add the following to your ~/.xinitrc:
   ```bash
   exec /path/to/wm
   ```
3. Start X with `startx`

## Configuration

The window manager can be configured by editing:
- `config.h` - Keybindings and general settings

### Important Settings
- `MAX_WINDOWS`: Maximum windows per workspace (default: 5)
- `BORDER_WIDTH`: Window border size in pixels (default: 2)
- `MASTER_SIZE`: Default master area ratio (default: 0.45)
- `GAP_WIDTH`: Size of gaps between windows (default: 5)
- `ACTIVE_BORDER`: Color of focused window border (default: purple)
- `INACTIVE_BORDER`: Color of unfocused window borders (default: dark grey)

Rebuild after making changes:
```bash
make clean install
```
