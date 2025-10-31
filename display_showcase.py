#!/usr/bin/env python3
"""
Display Showcase for QMK Keyboards with Raw HID
Showcases all the cool features of the chocmanyl36 display.

This script demonstrates:
- All four seasons with transitions
- Sun movement during a summer day
- Moon movement and phases during a winter night
- All special events (Halloween, Christmas, New Year's)
- Media title area for subtitles

Requirements:
    - hidapi

Install dependencies:
    pip3 install hidapi

Usage:
    python3 display_showcase.py
"""

import time
import sys
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

# Showcase segments
SEGMENT_DURATION = 10.0  # 10 seconds per segment


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

            print(f"✓ Found keyboard: {device_info['product_string']}")
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
        print("✓ Connected to keyboard!\n")
        return device
    except Exception as e:
        print(f"✗ Error opening HID device: {e}")
        return None


def send_datetime_update(device, dt):
    """Send date/time update to keyboard via Raw HID."""
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_DATETIME_UPDATE  # Command ID

    # Pack date/time: year (2 bytes), month, day, hour, minute, second
    packet[1] = dt.year & 0xFF  # Year low byte
    packet[2] = (dt.year >> 8) & 0xFF  # Year high byte
    packet[3] = dt.month
    packet[4] = dt.day
    packet[5] = dt.hour
    packet[6] = dt.minute
    packet[7] = dt.second

    try:
        bytes_written = device.write([0] + list(packet))
        return bytes_written > 0
    except Exception as e:
        print(f"\n✗ Error sending datetime packet: {e}")
        return False


def send_media_update(device, text):
    """Send media text update to keyboard via Raw HID."""
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_MEDIA_UPDATE  # Command ID

    # Convert text to bytes and pack as null-terminated string starting at byte 1
    # Max 31 bytes for text (packet is 32 bytes, 1 for command ID, rest for text)
    text_bytes = text.encode('utf-8')[:31]
    packet[1:1+len(text_bytes)] = text_bytes
    # Ensure null termination (already zeroed by bytearray initialization)

    try:
        bytes_written = device.write([0] + list(packet))
        return bytes_written > 0
    except Exception as e:
        print(f"\n✗ Error sending media packet: {e}")
        return False


def showcase_segment(device, title, dt, emoji="🎬"):
    """Show a single showcase segment with subtitle."""
    print(f"{emoji}  {title}")

    if not send_media_update(device, title):
        return False

    if not send_datetime_update(device, dt):
        return False

    time.sleep(0.5)  # Brief pause for display to update
    return True


def showcase_seasons(device):
    """Showcase all four seasons."""
    print("\n" + "="*60)
    print("🌍  SHOWCASING ALL SEASONS")
    print("="*60)

    seasons = [
        ("Winter Wonderland - Snowy Landscape", datetime(2025, 1, 15, 14, 0, 0), "❄️"),
        ("Spring Awakening - Cherry Blossoms", datetime(2025, 4, 15, 14, 0, 0), "🌸"),
        ("Summer Breeze - Sunny Paradise", datetime(2025, 7, 15, 14, 0, 0), "☀️"),
        ("Autumn Rain - Falling Leaves", datetime(2025, 10, 15, 14, 0, 0), "🍂"),
    ]

    for season_name, dt, emoji in seasons:
        if not showcase_segment(device, season_name, dt, emoji):
            return False
        time.sleep(SEGMENT_DURATION)

    return True


def showcase_sun_movement(device):
    """Showcase sun movement during a summer day."""
    print("\n" + "="*60)
    print("☀️  SHOWCASING SUN MOVEMENT (Summer Day)")
    print("="*60)

    # Show summer day from sunrise to sunset (6 AM to 8 PM)
    start_hour = 6
    end_hour = 20
    hours = end_hour - start_hour
    time_per_hour = SEGMENT_DURATION / hours

    # Set media text once for the entire sun movement cycle
    media_title = "Journey of the Sun - Dawn to Dusk"
    if not send_media_update(device, media_title):
        return False

    print(f"☀️  {media_title}")

    for hour in range(start_hour, end_hour + 1):
        dt = datetime(2025, 7, 15, hour, 0, 0)

        # Send datetime update only (no media update)
        if not send_datetime_update(device, dt):
            return False

        time.sleep(time_per_hour)

    return True


def showcase_moon_movement(device):
    """Showcase moon movement and phases during winter nights."""
    print("\n" + "="*60)
    print("🌙  SHOWCASING MOON MOVEMENT & PHASES")
    print("="*60)

    # Show different moon phases across winter nights
    moon_phases = [
        (1, 22),   # New Moon
        (5, 22),   # Waxing Crescent
        (8, 22),   # First Quarter
        (11, 22),  # Waxing Gibbous
        (15, 22),  # Full Moon
        (19, 22),  # Waning Gibbous
        (22, 22),  # Last Quarter
        (27, 22),  # Waning Crescent
    ]

    # Set media text once for the entire moon cycle
    media_title = "Lunar Phases - Winter Moonlight"
    if not send_media_update(device, media_title):
        return False

    print(f"🌙  {media_title}")

    time_per_phase = SEGMENT_DURATION / len(moon_phases)

    for day, hour in moon_phases:
        dt = datetime(2025, 1, day, hour, 0, 0)

        # Send datetime update only (no media update)
        if not send_datetime_update(device, dt):
            return False

        time.sleep(time_per_phase)

    return True


