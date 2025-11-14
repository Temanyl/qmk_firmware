#!/usr/bin/env python3
"""
QMK Keyboard Companion - Unified Monitor and High Score Manager
Monitors system volume, media playback, and manages Doodle Jump high scores via Raw HID.

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
import argparse
import json
import urllib.request
import urllib.error
import urllib.parse
from datetime import datetime
from pathlib import Path

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

# Command IDs for monitor protocol (0x01-0x04: computer → keyboard)
CMD_VOLUME_UPDATE = 0x01
CMD_MEDIA_UPDATE = 0x02
CMD_DATETIME_UPDATE = 0x03
CMD_WEATHER_UPDATE = 0x04

# Command IDs for game protocol (0x10-0x13: bidirectional)
MSG_SCORE_SUBMIT = 0x10  # Keyboard → Computer: score submission
MSG_ENTER_NAME = 0x11    # Computer → Keyboard: ask for name (top 10)
MSG_SHOW_SCORES = 0x12   # Computer → Keyboard: display leaderboard
MSG_NAME_SUBMIT = 0x13   # Keyboard → Computer: name + score

# Poll intervals
POLL_INTERVAL = 0.1
MEDIA_POLL_INTERVAL = 1.0  # Check media every second
DATETIME_UPDATE_INTERVAL = 60.0  # Send time every 60 seconds
WEATHER_UPDATE_INTERVAL = 600.0  # Check weather every 10 minutes (600 seconds)

# High score file
SCRIPT_DIR = Path(__file__).parent
SCORES_FILE = SCRIPT_DIR / "highscores.json"


# ============================================================================
# HIGH SCORE MANAGEMENT
# ============================================================================

class HighScoreManager:
    """Manages high scores for Doodle Jump game."""

    def __init__(self):
        self.scores = self.load_scores()

    def load_scores(self):
        """Load high scores from JSON file"""
        if SCORES_FILE.exists():
            try:
                with open(SCORES_FILE, 'r') as f:
                    data = json.load(f)
                    # Ensure scores are sorted
                    data.sort(key=lambda x: x['score'], reverse=True)
                    return data[:10]  # Keep only top 10
            except Exception as e:
                print(f"⚠ Error loading scores: {e}")
                return []
        return []

    def save_scores(self):
        """Save high scores to JSON file"""
        try:
            with open(SCORES_FILE, 'w') as f:
                json.dump(self.scores, f, indent=2)
            print(f"💾 Saved {len(self.scores)} scores to {SCORES_FILE}")
        except Exception as e:
            print(f"⚠ Error saving scores: {e}")

    def add_score(self, name, score):
        """Add a new score and return its rank (0-9) or -1 if not in top 10"""
        # Add the new score
        self.scores.append({'name': name, 'score': score})

        # Sort by score (descending)
        self.scores.sort(key=lambda x: x['score'], reverse=True)

        # Find rank
        for i, entry in enumerate(self.scores):
            if entry['name'] == name and entry['score'] == score:
                rank = i
                break
        else:
            rank = -1

        # Keep only top 10
        self.scores = self.scores[:10]

        # Save to file
        self.save_scores()

        return rank if rank < 10 else -1

    def check_score(self, score):
        """Check if score makes top 10, return rank or -1"""
        if len(self.scores) < 10:
            return len(self.scores)  # Automatic entry

        # Check if score beats the lowest top 10 score
        if score > self.scores[9]['score']:
            # Find where it would rank
            for i, entry in enumerate(self.scores):
                if score > entry['score']:
                    return i
            return 9

        return -1  # Didn't make top 10

    def print_scores(self):
        """Print current high scores to console"""
        if self.scores:
            print("\n" + "=" * 40)
            print("        DOODLE JUMP HIGH SCORES")
            print("=" * 40)
            for i, entry in enumerate(self.scores):
                print(f"  {i+1:2d}. {entry['name']:3s}  {entry['score']:5d}")
            print("=" * 40 + "\n")
        else:
            print("\n📊 No high scores yet!\n")


# ============================================================================
# WEATHER MONITORING
# ============================================================================

# Weather state constants (match keyboard firmware)
WEATHER_SUNNY = 0
WEATHER_RAIN_LIGHT = 1
WEATHER_RAIN_MEDIUM = 2
WEATHER_RAIN_HEAVY = 3
WEATHER_SNOW_LIGHT = 4
WEATHER_SNOW_MEDIUM = 5
WEATHER_SNOW_HEAVY = 6

# Legacy aliases for backward compatibility
WEATHER_RAIN = WEATHER_RAIN_MEDIUM
WEATHER_SNOW = WEATHER_SNOW_MEDIUM

# Default location (Otterndorf, Germany)
DEFAULT_WEATHER_LATITUDE = 53.800
DEFAULT_WEATHER_LONGITUDE = 8.900
DEFAULT_WEATHER_LOCATION = "Otterndorf, Germany"


def get_weather_openmeteo(latitude, longitude):
    """
    Fetch weather from Open-Meteo API (no API key required).

    Args:
        latitude: Location latitude
        longitude: Location longitude

    Returns:
        Weather state (WEATHER_SUNNY, WEATHER_RAIN, WEATHER_SNOW) or None on error
    """
    try:
        # Open-Meteo API endpoint
        url = f"https://api.open-meteo.com/v1/forecast?latitude={latitude}&longitude={longitude}&current_weather=true"

        with urllib.request.urlopen(url, timeout=10) as response:
            data = json.loads(response.read().decode())

            # Get current weather code
            # WMO Weather interpretation codes (WW)
            # https://open-meteo.com/en/docs
            weather_code = data['current_weather']['weathercode']

            # Map WMO codes to our weather states with rain intensity
            # 0: Clear sky
            # 1-3: Mainly clear, partly cloudy, overcast
            # 45, 48: Fog
            # 51-55: Drizzle (light to moderate)
            # 56-57: Freezing drizzle
            # 61-65: Rain (light to heavy)
            # 66-67: Freezing rain
            # 71-75: Snow fall
            # 77: Snow grains
            # 80-82: Rain showers (slight to violent)
            # 85-86: Snow showers
            # 95: Thunderstorm
            # 96, 99: Thunderstorm with hail

            if weather_code in [71]:
                # Light snow fall
                return WEATHER_SNOW_LIGHT
            elif weather_code in [73]:
                # Moderate snow fall
                return WEATHER_SNOW_MEDIUM
            elif weather_code in [75, 77, 85, 86]:
                # Heavy snow fall, snow grains, snow showers
                return WEATHER_SNOW_HEAVY
            elif weather_code in [51, 61, 80]:
                # Light rain: slight drizzle (51), slight rain (61), slight rain showers (80)
                return WEATHER_RAIN_LIGHT
            elif weather_code in [53, 63, 81]:
                # Medium rain: moderate drizzle (53), moderate rain (63), moderate rain showers (81)
                return WEATHER_RAIN_MEDIUM
            elif weather_code in [55, 56, 57, 65, 66, 67, 82, 95, 96, 99]:
                # Heavy rain: dense drizzle (55), freezing drizzle (56-57), heavy rain (65),
                # freezing rain (66-67), violent rain showers (82), thunderstorms (95-99)
                return WEATHER_RAIN_HEAVY
            else:
                # Clear/Cloudy/Fog -> Sunny
                return WEATHER_SUNNY

    except Exception as e:
        print(f"⚠ Error fetching weather from Open-Meteo: {e}")
        return None


def get_weather_wttr(location):
    """
    Fetch weather from wttr.in API (no API key required).

    Args:
        location: City name (e.g., "San Francisco" or "London,UK")

    Returns:
        Weather state (WEATHER_SUNNY, WEATHER_RAIN, WEATHER_SNOW) or None on error
    """
    try:
        # wttr.in API endpoint
        # Encode location for URL
        location_encoded = urllib.parse.quote(location)
        url = f"https://wttr.in/{location_encoded}?format=j1"

        with urllib.request.urlopen(url, timeout=10) as response:
            data = json.loads(response.read().decode())

            # Get current condition
            current = data['current_condition'][0]
            weather_desc = current['weatherDesc'][0]['value'].lower()
            weather_code = int(current['weatherCode'])

            # Map wttr.in weather codes to our states with rain and snow intensity
            # Full list: https://github.com/chubin/wttr.in/blob/master/lib/constants.py
            # Light snow: 179, 227, 323, 326 (patchy light, light snow)
            # Medium snow: 182, 185, 230, 311, 314, 317, 320, 329, 332, 335, 350 (moderate snow, sleet, freezing)
            # Heavy snow: 281, 284, 338, 371, 374, 377, 392, 395 (heavy snow, snow showers, thunder)
            # Light rain: 176, 263, 293, 353
            # Medium rain: 266, 296, 299, 356, 359, 362
            # Heavy rain: 302, 305, 308, 365, 368, 386, 389

            light_snow_codes = [179, 227, 323, 326]  # Patchy possible, blowing, patchy light, light snow
            medium_snow_codes = [182, 185, 230, 311, 314, 317, 320, 329, 332, 335, 350]  # Moderate, sleet, freezing
            heavy_snow_codes = [281, 284, 338, 371, 374, 377, 392, 395]  # Heavy, showers, thunder
            light_rain_codes = [176, 263, 293, 353]  # Patchy rain, light drizzle, light rain shower
            medium_rain_codes = [266, 296, 299, 356, 359, 362]  # Moderate rain, drizzle
            heavy_rain_codes = [302, 305, 308, 365, 368, 386, 389]  # Heavy rain, torrential showers, thunderstorms

            # Check snow codes and descriptions
            if weather_code in heavy_snow_codes or 'heavy snow' in weather_desc or 'blizzard' in weather_desc or 'snow shower' in weather_desc:
                return WEATHER_SNOW_HEAVY
            elif weather_code in medium_snow_codes or ('moderate' in weather_desc and 'snow' in weather_desc):
                return WEATHER_SNOW_MEDIUM
            elif weather_code in light_snow_codes or ('light' in weather_desc and 'snow' in weather_desc) or ('patchy' in weather_desc and 'snow' in weather_desc):
                return WEATHER_SNOW_LIGHT
            elif 'snow' in weather_desc:
                # Fallback: generic snow without intensity -> medium
                return WEATHER_SNOW_MEDIUM
            elif weather_code in heavy_rain_codes or 'heavy' in weather_desc or 'torrential' in weather_desc or 'thunder' in weather_desc:
                return WEATHER_RAIN_HEAVY
            elif weather_code in medium_rain_codes or 'moderate' in weather_desc:
                return WEATHER_RAIN_MEDIUM
            elif weather_code in light_rain_codes or 'light' in weather_desc or 'patchy' in weather_desc or 'drizzle' in weather_desc:
                return WEATHER_RAIN_LIGHT
            elif 'rain' in weather_desc or 'shower' in weather_desc:
                # Fallback: generic rain without intensity -> medium
                return WEATHER_RAIN_MEDIUM
            else:
                return WEATHER_SUNNY

    except Exception as e:
        print(f"⚠ Error fetching weather from wttr.in: {e}")
        return None


def get_current_weather(location=None, latitude=None, longitude=None):
    """
    Get current weather condition using available APIs.

    Args:
        location: City name (for wttr.in)
        latitude: Latitude (for Open-Meteo)
        longitude: Longitude (for Open-Meteo)

    Returns:
        Weather state (WEATHER_SUNNY, WEATHER_RAIN, WEATHER_SNOW) or None
    """
    # Try Open-Meteo first if coordinates provided
    if latitude is not None and longitude is not None:
        weather = get_weather_openmeteo(latitude, longitude)
        if weather is not None:
            return weather

    # Try wttr.in if location provided
    if location is not None:
        weather = get_weather_wttr(location)
        if weather is not None:
            return weather

    # All APIs failed
    return None


# ============================================================================
# SYSTEM MONITORING
# ============================================================================

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
        print(f"⚠ Error getting macOS volume: {e}")
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
        print(f"⚠ Error getting Windows volume: {e}")
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
            print(f"⚠ Error getting Linux volume with amixer: {e}")
            return None
    except Exception as e:
        print(f"⚠ Error getting Linux volume with pactl: {e}")
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
        print(f"⚠ Unsupported platform: {system}")
        return None


def get_media_macos():
    """Get currently playing media on macOS."""
    try:
        # Try Apple Music first
        result = subprocess.run(
            ['osascript', '-e', '''
                tell application "Music"
                    if player state is playing then
                        return name of current track & " - " & artist of current track
                    end if
                end tell
            '''],
            capture_output=True,
            text=True,
            timeout=0.5
        )
        if result.stdout.strip():
            return result.stdout.strip()

        # Try Spotify if Music didn't return anything
        result = subprocess.run(
            ['osascript', '-e', '''
                tell application "Spotify"
                    if player state is playing then
                        return name of current track & " - " & artist of current track
                    end if
                end tell
            '''],
            capture_output=True,
            text=True,
            timeout=0.5
        )
        if result.stdout.strip():
            return result.stdout.strip()

        return None
    except Exception:
        return None


def get_media_windows():
    """Get currently playing media on Windows."""
    # Windows Media API is complex, return None for now
    # Can be implemented using Windows.Media.Control
    return None


def get_media_linux():
    """Get currently playing media on Linux via MPRIS."""
    try:
        # Use playerctl if available
        result = subprocess.run(
            ['playerctl', 'metadata', '--format', '{{ title }} - {{ artist }}'],
            capture_output=True,
            text=True,
            timeout=0.5
        )
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
        return None
    except Exception:
        return None


def get_current_media():
    """Get currently playing media based on platform."""
    system = platform.system()

    if system == "Darwin":  # macOS
        return get_media_macos()
    elif system == "Windows":
        return get_media_windows()
    elif system == "Linux":
        return get_media_linux()
    else:
        return None


# ============================================================================
# HID COMMUNICATION
# ============================================================================

def find_keyboard_device(silent=False):
    """Find the keyboard HID device. Set silent=True to suppress output."""
    if not silent:
        print(f"🔍 Looking for keyboard (VID: 0x{VENDOR_ID:04X}, PID: 0x{PRODUCT_ID:04X})...")

    # List all HID devices
    devices = hid.enumerate()

    # Find our keyboard by VID/PID and usage page
    for device_info in devices:
        if (device_info['vendor_id'] == VENDOR_ID and
            device_info['product_id'] == PRODUCT_ID and
            device_info['usage_page'] == USAGE_PAGE and
            device_info['usage'] == USAGE):

            if not silent:
                print(f"✓ Found keyboard: {device_info['product_string']}")
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


def send_media_update(device, media_text):
    """Send media text update to keyboard via Raw HID."""
    # Create HID packet (32 bytes)
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_MEDIA_UPDATE  # Command ID

    # Encode media text (max 30 chars + null terminator)
    if media_text:
        media_bytes = media_text.encode('utf-8')[:30]  # Limit to 30 chars
        packet[1:1+len(media_bytes)] = media_bytes
        packet[1+len(media_bytes)] = 0  # Null terminator
    else:
        packet[1] = 0  # Empty string

    try:
        # Send the packet
        bytes_written = device.write([0] + list(packet))

        # Check if write was successful
        if bytes_written <= 0:
            print(f"✗ Media write failed: {bytes_written} bytes written")
            return False

        return True
    except Exception as e:
        print(f"✗ Error sending media packet: {e}")
        return False


def send_datetime_update(device, override_datetime=None):
    """Send current date/time update to keyboard via Raw HID.

    Args:
        device: HID device handle
        override_datetime: Optional datetime object to send instead of current time (for testing)
    """
    # Create HID packet (32 bytes)
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_DATETIME_UPDATE  # Command ID

    # Get current date/time or use override
    now = override_datetime if override_datetime else datetime.now()

    # Pack date/time: year (2 bytes), month, day, hour, minute, second
    packet[1] = now.year & 0xFF  # Year low byte
    packet[2] = (now.year >> 8) & 0xFF  # Year high byte
    packet[3] = now.month
    packet[4] = now.day
    packet[5] = now.hour
    packet[6] = now.minute
    packet[7] = now.second

    try:
        # Send the packet
        bytes_written = device.write([0] + list(packet))

        # Check if write was successful
        if bytes_written <= 0:
            print(f"✗ DateTime write failed: {bytes_written} bytes written")
            return False

        return True
    except Exception as e:
        print(f"✗ Error sending datetime packet: {e}")
        return False


def send_weather_update(device, weather_state):
    """Send weather update to keyboard via Raw HID.

    Args:
        device: HID device handle
        weather_state: Weather state (WEATHER_SUNNY=0, WEATHER_RAIN_LIGHT=1,
                       WEATHER_RAIN_MEDIUM=2, WEATHER_RAIN_HEAVY=3, WEATHER_SNOW_LIGHT=4,
                       WEATHER_SNOW_MEDIUM=5, WEATHER_SNOW_HEAVY=6)

    Returns:
        True on success, False on failure
    """
    # Create HID packet (32 bytes)
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_WEATHER_UPDATE  # Command ID (0x04)
    packet[1] = weather_state        # Weather state (0-6)

    try:
        # Send the packet
        bytes_written = device.write([0] + list(packet))

        # Check if write was successful
        if bytes_written <= 0:
            print(f"✗ Weather write failed: {bytes_written} bytes written")
            return False

        return True
    except Exception as e:
        print(f"✗ Error sending weather packet: {e}")
        return False


def send_enter_name(device, rank):
    """Send ENTER_NAME message to keyboard"""
    data = bytearray(HID_PACKET_SIZE)
    data[0] = MSG_ENTER_NAME
    data[1] = rank

    try:
        bytes_written = device.write([0] + list(data))
        if bytes_written <= 0:
            print(f"✗ Enter name write failed")
            return False
        print(f"🎮 Sent ENTER_NAME (rank: {rank + 1})")
        return True
    except Exception as e:
        print(f"✗ Error sending ENTER_NAME: {e}")
        return False


def send_show_scores(device, scores):
    """Send SHOW_SCORES message with top 10 list"""
    data = bytearray(HID_PACKET_SIZE)
    data[0] = MSG_SHOW_SCORES

    # Pack up to 10 scores (each: 3 chars + 2 bytes score = 5 bytes)
    offset = 1
    for i, entry in enumerate(scores[:10]):
        if offset + 5 > 32:
            break  # Max 6 scores per packet (1 + 6*5 = 31 bytes)

        # Encode name (3 chars)
        name = entry['name'][:3].ljust(3)
        for c in name:
            data[offset] = ord(c)
            offset += 1

        # Encode score (2 bytes, big-endian)
        score = min(entry['score'], 65535)
        data[offset] = (score >> 8) & 0xFF
        data[offset + 1] = score & 0xFF
        offset += 2

    try:
        bytes_written = device.write([0] + list(data))
        if bytes_written <= 0:
            print(f"✗ Show scores write failed")
            return False
        print(f"🎮 Sent SHOW_SCORES ({len(scores)} entries)")
        return True
    except Exception as e:
        print(f"✗ Error sending SHOW_SCORES: {e}")
        return False


def process_game_message(device, data, score_manager):
    """Process incoming game message from keyboard"""
    if len(data) == 0:
        return True

    msg_type = data[0]

    if msg_type == MSG_SCORE_SUBMIT:
        # Parse score (2 bytes, big-endian)
        score = (data[1] << 8) | data[2]
        print(f"\n{'=' * 50}")
        print(f"🎮 Score Received: {score}")
        print(f"{'=' * 50}")

        # Check if it makes top 10
        rank = score_manager.check_score(score)

        if rank >= 0:
            print(f"🏆 NEW HIGH SCORE! Rank: {rank + 1}")
            return send_enter_name(device, rank)
        else:
            print("📊 Score didn't make top 10")
            return send_show_scores(device, score_manager.scores)

    elif msg_type == MSG_NAME_SUBMIT:
        # Parse name (3 chars) and score (2 bytes)
        name = ''.join(chr(data[i]) for i in range(1, 4))
        score = (data[4] << 8) | data[5]
        print(f"\n{'=' * 50}")
        print(f"🎮 Name Submitted: {name} - {score}")
        print(f"{'=' * 50}")

        # Add to scores
        rank = score_manager.add_score(name, score)
        if rank >= 0:
            print(f"💾 Added to high scores at rank {rank + 1}")
            score_manager.print_scores()

        # Send updated scores
        return send_show_scores(device, score_manager.scores)

    return True


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
            print(f"⚠ Error opening HID device: {e}")
        return None


# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

def get_season_name(month):
    """Get season name from month number."""
    if month in [12, 1, 2]:
        return "Winter"
    elif month in [3, 4, 5]:
        return "Spring"
    elif month in [6, 7, 8]:
        return "Summer"
    else:
        return "Fall"


def get_time_of_day_name(hour):
    """Get time of day name from hour."""
    if 5 <= hour < 12:
        return "Morning"
    elif 12 <= hour < 18:
        return "Day"
    elif 18 <= hour < 22:
        return "Evening"
    else:
        return "Night"


def parse_test_datetime(datetime_str):
    """Parse test datetime string in format YYYY-MM-DD HH:MM or YYYY-MM-DD HH:MM:SS"""
    try:
        # Try with seconds first
        try:
            return datetime.strptime(datetime_str, "%Y-%m-%d %H:%M:%S")
        except ValueError:
            # Try without seconds
            return datetime.strptime(datetime_str, "%Y-%m-%d %H:%M")
    except ValueError as e:
        raise argparse.ArgumentTypeError(
            f"Invalid datetime format: {datetime_str}. Use YYYY-MM-DD HH:MM or YYYY-MM-DD HH:MM:SS"
        )


# ============================================================================
# MAIN LOOP
# ============================================================================

def main():
    """Main loop: monitor volume/media, send datetime, handle game messages."""
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description="QMK Keyboard Companion - Volume/media/weather monitoring + High score management",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Normal operation (weather enabled by default for Otterndorf, Germany)
  python3 keyboard_monitor.py

  # Disable weather monitoring
  python3 keyboard_monitor.py --no-weather

  # Override weather location with city name
  python3 keyboard_monitor.py --weather-location "San Francisco"

  # Override weather location with coordinates (more accurate)
  python3 keyboard_monitor.py --weather-latitude 37.7749 --weather-longitude -122.4194

  # Custom weather update interval (5 minutes instead of 10)
  python3 keyboard_monitor.py --weather-interval 300

  # Test winter night scene (January 1st at 11 PM)
  python3 keyboard_monitor.py --test-date "2025-01-01 23:00"

  # Test summer day (July 20th at 2 PM)
  python3 keyboard_monitor.py --test-date "2025-07-20 14:00"

Features:
  • Monitors system volume and sends to keyboard display
  • Tracks media playback (Music/Spotify on macOS)
  • Syncs date/time for seasonal animations
  • Fetches live weather data (Open-Meteo/wttr.in - no API key!)
  • Default location: Otterndorf, Germany
  • Default update interval: 10 minutes
  • Manages Doodle Jump high scores via Raw HID
  • Auto-reconnects if keyboard is unplugged
        """
    )
    parser.add_argument(
        '--test-date',
        type=parse_test_datetime,
        metavar='DATETIME',
        help='Override date/time for testing animations (format: YYYY-MM-DD HH:MM or YYYY-MM-DD HH:MM:SS)'
    )
    parser.add_argument(
        '--weather-location',
        type=str,
        metavar='LOCATION',
        help='Location for weather updates (e.g., "San Francisco" or "London,UK"). Overrides default location.'
    )
    parser.add_argument(
        '--weather-latitude',
        type=float,
        metavar='LAT',
        help='Latitude for weather updates (use with --weather-longitude). Overrides default location.'
    )
    parser.add_argument(
        '--weather-longitude',
        type=float,
        metavar='LON',
        help='Longitude for weather updates (use with --weather-latitude). Overrides default location.'
    )
    parser.add_argument(
        '--weather-interval',
        type=int,
        default=600,
        metavar='SECONDS',
        help='Weather update interval in seconds (default: 600 = 10 minutes)'
    )
    parser.add_argument(
        '--no-weather',
        action='store_true',
        help='Disable weather monitoring (weather is enabled by default with Otterndorf, Germany)'
    )
    args = parser.parse_args()

    print("=" * 60)
    print("    QMK KEYBOARD COMPANION")
    print("    Volume/Media/Weather Monitor + High Score Manager")
    print("=" * 60)

    # Validate weather configuration
    # Weather is enabled by default with default location unless --no-weather is specified
    weather_enabled = not args.no_weather

    # Determine which location to use
    weather_latitude = args.weather_latitude
    weather_longitude = args.weather_longitude
    weather_location = args.weather_location

    # If no custom location provided, use defaults
    if weather_enabled and weather_latitude is None and weather_longitude is None and weather_location is None:
        weather_latitude = DEFAULT_WEATHER_LATITUDE
        weather_longitude = DEFAULT_WEATHER_LONGITUDE
        weather_location = DEFAULT_WEATHER_LOCATION
        using_default = True
    else:
        using_default = False

    if weather_enabled:
        print(f"\n🌤️  WEATHER: Enabled")
        if weather_latitude is not None and weather_longitude is not None:
            location_str = f"{weather_latitude}, {weather_longitude}"
            if using_default:
                location_str += f" ({DEFAULT_WEATHER_LOCATION})"
            print(f"   Location: {location_str}")
            print(f"   API: Open-Meteo (primary)")
        if weather_location and not (weather_latitude and weather_longitude):
            print(f"   Location: {weather_location}")
            print(f"   API: wttr.in (primary)")
        print(f"   Update interval: {args.weather_interval} seconds ({args.weather_interval // 60} minutes)")
    else:
        print(f"\n🌤️  WEATHER: Disabled (use --weather-location or remove --no-weather to enable)")

    if args.test_date:
        print(f"\n🧪 TEST MODE: Using override date/time")
        print(f"   {args.test_date.strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"   Season: {get_season_name(args.test_date.month)}")
        print(f"   Time: {get_time_of_day_name(args.test_date.hour)}")

    print("\n⏳ Waiting for keyboard... (Press Ctrl+C to quit)\n")

    # Initialize high score manager
    score_manager = HighScoreManager()
    score_manager.print_scores()

    device = None
    last_volume = None
    last_media = None
    last_weather = None  # Last weather state sent
    reconnect_delay = 2.0  # Wait 2 seconds between reconnection attempts
    last_connect_attempt = 0
    first_connection = True  # Track if this is the first connection
    connection_check_interval = 1.0  # Check connection every second
    last_connection_check = 0
    last_media_check = 0  # Last time we checked media
    last_datetime_update = 0  # Last time we sent date/time
    last_weather_check = 0  # Last time we checked weather

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
                        print("📊 Monitoring system volume + game scores...\n")

                        # Reset connection check timer to check immediately
                        last_connection_check = current_time

                        # Immediately send current volume on (re)connect
                        current_volume = get_system_volume()
                        if current_volume is not None:
                            print(f"🔊 Syncing volume: {current_volume}%")
                            if send_volume_update(device, current_volume):
                                last_volume = current_volume
                            else:
                                # Send failed immediately after connect
                                print("✗ Initial sync failed")
                                print("⏳ Waiting for keyboard to reconnect...\n")
                                try:
                                    device.close()
                                except:
                                    pass
                                device = None
                                last_volume = None
                                last_media = None
                                last_weather = None
                                continue
                        else:
                            # Reset last_volume to force update on next successful read
                            last_volume = None

                        # Immediately send current media on (re)connect
                        current_media = get_current_media()
                        if current_media:
                            print(f"♪ Syncing media: {current_media}")
                            send_media_update(device, current_media)
                            last_media = current_media
                        else:
                            last_media = None

                        # Immediately send current date/time on (re)connect
                        dt_to_send = args.test_date if args.test_date else datetime.now()
                        print(f"📅 Syncing date/time: {dt_to_send.strftime('%Y-%m-%d %H:%M:%S')}")
                        if send_datetime_update(device, dt_to_send):
                            last_datetime_update = current_time
                        else:
                            print("✗ DateTime sync failed")

                        # Immediately send current weather on (re)connect if enabled
                        if weather_enabled:
                            current_weather = get_current_weather(
                                location=weather_location,
                                latitude=weather_latitude,
                                longitude=weather_longitude
                            )
                            if current_weather is not None:
                                weather_names = {
                                    WEATHER_SUNNY: "Sunny",
                                    WEATHER_RAIN_LIGHT: "Light Rain",
                                    WEATHER_RAIN_MEDIUM: "Rain",
                                    WEATHER_RAIN_HEAVY: "Heavy Rain",
                                    WEATHER_SNOW_LIGHT: "Light Snow",
                                    WEATHER_SNOW_MEDIUM: "Snow",
                                    WEATHER_SNOW_HEAVY: "Heavy Snow"
                                }
                                print(f"🌤️  Syncing weather: {weather_names.get(current_weather, 'Unknown')}")
                                if send_weather_update(device, current_weather):
                                    last_weather = current_weather
                                    last_weather_check = current_time
                                else:
                                    print("✗ Weather sync failed")
                            else:
                                print("⚠ Weather fetch failed")

                # Wait before next iteration
                time.sleep(0.5)
                continue

            # We're connected, periodically check if device is still there
            if current_time - last_connection_check >= connection_check_interval:
                last_connection_check = current_time
                if not is_keyboard_connected():
                    print("✗ Keyboard disconnected")
                    print("⏳ Waiting for keyboard to reconnect...\n")
                    try:
                        device.close()
                    except:
                        pass
                    device = None
                    last_volume = None
                    last_media = None
                    last_weather = None
                    continue

            # Check for incoming messages from keyboard (non-blocking)
            try:
                # Read with very short timeout to not block volume monitoring
                data = device.read(HID_PACKET_SIZE, timeout_ms=10)

                if data and len(data) > 0:
                    # Process game message
                    if not process_game_message(device, data, score_manager):
                        # Message handling failed, likely disconnected
                        print("✗ Game message handling failed")
                        print("⏳ Waiting for keyboard to reconnect...\n")
                        try:
                            device.close()
                        except:
                            pass
                        device = None
                        last_volume = None
                        last_media = None
                        last_weather = None
                        continue
            except Exception as e:
                # Read error doesn't necessarily mean disconnection
                # Will be caught by connection check
                pass

            # Monitor volume and send updates
            try:
                current_volume = get_system_volume()

                if current_volume is not None:
                    # Only send update if volume changed
                    if current_volume != last_volume:
                        print(f"🔊 Volume changed: {current_volume}%")
                        if send_volume_update(device, current_volume):
                            last_volume = current_volume
                        else:
                            # Send failed, likely disconnected
                            print("✗ Send failed, keyboard may be disconnected")
                            print("⏳ Waiting for keyboard to reconnect...\n")
                            try:
                                device.close()
                            except:
                                pass
                            device = None
                            last_volume = None
                            last_media = None
                            last_weather = None
                            continue

                # Check media info periodically (every MEDIA_POLL_INTERVAL)
                if current_time - last_media_check >= MEDIA_POLL_INTERVAL:
                    last_media_check = current_time
                    current_media = get_current_media()

                    # Send media update if it changed
                    if current_media != last_media:
                        if current_media:
                            print(f"♪ Now playing: {current_media}")
                        else:
                            print("♪ Playback stopped")

                        if send_media_update(device, current_media if current_media else ""):
                            last_media = current_media
                        else:
                            # Send failed, likely disconnected
                            print("✗ Media send failed, keyboard may be disconnected")
                            print("⏳ Waiting for keyboard to reconnect...\n")
                            try:
                                device.close()
                            except:
                                pass
                            device = None
                            last_volume = None
                            last_media = None
                            last_weather = None
                            continue

                # Periodically send date/time updates (every minute, unless using test date)
                # If using test date, only send once at connection
                if not args.test_date and current_time - last_datetime_update >= DATETIME_UPDATE_INTERVAL:
                    if send_datetime_update(device):
                        last_datetime_update = current_time
                    else:
                        # Send failed, likely disconnected
                        print("✗ DateTime send failed, keyboard may be disconnected")
                        print("⏳ Waiting for keyboard to reconnect...\n")
                        try:
                            device.close()
                        except:
                            pass
                        device = None
                        last_volume = None
                        last_media = None
                        last_weather = None
                        continue

                # Periodically check weather (if enabled)
                if weather_enabled and current_time - last_weather_check >= args.weather_interval:
                    current_weather = get_current_weather(
                        location=weather_location,
                        latitude=weather_latitude,
                        longitude=weather_longitude
                    )

                    if current_weather is not None:
                        # Only send update if weather changed
                        if current_weather != last_weather:
                            weather_names = {
                                WEATHER_SUNNY: "Sunny",
                                WEATHER_RAIN_LIGHT: "Light Rain",
                                WEATHER_RAIN_MEDIUM: "Rain",
                                WEATHER_RAIN_HEAVY: "Heavy Rain",
                                WEATHER_SNOW_LIGHT: "Light Snow",
                                WEATHER_SNOW_MEDIUM: "Snow",
                                WEATHER_SNOW_HEAVY: "Heavy Snow"
                            }
                            print(f"🌤️  Weather changed: {weather_names.get(current_weather, 'Unknown')}")

                            if send_weather_update(device, current_weather):
                                last_weather = current_weather
                                last_weather_check = current_time
                            else:
                                # Send failed, likely disconnected
                                print("✗ Weather send failed, keyboard may be disconnected")
                                print("⏳ Waiting for keyboard to reconnect...\n")
                                try:
                                    device.close()
                                except:
                                    pass
                                device = None
                                last_volume = None
                                last_media = None
                                last_weather = None
                                continue
                        else:
                            # Weather unchanged, just update check time
                            last_weather_check = current_time
                    else:
                        # Weather fetch failed, try again next interval
                        print("⚠ Weather fetch failed, will retry next interval")
                        last_weather_check = current_time

                # Wait before next poll
                time.sleep(POLL_INTERVAL)

            except Exception as e:
                # Any error during communication, assume disconnected
                print(f"✗ Connection error: {e}")
                print("⏳ Waiting for keyboard to reconnect...\n")
                try:
                    device.close()
                except:
                    pass
                device = None
                last_volume = None
                last_media = None
                last_weather = None

    except KeyboardInterrupt:
        print("\n\n👋 Stopping keyboard companion...")
    finally:
        if device is not None:
            try:
                device.close()
            except:
                pass


if __name__ == "__main__":
    main()
