#!/bin/bash

# Violet Build Script
# This script sets up and builds the Violet LV2 Plugin Host for Windows

set -e  # Exit on any error

echo "Building Violet LV2 Plugin Host..."

# Check if MinGW-w64 is available
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: MinGW-w64 cross-compiler not found"
    echo "On Fedora, install with: sudo dnf install mingw64-gcc mingw64-gcc-c++"
    exit 1
fi

# Clean previous build if requested
if [ "$1" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build/
fi

# Set up build directory
if [ ! -d "build" ]; then
    echo "Setting up build directory..."
    meson setup build --cross-file cross-mingw64.txt
fi

# Build the project
echo "Compiling..."
meson compile -C build

echo ""
echo "Build completed successfully!"
echo "Executables created:"
echo "  - build/violet.exe (GUI version)"
echo "  - build/violet-console.exe (Debug console version)"
echo ""
echo "To run on Windows, copy the .exe files to a Windows machine."
echo "The application requires Windows 10 or later."