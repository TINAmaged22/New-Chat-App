#!/bin/bash

echo "Building Simple Chat GUI..."
echo

# Compile the GUI chat application
echo "Compiling chat_gui.cpp..."
g++ chat_gui.cpp -o chat_gui -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c++11

if [ $? -ne 0 ]; then
    echo "Failed to compile!"
    exit 1
fi

echo
echo "Build successful!"
echo "Run './chat_gui YourName' to start chatting"
echo "Example: ./chat_gui Alice"
echo

chmod +x chat_gui
