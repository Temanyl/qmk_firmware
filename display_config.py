#!/usr/bin/env python3
"""
Display Configuration Tool for QMK Keyboards with Raw HID
Configure display settings: date/time, weather, and media text.

All parameters are optional - only send what you specify.

Requirements:
    - hidapi

Install dependencies:
    pip3 install hidapi

Usage Examples:
    # Set date only (April 10, 2025)
    python3 display_config.py --date 2025-04-10

    # Set date and time (Halloween night)
    python3 display_config.py --date 2025-10-31 --time 23:30

    # Set weather only (snow)
    python3 display_config.py --weather snow

    # Set weather with intensity
    python3 display_config.py --weather rain-heavy

    # Set media text only
    python3 display_config.py --text "Now Playing: Bohemian Rhapsody"

    # Set everything at once
    python3 display_config.py --date 2025-12-25 --time 14:00 --weather snow-medium --text "White Christmas!"

    # Clear media text
    python3 display_config.py --text ""

    # List available weather options
    python3 display_config.py --list-weather
"""

import sys
import argparse
from datetime import datetime

# Import hidapi
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

# USB Vendor and Product IDs
VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000

# Raw HID usage page and usage
USAGE_PAGE = 0xFF60
USAGE = 0x0061

# HID packet size
HID_PACKET_SIZE = 32

# Command IDs
CMD_DATETIME_UPDATE = 0x03
CMD_MEDIA_UPDATE = 0x02
CMD_WEATHER_UPDATE = 0x04

# Weather states mapping
WEATHER_STATES = {
    'sunny': 0,
    'rain-light': 1,
    'rain': 2,
    'rain-medium': 2,
    'rain-heavy': 3,
    'snow-light': 4,
    'snow': 5,
    'snow-medium': 5,
    'snow-heavy': 6,
    'cloudy': 7,
    'partly-cloudy': 7,
    'overcast': 8,
}

WEATHER_NAMES = {
    0: "Sunny",
    1: "Light Rain",
    2: "Rain (Medium)",
    3: "Heavy Rain",
    4: "Light Snow",
    5: "Snow (Medium)",
    6: "Heavy Snow",
    7: "Partly Cloudy",
    8: "Overcast"
}


def find_keyboard_device():
    """Find the keyboard HID device."""
    devices = hid.enumerate()

    for device_info in devices:
        if (device_info['vendor_id'] == VENDOR_ID and
            device_info['product_id'] == PRODUCT_ID and
            device_info['usage_page'] == USAGE_PAGE and
            device_info['usage'] == USAGE):
            return device_info['path']

    return None


def connect_to_keyboard():
    """Try to connect to the keyboard. Returns device handle or None."""
    device_path = find_keyboard_device()
    if not device_path:
        return None

    try:
        device = hid.device()
        device.open_path(device_path)
        return device
    except Exception as e:
        print(f"✗ Error opening HID device: {e}")
        return None


def send_datetime_update(device, dt):
    """Send date/time update to keyboard via Raw HID."""
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_DATETIME_UPDATE
    packet[1] = dt.year & 0xFF
    packet[2] = (dt.year >> 8) & 0xFF
    packet[3] = dt.month
    packet[4] = dt.day
    packet[5] = dt.hour
    packet[6] = dt.minute
    packet[7] = dt.second

    try:
        bytes_written = device.write([0] + list(packet))
        if bytes_written > 0:
            print(f"✓ Date/Time set: {dt.strftime('%Y-%m-%d %H:%M:%S')}")
            return True
        else:
            print("✗ Failed to send date/time")
            return False
    except Exception as e:
        print(f"✗ Error sending date/time: {e}")
        return False


def send_media_update(device, text):
    """Send media text update to keyboard via Raw HID."""
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_MEDIA_UPDATE
    text_bytes = text.encode('utf-8')[:31]
    packet[1:1+len(text_bytes)] = text_bytes

    try:
        bytes_written = device.write([0] + list(packet))
        if bytes_written > 0:
            if text:
                print(f"✓ Media text set: \"{text}\"")
            else:
                print("✓ Media text cleared")
            return True
        else:
            print("✗ Failed to send media text")
            return False
    except Exception as e:
        print(f"✗ Error sending media text: {e}")
        return False


def send_weather_update(device, weather_state):
    """Send weather update to keyboard via Raw HID."""
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_WEATHER_UPDATE
    packet[1] = weather_state

    try:
        bytes_written = device.write([0] + list(packet))
        if bytes_written > 0:
            print(f"✓ Weather set: {WEATHER_NAMES[weather_state]}")
            return True
        else:
            print("✗ Failed to send weather")
            return False
    except Exception as e:
        print(f"✗ Error sending weather: {e}")
        return False


def parse_date(date_str):
    """Parse date string in format YYYY-MM-DD."""
    try:
        return datetime.strptime(date_str, "%Y-%m-%d")
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"Invalid date format: {date_str}. Use YYYY-MM-DD (e.g., 2025-10-31)"
        )


