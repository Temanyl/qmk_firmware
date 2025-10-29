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


def find_keyboard_device(silent=False):
    """Find the keyboard HID device. Set silent=True to suppress output."""
    if not silent:
        print(f"Looking for keyboard (VID: 0x{VENDOR_ID:04X}, PID: 0x{PRODUCT_ID:04X})...")

    # List all HID devices
    devices = hid.enumerate()

    # Find our keyboard by VID/PID and usage page
    for device_info in devices:
        if (device_info['vendor_id'] == VENDOR_ID and
            device_info['product_id'] == PRODUCT_ID and
            device_info['usage_page'] == USAGE_PAGE and
            device_info['usage'] == USAGE):

            if not silent:
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
        bytes_written = device.write([0] + list(packet))

        # Check if write was successful
        if bytes_written <= 0:
            print(f"✗ Write failed: {bytes_written} bytes written")
            return False

        return True
    except Exception as e:
        print(f"✗ Error sending HID packet: {e}")
        return False


def is_keyboard_connected():
    """Check if keyboard is still in HID device list."""
    devices = hid.enumerate()
    for device_info in devices:
        if (device_info['vendor_id'] == VENDOR_ID and
            device_info['product_id'] == PRODUCT_ID and
            device_info['usage_page'] == USAGE_PAGE and
            device_info['usage'] == USAGE):
            return True
    return False


def connect_to_keyboard(silent=False):
    """Try to connect to the keyboard. Returns device handle or None."""
    device_path = find_keyboard_device(silent=silent)
    if not device_path:
        return None

    try:
        device = hid.device()
        device.open_path(device_path)
        return device
    except Exception as e:
        if not silent:
            print(f"Error opening HID device: {e}")
        return None


def main():
    """Main loop: monitor volume and send updates to keyboard."""
    print("QMK Volume Monitor")
    print("=" * 50)
    print("Waiting for keyboard... (Press Ctrl+C to quit)\n")

    device = None
    last_volume = None
    reconnect_delay = 2.0  # Wait 2 seconds between reconnection attempts
    last_connect_attempt = 0
    first_connection = True  # Track if this is the first connection
    connection_check_interval = 1.0  # Check connection every second
    last_connection_check = 0

    try:
        while True:
            current_time = time.time()

            # Try to connect/reconnect if not connected
            if device is None:
                # Only attempt reconnect every reconnect_delay seconds
                if current_time - last_connect_attempt >= reconnect_delay:
                    last_connect_attempt = current_time
                    # Be quiet during reconnection attempts, verbose on first connect
                    device = connect_to_keyboard(silent=not first_connection)

                    if device is not None:
                        if first_connection:
                            print(f"✓ Connected to keyboard!")
                            first_connection = False
                        else:
                            print(f"✓ Keyboard reconnected!")
                        print("Monitoring system volume...\n")

                        # Reset connection check timer to check immediately
                        last_connection_check = current_time

                        # Immediately send current volume on (re)connect
                        current_volume = get_system_volume()
                        if current_volume is not None:
                            print(f"Syncing volume: {current_volume}%")
                            if send_volume_update(device, current_volume):
                                last_volume = current_volume
                            else:
                                # Send failed immediately after connect
                                print("✗ Initial sync failed")
                                print("Waiting for keyboard to reconnect...\n")
                                try:
                                    device.close()
                                except:
                                    pass
                                device = None
                                last_volume = None
                                continue
                        else:
                            # Reset last_volume to force update on next successful read
                            last_volume = None

                # Wait before next iteration
                time.sleep(0.5)
                continue

            # We're connected, periodically check if device is still there
            if current_time - last_connection_check >= connection_check_interval:
                last_connection_check = current_time
                if not is_keyboard_connected():
                    print("✗ Keyboard disconnected")
                    print("Waiting for keyboard to reconnect...\n")
                    try:
                        device.close()
                    except:
                        pass
                    device = None
                    last_volume = None
                    continue

            # We're connected, try to get and send volume
            try:
                current_volume = get_system_volume()

                if current_volume is not None:
                    # Only send update if volume changed
                    if current_volume != last_volume:
                        print(f"Volume changed: {current_volume}%")
                        if send_volume_update(device, current_volume):
                            last_volume = current_volume
                        else:
                            # Send failed, likely disconnected
                            print("✗ Send failed, keyboard may be disconnected")
                            print("Waiting for keyboard to reconnect...\n")
                            try:
                                device.close()
                            except:
                                pass
                            device = None
                            last_volume = None
                            continue

                # Wait before next poll
                time.sleep(POLL_INTERVAL)

            except Exception as e:
                # Any error during communication, assume disconnected
                print(f"✗ Connection error: {e}")
                print("Waiting for keyboard to reconnect...\n")
                try:
                    device.close()
                except:
                    pass
                device = None
                last_volume = None

    except KeyboardInterrupt:
        print("\n\nStopping volume monitor...")
    finally:
        if device is not None:
            try:
                device.close()
            except:
                pass


if __name__ == "__main__":
    main()
