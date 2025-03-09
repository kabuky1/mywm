#!/bin/bash

wifi_device=$(ls /sys/class/ieee80211/*/device/net/* -d | sed -E 's|^.*(phy[^/]+)/.*/|\1 |')

# Function to get available networks
get_networks() {
    iwctl station $wifi_device scan
    sleep 2
    iwctl station $wifi_device get-networks | tail -n +5 | head -n -1 | sed 's/\x1B\[[0-9;]*[JKmsu]//g' | awk '{print $1}'
}

# Function to connect to a network
connect_network() {
    local network="$1"
    local password

    if iwctl known-networks list | grep -q "$network"; then
        iwctl station $wifi_device connect "$network"
    else
        password=$(rofi -dmenu -p "Enter password for $network: " -password)
        if [ -n "$password" ]; then
            iwctl --passphrase "$password" station wlan0 connect "$network"
        fi
    fi
}

# Main menu
main_menu() {
    local choice
    choice=$(echo -e "Connect to network\nDisconnect\nKnown networks\nExit" | rofi -dmenu -p "Wi-Fi Menu")

    case "$choice" in
        "Connect to network")
            network=$(get_networks | rofi -dmenu -p "Select network")
            [ -n "$network" ] && connect_network "$network"
            ;;
        "Disconnect")
            iwctl station $wifi_device disconnect
            ;;
        "Known networks")
            known_network=$(iwctl known-networks list | tail -n +5 | sed 's/\x1B\[[0-9;]*[JKmsu]//g' | awk '{print $1}' | rofi -dmenu -p "Known networks")
            [ -n "$known_network" ] && connect_network "$known_network"
            ;;
        "Exit")
            exit 0
            ;;
    esac
}

# Run the main menu
main_menu
