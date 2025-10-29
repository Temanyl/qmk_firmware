#!/usr/bin/env python3
"""
Volume Monitor for QMK Keyboards with Raw HID
Monitors system volume and sends updates to keyboard display via Raw HID.

Requirements:
    - macOS: osascript (built-in)
    - Windows: pycaw, comtypes
    - Linux: pulsectl or amixer
    - All: hidapi

Install dependencies:
    pip3 install hidapi pycaw comtypes pulsectl

Note: If you get import errors, uninstall conflicting packages:
    pip3 uninstall hid hidapi
    pip3 install hidapi
"""

import time
import subprocess
import platform
import sys

# Import hidapi - handle potential import issues
try:
    import hid
except ImportError:
    print("Error: hidapi not installed!")
    print("Install with: pip3 install hidapi")
    sys.exit(1)

# Verify we have the right hidapi module
if not hasattr(hid, 'device'):
    print("Error: Wrong 'hid' module detected!")
    print("\nFix this by:")
    print("  1. Uninstall conflicting packages:")
    print("     pip3 uninstall hid hidapi")
    print("  2. Install the correct package:")
    print("     pip3 install hidapi")
    sys.exit(1)

# USB Vendor and Product IDs for your keyboard
# Update these to match your keyboard's VID/PID from keyboard.json
VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000

# Raw HID usage page and usage (standard for QMK)
USAGE_PAGE = 0xFF60
USAGE = 0x0061

# HID packet size (32 bytes for Raw HID)
HID_PACKET_SIZE = 32

# Command IDs for the protocol
CMD_VOLUME_UPDATE = 0x01

# Poll interval in seconds
POLL_INTERVAL = 0.1


def get_volume_macos():
    """Get system volume on macOS (0-100)."""
    try:
        result = subprocess.run(
            ['osascript', '-e', 'output volume of (get volume settings)'],
            capture_output=True,
            text=True,
            check=True
        )
        volume = int(result.stdout.strip())
        return volume
    except Exception as e:
        print(f"Error getting macOS volume: {e}")
        return None


def get_volume_windows():
    """Get system volume on Windows (0-100)."""
    try:
        from ctypes import cast, POINTER
        from comtypes import CLSCTX_ALL
        from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume

        devices = AudioUtilities.GetSpeakers()
        interface = devices.Activate(
            IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
        volume_interface = cast(interface, POINTER(IAudioEndpointVolume))

        # Get volume as percentage (0.0 to 1.0)
        current_volume = volume_interface.GetMasterVolumeLevelScalar()
        return int(current_volume * 100)
    except Exception as e:
        print(f"Error getting Windows volume: {e}")
        return None


def get_volume_linux():
    """Get system volume on Linux (0-100)."""
    try:
        # Try using pactl (PulseAudio)
        result = subprocess.run(
            ['pactl', 'get-sink-volume', '@DEFAULT_SINK@'],
            capture_output=True,
            text=True,
            check=True
        )
        # Parse output like "Volume: front-left: 65536 /  100% / 0.00 dB"
        for line in result.stdout.split('\n'):
            if 'Volume:' in line:
                # Extract first percentage value
                parts = line.split('%')[0].split()
                volume = int(parts[-1])
                return volume
    except FileNotFoundError:
        # Try using amixer as fallback
        try:
            result = subprocess.run(
                ['amixer', 'get', 'Master'],
                capture_output=True,
                text=True,
                check=True
            )
            # Parse output like "[65%]"
            for line in result.stdout.split('\n'):
                if '[' in line and '%' in line:
                    start = line.index('[') + 1
                    end = line.index('%')
                    volume = int(line[start:end])
                    return volume
        except Exception as e:
            print(f"Error getting Linux volume with amixer: {e}")
            return None
    except Exception as e:
        print(f"Error getting Linux volume with pactl: {e}")
        return None


def get_system_volume():
    """Get system volume based on the current platform (0-100)."""
    system = platform.system()

    if system == "Darwin":  # macOS
        return get_volume_macos()
    elif system == "Windows":
        return get_volume_windows()
    elif system == "Linux":
        return get_volume_linux()
    else:
        print(f"Unsupported platform: {system}")
        return None


def find_keyboard_device():
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

            print(f"Found keyboard: {device_info['product_string']}")
            return device_info['path']

    return None


def send_volume_update(device, volume):
    """Send volume update to keyboard via Raw HID."""
    # Create HID packet (32 bytes)
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_VOLUME_UPDATE  # Command ID
    packet[1] = volume             # Volume level (0-100)

    try:
        # Send the packet (first byte is report ID, usually 0)
        device.write([0] + list(packet))
        return True
    except Exception as e:
        print(f"Error sending HID packet: {e}")
        return False


def main():
    """Main loop: monitor volume and send updates to keyboard."""
    print("QMK Volume Monitor")
    print("=" * 50)

    # Find the keyboard
    device_path = find_keyboard_device()
    if not device_path:
        print("Error: Keyboard not found!")
        print("\nMake sure:")
        print("  1. Your keyboard is connected")
        print("  2. The firmware with Raw HID is flashed")
        print("  3. VID/PID in this script match your keyboard.json")
        print("\nAvailable HID devices:")
        for dev in hid.enumerate():
            print(f"  VID: 0x{dev['vendor_id']:04X} PID: 0x{dev['product_id']:04X} "
                  f"Usage Page: 0x{dev['usage_page']:04X} Usage: 0x{dev['usage']:04X} "
                  f"- {dev['product_string']}")
        sys.exit(1)

    # Open the HID device
    try:
        device = hid.device()
        device.open_path(device_path)
        print(f"Connected to keyboard!")
    except Exception as e:
        print(f"Error opening HID device: {e}")
        sys.exit(1)

    print("\nMonitoring system volume... (Press Ctrl+C to quit)")

    last_volume = None

    try:
        while True:
            # Get current system volume
            current_volume = get_system_volume()

            if current_volume is not None:
                # Only send update if volume changed
                if current_volume != last_volume:
                    print(f"Volume changed: {current_volume}%")
                    if send_volume_update(device, current_volume):
                        last_volume = current_volume

            # Wait before next poll
            time.sleep(POLL_INTERVAL)

    except KeyboardInterrupt:
        print("\nStopping volume monitor...")
    finally:
        device.close()


if __name__ == "__main__":
    main()
