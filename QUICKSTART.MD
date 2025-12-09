# Chat System - Quick Start Guide

A simple Linux chat application with usernames using shared memory.

## What You Get

Two versions of the same chat app:
- **Terminal version** - Command-line interface
- **GUI version** - Graphical windows with buttons

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

## Cleanup

After closing all programs:
```bash
ipcs -m | grep 1234 | awk '{print $2}' | xargs ipcrm -m
```

## File List

**Core Files:**
- `writer.cpp` - Terminal writer
- `reader.cpp` - Terminal reader
- `writer_gui.cpp` - GUI writer
- `reader_gui.cpp` - GUI reader
- `build_gui.sh` - GUI build script

**Documentation:**
- `QUICKSTART_CHAT.md` - This file
- `USAGE.md` - Detailed usage
- `README_GUI.md` - GUI details

## Important Notes

- ✅ **Always start the reader FIRST**
- ✅ **Linux only** - Won't work on Windows
- ✅ **Multiple writers** - Many people can write, one reader shows all
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