def parse_time(time_str):
    """Parse time string in format HH:MM or HH:MM:SS."""
    try:
        # Try with seconds first
        try:
            parsed = datetime.strptime(time_str, "%H:%M:%S")
            return parsed.hour, parsed.minute, parsed.second
        except ValueError:
            # Try without seconds
            parsed = datetime.strptime(time_str, "%H:%M")
            return parsed.hour, parsed.minute, 0
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"Invalid time format: {time_str}. Use HH:MM or HH:MM:SS (e.g., 23:30)"
        )


def parse_weather(weather_str):
    """Parse weather string and return weather state."""
    weather_lower = weather_str.lower()
    if weather_lower not in WEATHER_STATES:
        raise argparse.ArgumentTypeError(
            f"Invalid weather: {weather_str}. Use --list-weather to see available options."
        )
    return WEATHER_STATES[weather_lower]


def list_weather_options():
    """Print all available weather options."""
    print("\nAvailable weather options:")
    print("=" * 50)
    print("\nSunny:")
    print("  sunny")
    print("\nRain (with intensity):")
    print("  rain-light")
    print("  rain, rain-medium      (default rain)")
    print("  rain-heavy")
    print("\nSnow (with intensity):")
    print("  snow-light")
    print("  snow, snow-medium      (default snow)")
    print("  snow-heavy")
    print("\nCloudy:")
    print("  cloudy, partly-cloudy  (2 clouds)")
    print("  overcast               (5 clouds)")
    print("\nExamples:")
    print("  python3 display_config.py --weather sunny")
    print("  python3 display_config.py --weather rain-heavy")
    print("  python3 display_config.py --weather snow-light")
    print()


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Configure QMK keyboard display: date/time, weather, and media text",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Set date only (April 10, 2025 - Easter)
  python3 display_config.py --date 2025-04-10

  # Set date and time (Halloween night)
  python3 display_config.py --date 2025-10-31 --time 23:30

  # Set weather only (snowy)
  python3 display_config.py --weather snow

  # Set weather with intensity
  python3 display_config.py --weather rain-heavy

  # Set media text only
  python3 display_config.py --text "Now Playing: Spooky Scary Skeletons"

  # Set everything at once (White Christmas!)
  python3 display_config.py --date 2025-12-25 --time 14:00 --weather snow-medium --text "White Christmas!"

  # Clear media text
  python3 display_config.py --text ""

  # List available weather options
  python3 display_config.py --list-weather

Note: All parameters are optional. Only specified values will be sent to the keyboard.
        """
    )

    parser.add_argument(
        '--date',
        type=parse_date,
        metavar='YYYY-MM-DD',
        help='Set date (format: YYYY-MM-DD, e.g., 2025-10-31)'
    )
    parser.add_argument(
        '--time',
        type=parse_time,
        metavar='HH:MM[:SS]',
        help='Set time (format: HH:MM or HH:MM:SS, e.g., 23:30)'
    )
    parser.add_argument(
        '--weather',
        type=parse_weather,
        metavar='WEATHER',
        help='Set weather condition (use --list-weather for options)'
    )
    parser.add_argument(
        '--text',
        type=str,
        metavar='TEXT',
        help='Set media text (max 31 chars, use "" to clear)'
    )
    parser.add_argument(
        '--list-weather',
        action='store_true',
        help='List all available weather options and exit'
    )

    args = parser.parse_args()

    # Handle --list-weather
    if args.list_weather:
        list_weather_options()
        return 0

    # Check if any config option was provided
    if not any([args.date, args.time, args.weather, args.text is not None]):
        parser.print_help()
        print("\n✗ Error: No configuration options provided")
        print("Use at least one of: --date, --time, --weather, --text")
        print("Or use --list-weather to see weather options")
        return 1

    # Connect to keyboard
    print("🔍 Looking for keyboard...")
    device = connect_to_keyboard()
    if not device:
        print("\n✗ Keyboard not found!")
        print("Make sure your keyboard is connected and the VID/PID are correct.")
        return 1

    print("✓ Connected to keyboard!\n")

    try:
        success = True

        # Handle date/time update
        if args.date or args.time:
            # Start with provided date or current date
            if args.date:
                dt = args.date
            else:
                dt = datetime.now()

            # Override time if provided
            if args.time:
                hour, minute, second = args.time
                dt = dt.replace(hour=hour, minute=minute, second=second)

            if not send_datetime_update(device, dt):
                success = False

        # Handle weather update
        if args.weather is not None:
            if not send_weather_update(device, args.weather):
                success = False

        # Handle media text update
        if args.text is not None:
            if not send_media_update(device, args.text):
                success = False

        if success:
            print("\n✓ Configuration applied successfully!")
            return 0
        else:
            print("\n⚠ Some configuration updates failed")
            return 1

    except KeyboardInterrupt:
        print("\n\n⏹️  Configuration interrupted by user")
        return 130
    finally:
        if device:
            try:
                device.close()
            except:
                pass


if __name__ == "__main__":
    sys.exit(main())
