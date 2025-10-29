# Date/Time Display Feature

## Overview

The keyboard now displays the current date and time instead of the uptime counter. The date/time is received from the host computer via HID, so it stays accurate without needing an RTC module.

## Display Format

- **Date**: DD.MM.YY (e.g., 29.10.25)
- **Time**: HH:MM (24-hour format)

The display updates automatically every minute and is positioned above the media text, using the same 7-segment digit style as before.

## Firmware Changes

### New State Variables (keymap.c:36-42)
```c
static uint8_t current_hour = 0;
static uint8_t current_minute = 0;
static uint8_t current_day = 1;
static uint8_t current_month = 1;
static uint16_t current_year = 2025;
static bool time_received = false;
```

### New HID Command (0x03)
```c
// Protocol format:
// Byte 0: 0x03 (Command ID)
// Bytes 1-2: Year (16-bit little-endian)
// Byte 3: Month (1-12)
// Byte 4: Day (1-31)
// Byte 5: Hour (0-23)
// Byte 6: Minute (0-59)
// Byte 7: Second (0-59, not displayed but received)
```

### Function Changes
- `draw_uptime_timer()` → `draw_date_time()`
  - Removed uptime calculation
  - Now draws date in DD.MM.YY format
  - Shows time in HH:MM format (no seconds)
  - Uses layer colors for digits

### Update Behavior
- Receives initial time from host on connection
- Local minute counter increments every 60 seconds
- Host re-syncs time every 60 seconds to prevent drift
- Display only updates when time actually changes

## Host Software Changes

### keyboard_monitor.py

**New constant**:
```python
CMD_DATETIME_UPDATE = 0x03
```

**New function** (`send_datetime_update()`):
- Packs current system date/time into HID packet
- Sends to keyboard every 60 seconds
- Also sends immediately on (re)connect

**Integration**:
- Syncs time on keyboard connect/reconnect
- Periodic updates every 60 seconds in main loop
- Handles connection failures gracefully

### send_datetime.py (standalone)

A simple standalone script for testing or one-time sync:
```bash
python3 send_datetime.py
```

This script:
- Finds the keyboard
- Sends current date/time once
- Exits

Useful for testing or manual sync without running the full monitor.

## Usage

### With keyboard_monitor.py (recommended)
The keyboard_monitor.py script now automatically handles:
- Volume monitoring
- Media text scrolling
- **Date/time synchronization** (new!)

Just run it as before:
```bash
python3 keyboard_monitor.py
```

The script will:
1. Connect to keyboard
2. Sync current date/time immediately
3. Update date/time every 60 seconds
4. Handle reconnections automatically

### Standalone sync (optional)
For one-time sync or testing:
```bash
python3 send_datetime.py
```

## Configuration

### Update Interval
To change how often the host sends time updates, edit `keyboard_monitor.py`:
```python
datetime_update_interval = 60.0  # Seconds between updates
```

### Display Format
To change the date format, edit the `draw_date_time()` function in `keymap.c`.

Current format: DD.MM.YY
Could be changed to: MM.DD.YY, DD/MM/YY, etc.

## Notes

- The keyboard does not have a real-time clock, so it relies on the host
- Time is kept roughly accurate between syncs using the keyboard's timer
- If the host disconnects, time continues counting but may drift
- On reconnect, time is immediately re-synced from host
- The display shows YY (2-digit year) to save space, but receives full 4-digit year

## Technical Details

### Memory Usage
- Added 7 bytes of state variables
- Time keeping uses existing timer infrastructure
- No additional dynamic memory allocation

### Performance
- Display updates once per minute (vs. once per second for uptime)
- Reduces SPI transactions by 98%
- More efficient than uptime display

### Compatibility
- Backwards compatible with existing volume/media commands
- New command (0x03) is optional - keyboard works without it
- If time is never received, display shows default values
