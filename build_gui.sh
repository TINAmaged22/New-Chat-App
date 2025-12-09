#!/bin/bash

echo "Building Chat System GUI for Linux..."
echo

# Check if GTK3 is installed
if ! pkg-config --exists gtk+-3.0; then
    echo "ERROR: GTK3 is not installed!"
    echo "Install it with: sudo apt-get install libgtk-3-dev"
    exit 1
fi

# Compile writer GUI
echo "Compiling writer GUI..."
g++ writer_gui.cpp -o writer_gui `pkg-config --cflags --libs gtk+-3.0`
if [ $? -ne 0 ]; then
    echo "Failed to compile writer GUI!"
    exit 1
fi

# Compile reader GUI
echo "Compiling reader GUI..."
g++ reader_gui.cpp -o reader_gui `pkg-config --cflags --libs gtk+-3.0`
if [ $? -ne 0 ]; then
    echo "Failed to compile reader GUI!"
    exit 1
fi

echo
echo "Build successful!"
echo "Run './writer_gui' to send messages"
echo "Run './reader_gui' to receive messages"
echo

chmod +x writer_gui
chmod +x reader_gui
