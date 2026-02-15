#!/bin/bash
set -eu

# Remove yourself from the input group
sudo gpasswd -d eswari input

# Remove the udev rule for /dev/uinput
sudo rm /etc/udev/rules.d/99-coral-uinput.rules
sudo udevadm control --reload-rules
sudo udevadm trigger /dev/uinput
