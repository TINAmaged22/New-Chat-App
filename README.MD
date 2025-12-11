# Chat System - Quick Start Guide

A simple Linux chat application with usernames using shared memory.

## Prerequisites (Linux Only)

```bash
# Install compiler
sudo apt-get update
sudo apt-get install g++

# For Raylib version
sudo apt-get install libraylib-dev
```

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
./chat_gui Martina
```

Terminal 2:
```bash
./chat_gui Youstina
```

### 3. Chat!
Type in the text box, click "Send" or press Enter. Messages appear in the reader window.

---

## File List

**Core Files:**
- `build_gui.sh` - GTK+ GUI build script

**Documentation:**
- `README.MD` - This file

## Important Notes

**For Terminal & GUI versions:**
- ✅ **Always start the reader FIRST**
- ✅ **Multiple writers** - Many people can write, one reader shows all

**General:**
- ✅ **Linux only** - Won't work on Windows
- ✅ **Usernames required** - Pass name as argument or enter when prompted

