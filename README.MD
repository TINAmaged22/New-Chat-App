# Chat System - Quick Start Guide

A simple Linux chat application with usernames using shared memory.

## What You Get

Three versions of the same chat app:
- **Terminal version** - Command-line interface
- **GUI version** - Graphical windows with buttons
- **Tabbed Chat** - WhatsApp-style tabbed interface (NEW!)

## Prerequisites (Linux Only)

```bash
# Install compiler
sudo apt-get update
sudo apt-get install g++

# For GUI version only
sudo apt-get install libgtk-3-dev
```

## Quick Start - Terminal Version

### 1. Compile
```bash
g++ -o writer writer.cpp
g++ -o reader reader.cpp
```

### 2. Run

**Open 3 terminals:**

Terminal 1:
```bash
./reader
```

Terminal 2:
```bash
./writer Alice
```

Terminal 3:
```bash
./writer Bob
```

### 3. Chat!
Type in Alice or Bob's terminal. Messages appear in the reader terminal:
```
Alice: Hello!
Bob: Hi Alice!
Alice: How are you?
```

Type `quit` to exit writer.

---

## Quick Start - GUI Version

### 1. Compile
```bash
chmod +x build_gui.sh
./build_gui.sh
```

### 2. Run

**Open 3 terminals:**

Terminal 1:
```bash
./reader_gui
```

Terminal 2:
```bash
./reader_gui Alice
```

Terminal 3:
```bash
./writer_gui Bob
```

### 3. Chat!
Type in the text box, click "Send" or press Enter. Messages appear in the reader window.

---

## Quick Start - Tabbed Chat (WhatsApp-Style)

### 1. Compile
```bash
chmod +x build_tabs.sh
./build_tabs.sh
```

### 2. Run
Open 2 terminals for two users:

Terminal 1 (Alice):
```bash
./chat_tabs Alice
```

Terminal 2 (Bob):
```bash
./chat_tabs Bob
```

### 3. Create Conversations
In Alice's window:
1. Click "+ New Chat"
2. Enter contact name: "Bob"
3. Enter room number: "1"
4. Click "Create"

In Bob's window:
1. Click "+ New Chat"
2. Enter contact name: "Alice"
3. Enter the SAME room number: "1"
4. Click "Create"

### 4. Chat!
Now both users can chat in their tabs. Each tab is a private 1-on-1 conversation.

**How it works:**
- Each tab = one conversation
- Both users must use the same room number to talk to each other
- You can have multiple tabs (Room 1 with Bob, Room 2 with Carol, etc.)
- Messages are real-time!

---

## Cleanup

After closing all programs:
```bash
# For old chat system (key 1234)
ipcs -m | grep 1234 | awk '{print $2}' | xargs ipcrm -m

# For tabbed chat (keys 2001-2999)
ipcs -m | grep -E '200[0-9]' | awk '{print $2}' | xargs -r ipcrm -m
```

## File List

**Core Files:**
- `writer.cpp` - Terminal writer
- `reader.cpp` - Terminal reader
- `writer_gui.cpp` - GUI writer
- `reader_gui.cpp` - GUI reader
- `chat_tabs.cpp` - Tabbed chat application (NEW!)
- `build_gui.sh` - GUI build script
- `build_tabs.sh` - Tabbed chat build script (NEW!)

**Documentation:**
- `QUICKSTART.MD` - This file

## Important Notes

**For Terminal & GUI versions:**
- ✅ **Always start the reader FIRST**
- ✅ **Multiple writers** - Many people can write, one reader shows all

**For Tabbed Chat:**
- ✅ **Both users need the same room number** - To chat together
- ✅ **Multiple tabs supported** - Have multiple conversations
- ✅ **Messages are bidirectional** - Both can send and receive

**General:**
- ✅ **Linux only** - Won't work on Windows
- ✅ **Usernames required** - Pass name as argument or enter when prompted

## Troubleshooting

**"Failed to create shared memory"**
- Run reader first
- Check permissions

**"Command not found"**
- Make sure you compiled the programs first
- Use `./` before the command

**GTK errors on Windows**
- Normal! This code is for Linux only
- Transfer files to Linux machine

## That's It!

Simple chat system in just 2 files. Enjoy!
