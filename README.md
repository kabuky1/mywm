# Simple X11 Tiling Window Manager

A lightweight tiling window manager for X11 with configurable keyboard shortcuts.

## Features

- Tiling window management
- Dynamic keyboard shortcuts defined in config.h
- Master-stack layout
- Floating window support
- Simple and clean codebase

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

## Complete Keybindings

All keybindings use the Super (Windows) key as the modifier:

- `Super + j`: Focus next window
- `Super + k`: Focus previous window
- `Super + Return`: Spawn terminal (alacritty)
- `Super + Shift + q`: Quit window manager
- `Super + h`: Decrease master area size
- `Super + l`: Increase master area size
- `Super + Return`: Move focused window to master area
- `Super + Space`: Toggle floating mode for focused window
- `Super + Shift + Return`: Swap master window
- `Super + p`: Launch application menu (rofi)
- `Super + w`: Launch Firefox
- `Super + Shift + w`: Launch WiFi chooser
- `Super + c`: Launch digital clock
- `Super + Shift + s`: Open power menu

## Multimedia Keys

- `Volume Up`: Increase volume
- `Volume Down`: Decrease volume
- `Mute`: Toggle mute
- `Brightness Up`: Increase brightness
- `Brightness Down`: Decrease brightness

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

## Customization

Edit `config.h` to modify:
- Keyboard shortcuts
- Border colors and width
- Default layout parameters

Rebuild after making changes:
```bash
make clean && make
```
