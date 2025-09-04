#!/bin/bash

# Check if libfuse2 is installed
if ldconfig -p | grep -q libfuse.so.2; then
    echo "FUSE (libfuse2) is already installed."
    exit 0
fi

echo "FUSE (libfuse2) is not installed. Attempting to install..."

# Detect package manager and install
if [ -x "$(command -v apt-get)" ]; then
    echo "Detected apt-get. Installing libfuse2..."
    sudo apt-get update
    sudo apt-get install -y libfuse2
elif [ -x "$(command -v dnf)" ]; then
    echo "Detected dnf. Installing fuse..."
    sudo dnf install -y fuse
elif [ -x "$(command -v pacman)" ]; then
    echo "Detected pacman. Installing fuse2..."
    sudo pacman -Sy --noconfirm fuse2
else
    echo "Unsupported package manager. Please install FUSE manually."
    exit 1
fi

# Check again
if ldconfig -p | grep -q libfuse.so.2; then
    echo "FUSE (libfuse2) installed successfully."
    exit 0
else
    echo "Failed to install FUSE. Please install it manually."
    exit 1
fi 