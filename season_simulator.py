#!/usr/bin/env python3
"""
Season Simulator for QMK Keyboards with Raw HID
Simulates seasonal and day/night cycles on the keyboard display.

Simulates one month of each season in 60 seconds (1 minute). Shows 5 days per month
with different moon phases. Can also simulate just one specific season.

Requirements:
    - hidapi

Install dependencies:
    pip3 install hidapi

Usage:
    # Simulate all 4 seasons (4 minutes total)
    python3 season_simulator.py

    # Simulate only winter (1 minute)
    python3 season_simulator.py --season winter

    # Simulate only spring (1 minute)
    python3 season_simulator.py --season spring
"""

import time
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

# Command ID for datetime update
CMD_DATETIME_UPDATE = 0x03

# Season definitions (matching keymap.c)
SEASONS = {
    'winter': {'name': 'Winter', 'months': [12, 1, 2], 'representative_month': 1},
    'spring': {'name': 'Spring', 'months': [3, 4, 5], 'representative_month': 4},
    'summer': {'name': 'Summer', 'months': [6, 7, 8], 'representative_month': 7},
    'fall': {'name': 'Fall', 'months': [9, 10, 11], 'representative_month': 10},
}

# Simulation parameters
MONTH_DURATION = 60.0  # 60 seconds per month (1 minute)
DAYS_PER_MONTH = 5     # Simulate 5 days per month (to see moon phases)
DAY_DURATION = MONTH_DURATION / DAYS_PER_MONTH  # 12 seconds per day
HOURS_PER_DAY = 24
HOUR_DURATION = DAY_DURATION / HOURS_PER_DAY  # 0.5 seconds per hour


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
    # Create HID packet (32 bytes)
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
        # Send the packet
        bytes_written = device.write([0] + list(packet))

        # Check if write was successful
        if bytes_written <= 0:
            return False

        return True
    except Exception as e:
        print(f"\n✗ Error sending datetime packet: {e}")
        return False


def get_time_emoji(hour):
    """Get emoji for time of day."""
    if 5 <= hour < 7:
        return "🌅"  # Sunrise
    elif 7 <= hour < 12:
        return "☀️"  # Morning sun
    elif 12 <= hour < 18:
        return "🌞"  # Day sun
    elif 18 <= hour < 20:
        return "🌇"  # Sunset
    elif 20 <= hour < 22:
        return "🌆"  # Evening
    else:
        return "🌙"  # Night


def get_season_emoji(season_key):
    """Get emoji for season."""
    emojis = {
        'winter': '❄️',
        'spring': '🌸',
        'summer': '☀️',
        'fall': '🍂'
    }
    return emojis.get(season_key, '🌍')


def simulate_month(device, season_key, month, year=2025):
    """Simulate one month of a season."""
    season = SEASONS[season_key]
    season_emoji = get_season_emoji(season_key)

    print(f"\n{'='*60}")
    print(f"{season_emoji}  Simulating {season['name']} (Month {month})")
    print(f"{'='*60}")
    print(f"Duration: {MONTH_DURATION:.0f} seconds | Days: {DAYS_PER_MONTH} | Hours per day: {HOURS_PER_DAY}")
    print()

    start_time = time.time()

    # Spread days across the month to show different moon phases
    # Days: 1, 8, 15, 22, 29 (roughly new moon, first quarter, full, last quarter, new)
    day_numbers = [1, 8, 15, 22, 29]

    for day_index in range(DAYS_PER_MONTH):
        day = day_numbers[day_index]

        # Cycle through all hours of the day
        for hour in range(HOURS_PER_DAY):
            # Calculate when this hour should occur
            target_time = start_time + day_index * DAY_DURATION + hour * HOUR_DURATION

            # Wait until the target time
            sleep_time = target_time - time.time()
            if sleep_time > 0:
                time.sleep(sleep_time)

            # Create datetime for this moment
            dt = datetime(year, month, day, hour, 0, 0)

            # Send update to keyboard
            if not send_datetime_update(device, dt):
                print("\n✗ Failed to send update. Keyboard may be disconnected.")
                return False

            # Print progress (update every 6 hours to reduce clutter)
            if hour % 6 == 0:
                time_emoji = get_time_emoji(hour)
                elapsed = time.time() - start_time
                progress = (elapsed / MONTH_DURATION) * 100
                # Show moon phase info
                moon_phase = (day * 29) // 31
                moon_emojis = {0: "🌑", 7: "🌓", 14: "🌕", 22: "🌗", 29: "🌑"}
                moon_emoji = moon_emojis.get(moon_phase, "🌘")
                print(f"{time_emoji} Day {day:2d} - {hour:02d}:00 {moon_emoji}  [{progress:5.1f}% complete]")

        # Brief summary at end of each day
        elapsed = time.time() - start_time
        print(f"   └─ Day {day} complete ({elapsed:.1f}s elapsed)")

    elapsed = time.time() - start_time
    print(f"\n✓ {season['name']} complete! (Duration: {elapsed:.1f}s)")

    return True


