#!/usr/bin/env python3
"""
Weather Change Simulator for QMK Keyboard Testing
Sends weather updates via Raw HID to test weather transitions on the keyboard display.

Usage:
    python3 test_weather.py                    # Interactive mode
    python3 test_weather.py sunny              # Send sunny
    python3 test_weather.py rain               # Send rain
    python3 test_weather.py snow               # Send snow
    python3 test_weather.py cycle              # Cycle through all weather states
    python3 test_weather.py cycle --interval 5 # Cycle with 5 second intervals
"""

import time
import sys
import argparse

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

# USB Vendor and Product IDs (same as keyboard_monitor.py)
VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000

# Raw HID usage page and usage
USAGE_PAGE = 0xFF60
USAGE = 0x0061

# HID packet size
HID_PACKET_SIZE = 32

# Command ID for weather updates
CMD_WEATHER_UPDATE = 0x04

# Weather state constants (match keyboard firmware)
WEATHER_SUNNY = 0
WEATHER_RAIN = 1
WEATHER_SNOW = 2

WEATHER_NAMES = {
    WEATHER_SUNNY: "Sunny ☀️",
    WEATHER_RAIN: "Rain 🌧️",
    WEATHER_SNOW: "Snow ❄️"
}

WEATHER_MAP = {
    "sunny": WEATHER_SUNNY,
    "rain": WEATHER_RAIN,
    "snow": WEATHER_SNOW,
}


def find_keyboard_device():
    """Find the keyboard HID device."""
    print(f"🔍 Looking for keyboard (VID: 0x{VENDOR_ID:04X}, PID: 0x{PRODUCT_ID:04X})...")

    devices = hid.enumerate()

    for device_info in devices:
        if (device_info['vendor_id'] == VENDOR_ID and
            device_info['product_id'] == PRODUCT_ID and
            device_info['usage_page'] == USAGE_PAGE and
            device_info['usage'] == USAGE):
            print(f"✓ Found keyboard: {device_info['product_string']}")
            return device_info['path']

    return None


def send_weather_update(device, weather_state):
    """Send weather update to keyboard via Raw HID."""
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_WEATHER_UPDATE
    packet[1] = weather_state

    try:
        bytes_written = device.write([0] + list(packet))

        if bytes_written <= 0:
            print(f"✗ Write failed: {bytes_written} bytes written")
            return False

        return True
    except Exception as e:
        print(f"✗ Error sending weather packet: {e}")
        return False


def interactive_mode(device):
    """Interactive mode: user selects weather states."""
    print("\n" + "=" * 60)
    print("             WEATHER CHANGE SIMULATOR")
    print("=" * 60)
    print("\nCommands:")
    print("  1 or sunny - Send sunny weather")
    print("  2 or rain  - Send rain weather")
    print("  3 or snow  - Send snow weather")
    print("  c or cycle - Cycle through all weather states")
    print("  q or quit  - Exit")
    print("\n" + "=" * 60 + "\n")

    while True:
        try:
            choice = input("Enter command: ").strip().lower()

            if choice in ['q', 'quit', 'exit']:
                print("\n👋 Exiting weather simulator")
                break

            elif choice in ['c', 'cycle']:
                print("\n🔄 Cycling through weather states (Ctrl+C to stop)...")
                try:
                    interval = 3  # Default 3 seconds
                    while True:
                        for weather_state in [WEATHER_SUNNY, WEATHER_RAIN, WEATHER_SNOW]:
                            print(f"\n➜ Sending: {WEATHER_NAMES[weather_state]}")
                            if send_weather_update(device, weather_state):
                                print("  ✓ Sent successfully")
                            else:
                                print("  ✗ Send failed")
                            time.sleep(interval)
                except KeyboardInterrupt:
                    print("\n⏸️  Cycle stopped")

            elif choice in ['1', 'sunny']:
                print(f"\n➜ Sending: {WEATHER_NAMES[WEATHER_SUNNY]}")
                if send_weather_update(device, WEATHER_SUNNY):
                    print("  ✓ Sent successfully")
                else:
                    print("  ✗ Send failed")

            elif choice in ['2', 'rain']:
                print(f"\n➜ Sending: {WEATHER_NAMES[WEATHER_RAIN]}")
                if send_weather_update(device, WEATHER_RAIN):
                    print("  ✓ Sent successfully")
                else:
                    print("  ✗ Send failed")

            elif choice in ['3', 'snow']:
                print(f"\n➜ Sending: {WEATHER_NAMES[WEATHER_SNOW]}")
                if send_weather_update(device, WEATHER_SNOW):
                    print("  ✓ Sent successfully")
                else:
                    print("  ✗ Send failed")

            else:
                print("❌ Invalid command. Try 'sunny', 'rain', 'snow', 'cycle', or 'quit'")

        except KeyboardInterrupt:
            print("\n\n👋 Exiting weather simulator")
            break
        except Exception as e:
            print(f"❌ Error: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Weather Change Simulator for QMK Keyboard Testing",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 test_weather.py                    # Interactive mode
  python3 test_weather.py sunny              # Send sunny
  python3 test_weather.py rain               # Send rain
  python3 test_weather.py snow               # Send snow
  python3 test_weather.py cycle              # Cycle through all states (3s intervals)
  python3 test_weather.py cycle --interval 5 # Cycle with 5 second intervals
        """
    )
    parser.add_argument(
        'weather',
        nargs='?',
        choices=['sunny', 'rain', 'snow', 'cycle'],
        help='Weather state to send (omit for interactive mode)'
    )
    parser.add_argument(
        '--interval',
        type=float,
        default=3.0,
        metavar='SECONDS',
        help='Interval between cycles in seconds (default: 3.0)'
    )

    args = parser.parse_args()

    # Find keyboard
    device_path = find_keyboard_device()
    if not device_path:
        print("✗ Keyboard not found!")
        print("\nMake sure:")
        print("  1. Keyboard is connected")
        print("  2. VID/PID match your keyboard (check keyboard_monitor.py)")
        sys.exit(1)

    # Connect to keyboard
    try:
        device = hid.device()
        device.open_path(device_path)
        print("✓ Connected to keyboard\n")
    except Exception as e:
        print(f"✗ Error opening HID device: {e}")
        sys.exit(1)

    try:
        # Command-line mode
        if args.weather:
            if args.weather == 'cycle':
                print(f"🔄 Cycling through weather states ({args.interval}s intervals)")
                print("   Press Ctrl+C to stop\n")
                try:
                    while True:
                        for weather_state in [WEATHER_SUNNY, WEATHER_RAIN, WEATHER_SNOW]:
                            print(f"➜ Sending: {WEATHER_NAMES[weather_state]}")
                            if send_weather_update(device, weather_state):
                                print("  ✓ Sent successfully\n")
                            else:
                                print("  ✗ Send failed\n")
                            time.sleep(args.interval)
                except KeyboardInterrupt:
                    print("\n👋 Cycle stopped")
            else:
                weather_state = WEATHER_MAP[args.weather]
                print(f"➜ Sending: {WEATHER_NAMES[weather_state]}")
                if send_weather_update(device, weather_state):
                    print("✓ Sent successfully")
                else:
                    print("✗ Send failed")
                    sys.exit(1)
        else:
            # Interactive mode
            interactive_mode(device)

    finally:
        try:
            device.close()
        except:
            pass


if __name__ == "__main__":
    main()
