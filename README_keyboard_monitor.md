# QMK Keyboard Monitor - Usage Guide

## What This Script Does

The `keyboard_monitor.py` script monitors your system volume and currently playing media, displaying both on your QMK keyboard's display via Raw HID communication.

## Features

### ✅ Start Before Keyboard Connected
You can start the script before plugging in your keyboard. It will wait and automatically connect when the keyboard is available.

### ✅ Automatic Reconnection
If you unplug and replug your keyboard (or it disconnects for any reason), the script will:
1. Detect the disconnection
2. Wait for the keyboard to reappear (checks every 2 seconds)
3. Automatically reconnect
4. **Immediately sync the current volume to the keyboard**

### ✅ No Console Spam
- First connection: Shows connection messages
- Reconnection attempts: Silent (no spam while waiting)
- Successful reconnect: Shows "✓ Keyboard reconnected!"

## Usage

### Basic Usage
```bash
python3 keyboard_monitor.py
```

### Running in Background (macOS)
```bash
# Start in background
python3 keyboard_monitor.py > /tmp/keyboard_monitor.log 2>&1 &

# Check if running
ps aux | grep keyboard_monitor

# Stop
pkill -f keyboard_monitor.py
```

### Autostart on Login (macOS)
Use the LaunchAgent instructions from the main documentation.

## Expected Behavior

### On First Start (Keyboard Already Connected)
```
QMK Keyboard Monitor
==================================================
Waiting for keyboard... (Press Ctrl+C to quit)

Looking for keyboard (VID: 0xFEED, PID: 0x0000)...
Found keyboard: Chromanly36
✓ Connected to keyboard!
Monitoring system volume...

Syncing volume: 44%
```

### On First Start (Keyboard NOT Connected)
```
QMK Keyboard Monitor
==================================================
Waiting for keyboard... (Press Ctrl+C to quit)

(waits silently... plug in keyboard...)

✓ Connected to keyboard!
Monitoring system volume...

Syncing volume: 44%
```

### When Keyboard Disconnects and Reconnects
```
Volume changed: 50%
✗ Send failed, keyboard may be disconnected

(waits silently for ~2 seconds...)

✓ Keyboard reconnected!
Monitoring system volume...

Syncing volume: 50%
```

## Troubleshooting

### Volume not updating after reconnect?
- **Fixed!** The script now immediately sends the current volume when reconnecting
- The volume bar on your keyboard should update within 1 second of reconnection

### Script can't find keyboard?
- Check that VID/PID in the script matches your keyboard.json
- Make sure Raw HID firmware is flashed
- Try: `python3 -c "import hid; print([d for d in hid.enumerate() if d['vendor_id'] == 0xFEED])"`

### Permission denied on macOS?
- macOS may require Input Monitoring permissions
- System Preferences → Security & Privacy → Privacy → Input Monitoring
- Add Terminal.app or your terminal emulator

## Technical Details

### Reconnection Logic
- Checks for keyboard every 2 seconds when disconnected
- Uses exponential backoff to avoid excessive CPU usage
- Immediately syncs volume state on successful reconnection
- Handles HID write errors gracefully

### Volume Sync on Reconnect
When the keyboard reconnects, the script:
1. Calls `get_system_volume()` to get current volume
2. Sends it immediately via `send_volume_update()`
3. Updates the volume bar on the keyboard display
4. Continues normal monitoring

This ensures the keyboard display is always accurate, even after disconnection.