def showcase_halloween(device):
    """Showcase Halloween special event."""
    print("\n" + "="*60)
    print("🎃  SHOWCASING HALLOWEEN EVENT")
    print("="*60)

    halloween_scenes = [
        ("Spooky Season - Jack-o'-Lanterns", datetime(2025, 10, 31, 20, 0, 0)),
        ("Haunted Night - Dancing Ghosts", datetime(2025, 10, 31, 21, 0, 0)),
        ("Midnight Horror - Ghostly Flight", datetime(2025, 10, 31, 23, 0, 0)),
    ]

    time_per_scene = SEGMENT_DURATION / len(halloween_scenes)

    for title, dt in halloween_scenes:
        if not showcase_segment(device, title, dt, "🎃"):
            return False
        time.sleep(time_per_scene)

    return True


def showcase_christmas(device):
    """Showcase Christmas special event with advent calendar."""
    print("\n" + "="*60)
    print("🎄  SHOWCASING CHRISTMAS EVENT")
    print("="*60)

    christmas_scenes = [
        ("Advent Calendar - First Gift", datetime(2025, 12, 1, 14, 0, 0)),
        ("Week One - Candy Canes & Bells", datetime(2025, 12, 7, 14, 0, 0)),
        ("Halfway There - Tree Decorations", datetime(2025, 12, 14, 14, 0, 0)),
        ("Christmas Eve - All 24 Gifts", datetime(2025, 12, 24, 14, 0, 0)),
        ("Jingle Bells - Santa's Flight", datetime(2025, 12, 25, 14, 0, 0)),
    ]

    time_per_scene = SEGMENT_DURATION / len(christmas_scenes)

    for title, dt in christmas_scenes:
        if not showcase_segment(device, title, dt, "🎄"):
            return False
        time.sleep(time_per_scene)

    return True


def showcase_new_years(device):
    """Showcase New Year's Eve fireworks."""
    print("\n" + "="*60)
    print("🎆  SHOWCASING NEW YEAR'S EVE")
    print("="*60)

    title = "Happy New Year - Fireworks Show!"
    dt = datetime(2025, 12, 31, 23, 59, 0)

    if not showcase_segment(device, title, dt, "🎆"):
        return False

    time.sleep(SEGMENT_DURATION)
    return True


def showcase_weather_effects(device):
    """Showcase seasonal weather effects."""
    print("\n" + "="*60)
    print("🌦️  SHOWCASING WEATHER EFFECTS")
    print("="*60)

    weather_scenes = [
        ("White Christmas - Gentle Snowfall", datetime(2025, 1, 15, 14, 0, 0)),
        ("Spring Garden - Butterflies Dance", datetime(2025, 4, 15, 14, 0, 0)),
        ("Golden Fields - Sunflower Dreams", datetime(2025, 7, 15, 14, 0, 0)),
        ("November Rain - Autumn Showers", datetime(2025, 10, 15, 14, 0, 0)),
    ]

    time_per_scene = SEGMENT_DURATION / len(weather_scenes)

    for title, dt in weather_scenes:
        if not showcase_segment(device, title, dt, "🌦️"):
            return False
        time.sleep(time_per_scene)

    return True


def main():
    """Main entry point."""
    print("QMK Display Showcase")
    print("=" * 60)
    print("\nThis showcase will demonstrate all display features:")
    print("  • All four seasons (Winter, Spring, Summer, Fall)")
    print("  • Sun movement during a summer day")
    print("  • Moon phases and movement during winter nights")
    print("  • Halloween event with pumpkins and ghosts")
    print("  • Christmas advent calendar with Santa")
    print("  • New Year's Eve fireworks")
    print("  • Seasonal weather effects")
    print()
    total_time = SEGMENT_DURATION * 11  # Approximate total time
    print(f"Total showcase duration: ~{total_time:.0f} seconds ({total_time/60:.1f} minutes)")
    print()

    # Connect to keyboard
    device = connect_to_keyboard()
    if not device:
        print("\n✗ Keyboard not found!")
        print("Make sure your keyboard is connected and the VID/PID are correct.")
        return 1

    try:
        showcase_start = time.time()

        # Run all showcase segments
        showcases = [
            showcase_seasons,
            showcase_sun_movement,
            showcase_moon_movement,
            showcase_halloween,
            showcase_christmas,
            showcase_new_years,
            showcase_weather_effects,
        ]

        for showcase_func in showcases:
            if not showcase_func(device):
                print("\n✗ Showcase failed or interrupted")
                return 1

            # Brief pause between showcases
            time.sleep(1.0)

        # Show completion
        showcase_duration = time.time() - showcase_start
        print("\n" + "=" * 60)
        print(f"✓ Showcase complete! (Duration: {showcase_duration:.1f}s)")
        print("=" * 60)

        # Return to current time with completion message
        current_dt = datetime.now()
        send_media_update(device, "Display Showcase - Complete!")
        send_datetime_update(device, current_dt)

        # Wait a moment before clearing
        time.sleep(2.0)

        # Clear media text
        send_media_update(device, "")

        return 0

    except KeyboardInterrupt:
        print("\n\n⏹️  Showcase interrupted by user")
        return 130
    finally:
        if device:
            try:
                device.close()
                print("\n✓ Disconnected from keyboard")
            except:
                pass


if __name__ == "__main__":
    sys.exit(main())
