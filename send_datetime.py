#!/usr/bin/env python3
"""
Send current date and time to the keyboard via HID.
This script should be run periodically (e.g., every minute) to keep the keyboard's clock synced.
"""

import hid
import time
from datetime import datetime

# USB Vendor and Product IDs for the keyboard
VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000  # Update this with your keyboard's product ID

def find_keyboard():
    """Find the keyboard HID device."""
    for device in hid.enumerate():
        if device['vendor_id'] == VENDOR_ID and device['product_id'] == PRODUCT_ID:
            if device['usage_page'] == 0xFF60 or device['usage'] == 0x61:  # Raw HID
                return device['path']
    return None

def send_datetime(device_path):
    """Send current date and time to the keyboard."""
    try:
        # Open the device
        h = hid.device()
        h.open_path(device_path)

        # Get current date/time
        now = datetime.now()

        # Prepare the HID packet (32 bytes for raw HID)
        # Command 0x03 = Date/Time update
        # Format: [cmd, year_low, year_high, month, day, hour, minute, second, padding...]
        packet = [0] * 32
        packet[0] = 0x03  # Command ID
        packet[1] = now.year & 0xFF  # Year low byte
        packet[2] = (now.year >> 8) & 0xFF  # Year high byte
        packet[3] = now.month
        packet[4] = now.day
        packet[5] = now.hour
        packet[6] = now.minute
        packet[7] = now.second

        # Send the packet
        h.write(packet)
        h.close()

        print(f"Sent date/time to keyboard: {now.strftime('%Y-%m-%d %H:%M:%S')}")
        return True

    except Exception as e:
        print(f"Error sending date/time: {e}")
        return False

def main():
    print("Looking for keyboard...")
    device_path = find_keyboard()

    if not device_path:
        print(f"Keyboard not found (VID: 0x{VENDOR_ID:04X}, PID: 0x{PRODUCT_ID:04X})")
        print("Make sure your keyboard is connected and the product ID is correct.")
        return

    print(f"Found keyboard at: {device_path}")

    # Send time once
    send_datetime(device_path)

    print("\nTo keep the clock synced, run this script periodically.")
    print("For example, add to crontab:")
    print("  */1 * * * * /usr/bin/python3 /path/to/send_datetime.py")

if __name__ == "__main__":
    main()
