# Doodle Jump High Score System

This document explains how to use the high score system for the Doodle Jump game on the chocmanyl36 keyboard.

## Overview

The high score system allows you to:
- Save your top 10 scores to a file on your computer
- Enter a 3-letter name (arcade-style) when you achieve a top 10 score
- View the high score leaderboard after each game
- Restart the game from the high score screen
- **Works offline!** Even without the Python script running, you can test the name entry UI

## Setup

### 1. Install Python Dependencies

The high score manager requires Python 3 and the `hid` library:

```bash
pip3 install hid
```

### 2. Flash the Firmware

Build and flash the updated firmware:

```bash
# From the repository root
make handwired/temanyl/chocmanyl36:default:flash
```

### 3. Start the High Score Manager

Run the Python script to start the high score manager:

```bash
cd keyboards/handwired/temanyl/chocmanyl36/keymaps/default
python3 highscore_manager.py
```

The script will:
- Connect to your keyboard via Raw HID
- Display current high scores (if any)
- Listen for new scores from the game
- Save scores to `highscores.json` in the same directory

**Important**: The script must be running for the high score system to work!

## How to Use

### Playing the Game

1. Switch to the arrow layer (game layer) on your keyboard
2. Play Doodle Jump using the arrow keys:
   - **LEFT/RIGHT**: Move the player
   - **SHIFT**: Exit game (during gameplay)
3. When you die, your score is automatically sent to the computer

### Online Mode (Python Script Running)

**If Your Score Makes Top 10:**

You'll see the **"NEW HIGH SCORE!"** screen with arcade-style name entry:

- **UP/DOWN**: Cycle through letters A-Z for the current position
- **LEFT/RIGHT**: Move to the next/previous character position
- **SHIFT**: Submit your name

The current character is highlighted in **green**. Navigate through all 3 positions to spell your name.

**If Your Score Doesn't Make Top 10:**

You'll see the **"HIGH SCORES"** screen showing the top 10 scores:

- **SHIFT**: Restart the game

### Offline Mode (No Python Script)

**Don't worry if the Python script isn't running!** The game will automatically detect this after a 2-second timeout and let you test the name entry system anyway.

When offline:
1. After dying, wait 2 seconds
2. You'll see the name entry screen with "OFFLINE MODE" indicator
3. Enter your 3-letter name using the same controls
4. Press **SHIFT** to submit
5. You'll see a "THANK YOU" screen showing your name and score
6. Press **SHIFT** to restart

**Note**: Scores entered in offline mode are NOT saved. Start the Python script to enable score persistence.

## High Score File Format

Scores are saved in `highscores.json` with the following format:

```json
[
  {
    "name": "ABC",
    "score": 42
  },
  {
    "name": "XYZ",
    "score": 30
  }
]
```

You can manually edit this file to reset scores or add custom entries.

## Communication Protocol

The keyboard and computer communicate via Raw HID with these message types:

| Message ID | Direction | Description |
|------------|-----------|-------------|
| 0x10 | Keyboard → PC | Score submission (2 bytes: score) |
| 0x11 | PC → Keyboard | Enter name prompt (1 byte: rank) |
| 0x12 | PC → Keyboard | Show scores (top 10 list) |
| 0x13 | Keyboard → PC | Name submission (3 chars + 2 bytes score) |

## Troubleshooting

### Script Can't Find Keyboard

- Ensure the keyboard is plugged in
- Check that Raw HID is enabled in the firmware
- Verify VID (0xFEED) and PID (0x1805) match your keyboard

### Scores Not Saving

- Check that the Python script is running
- Verify file permissions in the keymap directory
- Check for error messages in the script output
- **Remember**: Offline mode doesn't save scores - this is intentional for testing!

### Name Entry Not Working

- Ensure you're using arrow keys and SHIFT
- Check that you're on the name entry screen (not score display)
- Try restarting the game and Python script

### Game Stuck on "GAME OVER" Screen

- This should no longer happen! The game now has a 2-second timeout
- After 2 seconds without computer response, it automatically enters offline mode
- If still stuck, try exiting with SHIFT and restarting the game

### Want to Test Name Entry Without Python Script

- Just play the game normally!
- When you die, wait 2 seconds
- The game will automatically enter offline mode
- You can test the full name entry UI
- Your name won't be saved (this is intentional)

## Technical Details

### Files Modified

- `game_doodle.h` - Added game states and high score structures
- `game_doodle.c` - Implemented name entry, score display, HID communication
- `keymap.c` - Added game HID handling to existing Raw HID callback
- `rules.mk` - Enabled Raw HID feature
- `highscore_manager.py` - Computer-side score management script

### Game States

1. **GAME_PLAYING** - Normal gameplay
2. **GAME_NAME_ENTRY** - Arcade-style 3-letter name input (online or offline)
3. **GAME_SCORE_DISPLAY** - Show top 10 leaderboard (online) or thank you screen (offline)

### Offline Mode Features

- **2-second timeout**: If no computer response, automatically enter offline mode
- **Full UI testing**: Test the entire name entry system without the Python script
- **Visual indicators**: "OFFLINE MODE" text shows when computer isn't responding
- **No persistence**: Scores entered offline are intentionally not saved (testing only)

### Display Rendering

- **Name Entry**: Dark background with yellow title, white text, green highlighted current character
- **Score Display**: Dark background with yellow title, green names, white scores

## Credits

High score system implementation for the chocmanyl36 Doodle Jump game.
Uses QMK Raw HID for keyboard-to-computer communication.
