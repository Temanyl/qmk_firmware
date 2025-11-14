#!/usr/bin/env python3
"""
Comprehensive Display Showcase for QMK Keyboards with Raw HID
Demonstrates ALL features and combinations of the chocmanyl36 display.

This script showcases:
- All four seasons (Winter, Spring, Summer, Fall)
- All special events (Halloween, Christmas, Easter)
- All weather conditions (sunny, light/medium/heavy rain, light/medium/heavy snow, cloudy, overcast)
- Sun movement across different times of day
- Moon phases and movement
- All possible combinations (e.g., Halloween with snow, Halloween with sunshine, etc.)

Requirements:
    - hidapi

Install dependencies:
    pip3 install hidapi

Usage:
    python3 display_showcase_full.py
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
CMD_WEATHER_UPDATE = 0x04

# Weather states
WEATHER_SUNNY = 0
WEATHER_RAIN_LIGHT = 1
WEATHER_RAIN_MEDIUM = 2
WEATHER_RAIN_HEAVY = 3
WEATHER_SNOW_LIGHT = 4
WEATHER_SNOW_MEDIUM = 5
WEATHER_SNOW_HEAVY = 6
WEATHER_CLOUDY = 7
WEATHER_OVERCAST = 8

WEATHER_NAMES = {
    WEATHER_SUNNY: "Sunny",
    WEATHER_RAIN_LIGHT: "Light Rain",
    WEATHER_RAIN_MEDIUM: "Rain",
    WEATHER_RAIN_HEAVY: "Heavy Rain",
    WEATHER_SNOW_LIGHT: "Light Snow",
    WEATHER_SNOW_MEDIUM: "Snow",
    WEATHER_SNOW_HEAVY: "Heavy Snow",
    WEATHER_CLOUDY: "Partly Cloudy",
    WEATHER_OVERCAST: "Overcast"
}

# Showcase timing
SEGMENT_DURATION = 8.0  # 8 seconds per segment
QUICK_DURATION = 3.0    # 3 seconds for quick transitions


def find_keyboard_device():
    """Find the keyboard HID device."""
    # List all HID devices
    devices = hid.enumerate()

    # Find our keyboard by VID/PID and usage page
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
        return bytes_written > 0
    except Exception as e:
        print(f"\n✗ Error sending datetime: {e}")
        return False


def send_media_update(device, text):
    """Send media text update to keyboard via Raw HID."""
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_MEDIA_UPDATE
    text_bytes = text.encode('utf-8')[:31]
    packet[1:1+len(text_bytes)] = text_bytes

    try:
        bytes_written = device.write([0] + list(packet))
        return bytes_written > 0
    except Exception as e:
        print(f"\n✗ Error sending media: {e}")
        return False


def send_weather_update(device, weather_state):
    """Send weather update to keyboard via Raw HID."""
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_WEATHER_UPDATE
    packet[1] = weather_state

    try:
        bytes_written = device.write([0] + list(packet))
        return bytes_written > 0
    except Exception as e:
        print(f"\n✗ Error sending weather: {e}")
        return False


def showcase_segment(device, title, dt, weather=None, emoji="🎬", duration=SEGMENT_DURATION):
    """Show a single showcase segment."""
    weather_str = f" - {WEATHER_NAMES[weather]}" if weather is not None else ""
    print(f"{emoji}  {title}{weather_str}")

    if not send_media_update(device, title):
        return False

    if not send_datetime_update(device, dt):
        return False

    if weather is not None:
        if not send_weather_update(device, weather):
            return False

    time.sleep(0.3)  # Brief pause for display to update
    time.sleep(duration)
    return True


def showcase_seasons_all_weather(device):
    """Showcase all four seasons with all weather conditions."""
    print("\n" + "="*70)
    print("🌍  SHOWCASING ALL SEASONS WITH ALL WEATHER CONDITIONS")
    print("="*70)

    seasons = [
        ("Winter", datetime(2025, 1, 15, 14, 0, 0), "❄️", [
            WEATHER_SUNNY, WEATHER_CLOUDY, WEATHER_OVERCAST,
            WEATHER_SNOW_LIGHT, WEATHER_SNOW_MEDIUM, WEATHER_SNOW_HEAVY
        ]),
        ("Spring", datetime(2025, 4, 15, 14, 0, 0), "🌸", [
            WEATHER_SUNNY, WEATHER_CLOUDY, WEATHER_OVERCAST,
            WEATHER_RAIN_LIGHT, WEATHER_RAIN_MEDIUM, WEATHER_RAIN_HEAVY
        ]),
        ("Summer", datetime(2025, 7, 15, 14, 0, 0), "☀️", [
            WEATHER_SUNNY, WEATHER_CLOUDY, WEATHER_OVERCAST,
            WEATHER_RAIN_LIGHT, WEATHER_RAIN_MEDIUM, WEATHER_RAIN_HEAVY
        ]),
        ("Fall", datetime(2025, 10, 10, 14, 0, 0), "🍂", [
            WEATHER_SUNNY, WEATHER_CLOUDY, WEATHER_OVERCAST,
            WEATHER_RAIN_LIGHT, WEATHER_RAIN_MEDIUM, WEATHER_RAIN_HEAVY
        ]),
    ]

    for season_name, dt, emoji, weather_conditions in seasons:
        for weather in weather_conditions:
            title = f"{season_name} - {WEATHER_NAMES[weather]}"
            if not showcase_segment(device, title, dt, weather, emoji, QUICK_DURATION):
                return False

    return True


def showcase_halloween_all_weather(device):
    """Showcase Halloween with medium rain."""
    print("\n" + "="*70)
    print("🎃  SHOWCASING HALLOWEEN - RAINY NIGHT")
    print("="*70)

    # Halloween dates: Oct 28 - Nov 3
    halloween_dt = datetime(2025, 10, 31, 20, 0, 0)

    # Only medium rain
    title = f"Halloween - {WEATHER_NAMES[WEATHER_RAIN_MEDIUM]}"
    if not showcase_segment(device, title, halloween_dt, WEATHER_RAIN_MEDIUM, "🎃", SEGMENT_DURATION):
        return False

    return True


def showcase_christmas_all_weather(device):
    """Showcase Christmas with heavy snow."""
    print("\n" + "="*70)
    print("🎄  SHOWCASING CHRISTMAS - HEAVY SNOW")
    print("="*70)

    # Christmas dates: Dec 15 - Dec 31
    christmas_dt = datetime(2025, 12, 25, 14, 0, 0)

    # Only heavy snow for White Christmas
    title = f"Christmas - {WEATHER_NAMES[WEATHER_SNOW_HEAVY]}"
    if not showcase_segment(device, title, christmas_dt, WEATHER_SNOW_HEAVY, "🎄", SEGMENT_DURATION):
        return False

    return True


def showcase_easter_all_weather(device):
    """Showcase Easter with all weather conditions."""
    print("\n" + "="*70)
    print("🐰  SHOWCASING EASTER WITH ALL WEATHER CONDITIONS")
    print("="*70)

    # Easter date (example: April 10)
    easter_dt = datetime(2025, 4, 10, 14, 0, 0)

    weather_conditions = [
        WEATHER_SUNNY,
        WEATHER_CLOUDY,
        WEATHER_OVERCAST,
        WEATHER_RAIN_LIGHT,
        WEATHER_RAIN_MEDIUM,
        WEATHER_RAIN_HEAVY,
    ]

    for weather in weather_conditions:
        title = f"Easter - {WEATHER_NAMES[weather]}"
        if not showcase_segment(device, title, easter_dt, weather, "🐰", QUICK_DURATION):
            return False

    return True


def showcase_sun_movement(device):
    """Showcase sun movement during autumn sunny day."""
    print("\n" + "="*70)
    print("☀️  SHOWCASING SUN MOVEMENT - AUTUMN SUNNY DAY")
    print("="*70)

    # Only autumn sunny weather
    base_dt = datetime(2025, 10, 10, 6, 0, 0)

    # Show sun movement from dawn (6am) to dusk (8pm)
    send_media_update(device, "Autumn - Sun Movement (Dawn to Dusk)")
    print("🍂  Autumn - Sun Movement (6am to 8pm)")

    for hour in range(6, 21, 2):  # Every 2 hours
        dt = base_dt.replace(hour=hour)
        send_datetime_update(device, dt)
        send_weather_update(device, WEATHER_SUNNY)
        time.sleep(1.5)  # Quick transitions

    return True


def showcase_moon_phases(device):
    """Showcase moon phases during autumn sunny nights."""
    print("\n" + "="*70)
    print("🌙  SHOWCASING MOON PHASES - AUTUMN SUNNY NIGHT")
    print("="*70)

    # Different moon phases (days in lunar cycle) - October for autumn
    moon_phases = [
        (1, "New Moon"),
        (5, "Waxing Crescent"),
        (8, "First Quarter"),
        (11, "Waxing Gibbous"),
        (15, "Full Moon"),
        (19, "Waning Gibbous"),
        (22, "Last Quarter"),
        (27, "Waning Crescent"),
    ]

    send_media_update(device, "Lunar Phases - Autumn Night")
    print("🌙  Lunar Phases - Autumn Night")

    for day, phase_name in moon_phases:
        dt = datetime(2025, 10, day, 22, 0, 0)  # October for autumn
        send_datetime_update(device, dt)
        send_weather_update(device, WEATHER_SUNNY)  # Sunny (clear) night
        print(f"    Day {day:2d} - {phase_name}")
        time.sleep(2.0)

    return True


def showcase_special_events_times(device):
    """Showcase special events at different times of day."""
    print("\n" + "="*70)
    print("🕐  SHOWCASING SPECIAL EVENTS AT DIFFERENT TIMES")
    print("="*70)

    events = [
        ("Halloween Dawn", datetime(2025, 10, 31, 6, 0, 0), "🎃"),
        ("Halloween Noon", datetime(2025, 10, 31, 12, 0, 0), "🎃"),
        ("Halloween Dusk", datetime(2025, 10, 31, 18, 0, 0), "🎃"),
        ("Halloween Midnight", datetime(2025, 10, 31, 23, 59, 0), "🎃"),
        ("Christmas Morning", datetime(2025, 12, 25, 8, 0, 0), "🎄"),
        ("Christmas Noon", datetime(2025, 12, 25, 12, 0, 0), "🎄"),
        ("Christmas Evening", datetime(2025, 12, 25, 18, 0, 0), "🎄"),
        ("Christmas Night", datetime(2025, 12, 25, 22, 0, 0), "🎄"),
    ]

    for title, dt, emoji in events:
        if not showcase_segment(device, title, dt, WEATHER_SUNNY, emoji, QUICK_DURATION):
            return False

    return True


def showcase_weather_transitions(device):
    """Showcase weather transitions within a single day."""
    print("\n" + "="*70)
    print("🌦️  SHOWCASING WEATHER TRANSITIONS")
    print("="*70)

    # Summer day with changing weather
    base_dt = datetime(2025, 7, 15, 12, 0, 0)

    weather_sequence = [
        (WEATHER_SUNNY, "Clear Morning"),
        (WEATHER_CLOUDY, "Clouds Rolling In"),
        (WEATHER_OVERCAST, "Overcast Sky"),
        (WEATHER_RAIN_LIGHT, "Light Drizzle Begins"),
        (WEATHER_RAIN_MEDIUM, "Steady Rain"),
        (WEATHER_RAIN_HEAVY, "Heavy Downpour"),
        (WEATHER_RAIN_MEDIUM, "Rain Subsiding"),
        (WEATHER_CLOUDY, "Clouds Clearing"),
        (WEATHER_SUNNY, "Sunshine Returns"),
    ]

    send_media_update(device, "Summer Storm - Weather Transitions")
    print("🌦️  Summer Storm - Weather Transitions")

    for weather, description in weather_sequence:
        print(f"    {description} - {WEATHER_NAMES[weather]}")
        send_weather_update(device, weather)
        send_datetime_update(device, base_dt)
        time.sleep(2.5)

    return True


def showcase_seasonal_animals(device):
    """Showcase seasonal animals and effects."""
    print("\n" + "="*70)
    print("🦋  SHOWCASING SEASONAL ANIMALS & EFFECTS")
    print("="*70)

    scenes = [
        ("Spring - Birds & Butterflies", datetime(2025, 4, 15, 10, 0, 0), WEATHER_SUNNY, "🦋"),
        ("Summer - Bees & Flowers", datetime(2025, 7, 15, 14, 0, 0), WEATHER_SUNNY, "🐝"),
        ("Summer Evening - Fireflies", datetime(2025, 7, 20, 20, 0, 0), WEATHER_SUNNY, "✨"),
        ("Winter - Snowman & Drifts", datetime(2025, 1, 15, 12, 0, 0), WEATHER_SNOW_MEDIUM, "⛄"),
    ]

    for title, dt, weather, emoji in scenes:
        if not showcase_segment(device, title, dt, weather, emoji, SEGMENT_DURATION):
            return False

    return True


def main():
    """Main entry point."""
    print("=" * 70)
    print("   QMK COMPREHENSIVE DISPLAY SHOWCASE")
    print("=" * 70)
    print("\nThis showcase demonstrates:")
    print("  ✓ Sun movement during autumn sunny day (dawn to dusk)")
    print("  ✓ All 8 moon phases during autumn clear nights")
    print("  ✓ All 4 seasons with all weather conditions")
    print("  ✓ Halloween with medium rain")
    print("  ✓ Christmas with heavy snow (White Christmas!)")
    print("  ✓ Easter with all weather conditions")
    print("  ✓ Special events at different times of day")
    print()

    # Calculate approximate total time
    total_segments = (
        8 +       # Sun movement segments (autumn)
        8 +       # Moon phases (autumn)
        4 * 6 +   # Seasons with weather (4 seasons × 6 weather conditions)
        1 +       # Halloween with rain
        1 +       # Christmas with heavy snow
        6 +       # Easter with all weather
        8         # Special events times
    )
    total_time = total_segments * 3 + 60  # Rough estimate with overhead (3s per segment)
    print(f"Estimated duration: ~{total_time:.0f} seconds ({total_time/60:.1f} minutes)")
    print()

    # Connect to keyboard
    print("🔍 Looking for keyboard...")
    device = connect_to_keyboard()
    if not device:
        print("\n✗ Keyboard not found!")
        print("Make sure your keyboard is connected and the VID/PID are correct.")
        return 1

    print("✓ Connected to keyboard!\n")

    try:
        showcase_start = time.time()

        # Run showcases in order: sun/moon first, then seasons, then special events
        showcases = [
            showcase_sun_movement,
            showcase_moon_phases,
            showcase_seasons_all_weather,
            showcase_halloween_all_weather,
            showcase_christmas_all_weather,
            showcase_easter_all_weather,
            showcase_special_events_times,
        ]

        for i, showcase_func in enumerate(showcases, 1):
            print(f"\n[{i}/{len(showcases)}]")
            if not showcase_func(device):
                print("\n✗ Showcase failed or interrupted")
                return 1

            # Brief pause between showcases
            time.sleep(1.5)

        # Show completion
        showcase_duration = time.time() - showcase_start
        print("\n" + "=" * 70)
        print(f"✓ COMPREHENSIVE SHOWCASE COMPLETE!")
        print(f"  Duration: {showcase_duration:.1f}s ({showcase_duration/60:.1f} minutes)")
        print("=" * 70)

        # Return to current time with completion message
        current_dt = datetime.now()
        send_media_update(device, "Showcase Complete - All Features Demonstrated!")
        send_datetime_update(device, current_dt)
        send_weather_update(device, WEATHER_SUNNY)

        time.sleep(3.0)

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
