#!/bin/bash

echo "Building Tabbed Chat Application for Linux..."
echo

# Check if GTK3 is installed
if ! pkg-config --exists gtk+-3.0; then
    echo "ERROR: GTK3 is not installed!"
    echo "Install it with: sudo apt-get install libgtk-3-dev"
    exit 1
fi

# Compile tabbed chat application
echo "Compiling tabbed chat application..."
g++ chat_tabs.cpp -o chat_tabs `pkg-config --cflags --libs gtk+-3.0`
if [ $? -ne 0 ]; then
    echo "Failed to compile tabbed chat application!"
    exit 1
fi

echo
echo "Build successful!"
echo "Run './chat_tabs YourName' to start chatting"
echo "Example: ./chat_tabs Alice"
echo

chmod +x chat_tabs
