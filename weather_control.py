#!/usr/bin/env python3
"""
Weather Control Script for QMK Keyboard
Sends weather transition commands via Raw HID.

Usage:
    python3 weather_control.py sunny
    python3 weather_control.py rain
    python3 weather_control.py snow

Requirements:
    pip3 install hidapi
"""

import sys
import time

# Import hidapi
try:
    import hid
except ImportError:
    print("Error: hidapi not installed!")
    print("Install with: pip3 install hidapi")
    sys.exit(1)

# USB Vendor and Product IDs for your keyboard
VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000

# Raw HID usage page and usage (standard for QMK)
USAGE_PAGE = 0xFF60
USAGE = 0x0061

# HID packet size (32 bytes for Raw HID)
HID_PACKET_SIZE = 32

# Weather control command
CMD_WEATHER_CONTROL = 0x04

# Weather states
WEATHER_SUNNY = 0
WEATHER_RAIN = 1
WEATHER_SNOW = 2

WEATHER_NAMES = {
    'sunny': WEATHER_SUNNY,
    'rain': WEATHER_RAIN,
    'snow': WEATHER_SNOW
}


def find_keyboard():
    """Find the keyboard HID device."""
    print(f"Looking for keyboard (VID: 0x{VENDOR_ID:04X}, PID: 0x{PRODUCT_ID:04X})...")

    # List all HID devices
    devices = hid.enumerate()

    # Find our keyboard by VID/PID and usage page
    for device_info in devices:
        if (device_info['vendor_id'] == VENDOR_ID and
            device_info['product_id'] == PRODUCT_ID and
            device_info['usage_page'] == USAGE_PAGE and
            device_info['usage'] == USAGE):

            print(f"✓ Found keyboard: {device_info['product_string']}")
            return device_info['path']

    return None


def send_weather_command(device, weather_state):
    """Send weather control command to keyboard."""
    # Create HID packet (32 bytes)
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_WEATHER_CONTROL
    packet[1] = weather_state

    try:
        # Send the packet (first byte is report ID, usually 0)
        bytes_written = device.write([0] + list(packet))

        if bytes_written <= 0:
            print(f"✗ Write failed: {bytes_written} bytes written")
            return False

        return True
    except Exception as e:
        print(f"✗ Error sending HID packet: {e}")
        return False


def main():
    if len(sys.argv) < 2:
        print("Weather Control Script for QMK Keyboard")
        print()
        print("Usage:")
        print("  python3 weather_control.py <weather>")
        print()
        print("Weather options:")
        print("  sunny  - Clear sunny weather")
        print("  rain   - Rainy weather")
        print("  snow   - Snowy weather")
        print()
        print("Examples:")
        print("  python3 weather_control.py sunny")
        print("  python3 weather_control.py rain")
        print("  python3 weather_control.py snow")
        sys.exit(1)

    weather_name = sys.argv[1].lower()

    if weather_name not in WEATHER_NAMES:
        print(f"✗ Invalid weather: {weather_name}")
        print(f"Valid options: {', '.join(WEATHER_NAMES.keys())}")
        sys.exit(1)

    weather_state = WEATHER_NAMES[weather_name]

    # Find keyboard
    device_path = find_keyboard()
    if not device_path:
        print("✗ Keyboard not found!")
        print("Make sure the keyboard is plugged in.")
        sys.exit(1)

    # Open device
    try:
        device = hid.device()
        device.open_path(device_path)
        print("✓ Connected to keyboard")
    except Exception as e:
        print(f"✗ Error opening device: {e}")
        sys.exit(1)

    # Send weather command
    print(f"📤 Sending weather command: {weather_name}")
    if send_weather_command(device, weather_state):
        print(f"✓ Weather transition started: {weather_name}")
        print(f"⏱  Transition will complete in ~30 seconds")
    else:
        print("✗ Failed to send weather command")
        sys.exit(1)

    # Close device
    device.close()


if __name__ == "__main__":
    main()
