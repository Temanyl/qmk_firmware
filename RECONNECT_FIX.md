# Volume Monitor - Reconnection Fix

## What Was Fixed

### Problem
- No message when keyboard disconnected
- No message when keyboard reconnected
- Volume bar stayed empty after reconnection
- Volume changes weren't sent after reconnection

### Solution
Added **active disconnection detection** that checks every second if the keyboard is still connected, plus immediate volume sync on reconnection.

## What You'll See Now

### Starting Without Keyboard
```
QMK Volume Monitor
==================================================
Waiting for keyboard... (Press Ctrl+C to quit)

Looking for keyboard (VID: 0xFEED, PID: 0x0000)...
Found keyboard: Chromanly36
✓ Connected to keyboard!
Monitoring system volume...

Syncing volume: 25%
```

### When You Unplug Keyboard
Within 1 second:
```
✗ Keyboard disconnected
Waiting for keyboard to reconnect...
```

### When You Plug Keyboard Back In
Within 2 seconds:
```
✓ Keyboard reconnected!
Monitoring system volume...

Syncing volume: 25%
```
**Volume bar on keyboard updates immediately!**

### Normal Volume Changes (while connected)
```
Volume changed: 30%
Volume changed: 35%
Volume changed: 40%
```

## How It Works

### Active Connection Monitoring
- Every 1 second, checks if keyboard still exists in HID device list
- Uses `hid.enumerate()` to verify device presence
- Faster detection than waiting for write errors

### Immediate Sync on Reconnect
When keyboard reconnects:
1. Gets current system volume
2. Sends it to keyboard immediately
3. Prints "Syncing volume: XX%"
4. Volume bar updates within 1 second

### Error Detection
- Checks `device.write()` return value (should be > 0)
- Catches all HID communication exceptions
- Gracefully handles device.close() errors

## Testing

### Test 1: Start without keyboard
```bash
python3 volume_monitor.py
# Wait a few seconds, then plug in keyboard
# Should see: "✓ Connected to keyboard!" and "Syncing volume: XX%"
```

### Test 2: Disconnect/Reconnect
```bash
python3 volume_monitor.py
# Wait for connection
# Unplug keyboard → Should see "✗ Keyboard disconnected"
# Plug back in → Should see "✓ Keyboard reconnected!" and "Syncing volume: XX%"
# Change volume → Should see "Volume changed: XX%"
```

### Test 3: Multiple disconnections
```bash
python3 volume_monitor.py
# Unplug and replug multiple times
# Each time should reconnect and sync volume
```

## Technical Changes

### New Function: `is_keyboard_connected()`
```python
def is_keyboard_connected():
    """Check if keyboard is still in HID device list."""
    devices = hid.enumerate()
    for device_info in devices:
        if (device_info['vendor_id'] == VENDOR_ID and ...):
            return True
    return False
```

### Connection Check in Main Loop
```python
# Check every 1 second if device is still there
if current_time - last_connection_check >= 1.0:
    if not is_keyboard_connected():
        print("✗ Keyboard disconnected")
        device = None  # Trigger reconnection
```

### Write Error Detection
```python
bytes_written = device.write([0] + list(packet))
if bytes_written <= 0:
    return False  # Write failed
```

### All Error Paths Print Message
Every place that detects disconnection now prints:
- "✗ Keyboard disconnected"
- "✗ Send failed, keyboard may be disconnected"
- "✗ Connection error: {error}"
- "Waiting for keyboard to reconnect..."

## Important Notes

### Only One Instance at a Time!
The HID device can only be opened by one process. If you see:
```
Error opening HID device: open failed
```

Kill existing instances:
```bash
pkill -f volume_monitor.py
```

### Detection Timing
- Disconnection detected: **within 1 second**
- Reconnection attempt: **every 2 seconds**
- Volume sync after reconnect: **immediate**

## Expected Behavior Summary

✅ Clear disconnect message
✅ Clear reconnect message
✅ Volume syncs immediately on reconnect
✅ Volume bar shows correct level after reconnect
✅ Volume changes work after reconnect
✅ No console spam during reconnection attempts
✅ Works with multiple disconnect/reconnect cycles

## Troubleshooting

### Volume still empty after reconnect?
Check the console output:
- Do you see "✓ Keyboard reconnected!"?
- Do you see "Syncing volume: XX%"?
- Do you see "Volume changed: XX%" when you change volume?

If not, there may be permission issues or another process has the device open.

### No "disconnected" message?
The check happens every 1 second. Wait at least 1 second after unplugging.

### Takes long to reconnect?
Reconnection attempts happen every 2 seconds. This is to avoid excessive CPU usage and HID enumeration.
