# Minimal X11 Window Manager

A lightweight, customizable window manager written in C for X11.

## Features

- Dynamic tiling layout with master/stack configuration
- Multiple workspaces (1-9)
- Floating window support
- Fullscreen mode
- Window gaps
- Workspace indicators
- Live config reload
- Configurable keybindings
- Child window handling for fullscreen applications

## Dependencies

- X11 development libraries
- gcc
- make
- rofi (for application launcher)
- alacritty (default terminal)
- pactl (for volume control)
- brightnessctl (for brightness control)

## Installation

```bash
# Install dependencies (for Arch Linux)
sudo pacman -S libx11 gcc make rofi alacritty pulseaudio brightnessctl

# Clone and build
git clone https://github.com/kabuky1/wm.git
cd wm
make
make install
```

## Configuration

Edit `config.h` to customize:
- Keybindings
- Colors
- Border width
- Gap size
- Window limits
- External commands

## Default Keybindings

### System
- Super + Shift + r: Reload window manager
- Super + Shift + q: Quit window manager
- Super + Escape: Show current workspace

### Window Management
- Super + j/k: Focus next/previous window
- Super + Shift + f: Toggle fullscreen
- Super + Space: Toggle floating
- Super + h/l: Decrease/increase master area
- Super + Shift + Return: Swap with master
- Super + Shift + c: Close window

### Workspaces
- Super + [1-9]: Switch to workspace
- Super + Shift + [1-9]: Move window to workspace
- Super + Control + [1-9]: Clear workspace

### Launchers
- Super + p: Application launcher (rofi)
- Super + Return: Terminal (alacritty)
- Super + w: Web browser (firefox)

## Logging

Logs are written to `~/.local/share/wm/wm.log`

## License

MIT License