def simulate_seasons(device, specific_season=None):
    """Simulate seasons. If specific_season is provided, only simulate that one."""

    if specific_season:
        # Simulate just one season
        if specific_season not in SEASONS:
            print(f"✗ Unknown season: {specific_season}")
            print(f"Valid seasons: {', '.join(SEASONS.keys())}")
            return False

        season = SEASONS[specific_season]
        month = season['representative_month']

        print(f"\n🎬 Starting simulation: {season['name']} only")
        print(f"Total duration: ~{MONTH_DURATION:.0f} seconds (1 minute)")

        return simulate_month(device, specific_season, month)

    else:
        # Simulate all 4 seasons
        total_duration = MONTH_DURATION * 4
        print(f"\n🎬 Starting simulation: All 4 seasons")
        print(f"Total duration: ~{total_duration:.0f} seconds ({total_duration/60:.0f} minutes)")

        season_order = ['winter', 'spring', 'summer', 'fall']

        for season_key in season_order:
            season = SEASONS[season_key]
            month = season['representative_month']

            if not simulate_month(device, season_key, month):
                return False

            # Brief pause between seasons
            time.sleep(0.5)

        return True


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="QMK Season Simulator - Simulate seasonal and day/night cycles",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Simulate all 4 seasons (4 minutes total)
  python3 season_simulator.py

  # Simulate only winter (1 minute)
  python3 season_simulator.py --season winter

  # Simulate only spring (1 minute)
  python3 season_simulator.py --season spring

  # Simulate only summer (1 minute)
  python3 season_simulator.py --season summer

  # Simulate only fall (1 minute)
  python3 season_simulator.py --season fall

Season Details:
  - Each season is simulated for 60 seconds (1 minute)
  - 5 days are shown per season (days 1, 8, 15, 22, 29 for moon phases)
  - Each day takes 12 seconds and cycles through all 24 hours
  - Each hour takes 0.5 seconds to show smooth day/night transitions
  - Seasons match the keyboard firmware:
    * Winter: December, January, February (shows month 1)
    * Spring: March, April, May (shows month 4)
    * Summer: June, July, August (shows month 7)
    * Fall: September, October, November (shows month 10)
        """
    )
    parser.add_argument(
        '--season',
        type=str.lower,
        choices=['winter', 'spring', 'summer', 'fall'],
        help='Simulate only a specific season (winter, spring, summer, or fall)'
    )
    args = parser.parse_args()

    print("QMK Season Simulator")
    print("=" * 60)

    # Connect to keyboard
    device = connect_to_keyboard()
    if not device:
        print("\n✗ Keyboard not found!")
        print("Make sure your keyboard is connected and the VID/PID are correct.")
        return 1

    try:
        # Run simulation
        success = simulate_seasons(device, args.season)

        if success:
            print("\n" + "=" * 60)
            print("✓ Simulation complete!")
            print("=" * 60)
            return 0
        else:
            print("\n✗ Simulation failed or interrupted")
            return 1

    except KeyboardInterrupt:
        print("\n\n⏹️  Simulation interrupted by user")
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
