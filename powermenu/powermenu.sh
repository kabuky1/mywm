#!/bin/bash

options="Lock\nLogout\nReboot\nShutdown"

selected=$(echo -e "$options" | rofi -dmenu -i -p "Power Menu")

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

detect_session_type() {
    if [ -n "$WAYLAND_DISPLAY" ]; then
        echo "wayland"
    elif [ -n "$DISPLAY" ]; then
        echo "x11"
    else
        echo "unknown"
    fi
}

case $selected in
    Lock)
        session_type=$(detect_session_type)
        if [ "$session_type" = "wayland" ]; then
            hyprlock
        else
            i3lock -c 000000
        fi
        ;;
    Logout)
        loginctl terminate-user $USER
        ;;
    Reboot)
        systemctl reboot
        ;;
    Shutdown)
        systemctl poweroff
        ;;
esac
