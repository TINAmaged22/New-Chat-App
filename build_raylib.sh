#!/bin/bash

echo "Building Raylib Tabbed Chat Application..."
echo

# Check if raylib is installed
if ! pkg-config --exists raylib; then
    echo "ERROR: Raylib is not installed!"
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt-get install libraylib-dev"
    echo "  Or download from: https://www.raylib.com/"
    exit 1
fi

# Compile Raylib tabbed chat application
echo "Compiling Raylib tabbed chat..."
g++ chat_tabs_raylib.cpp -o chat_tabs_raylib `pkg-config --cflags --libs raylib` -std=c++11
if [ $? -ne 0 ]; then
    echo "Failed to compile Raylib chat application!"
    exit 1
fi

echo
echo "Build successful!"
echo "Run './chat_tabs_raylib YourName' to start chatting"
echo "Example: ./chat_tabs_raylib Alice"
echo

chmod +x chat_tabs_raylib
