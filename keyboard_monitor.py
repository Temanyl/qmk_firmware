#!/usr/bin/env python3
"""
Keyboard Monitor for QMK Keyboards with Raw HID
Monitors system volume and media playback, sending updates to keyboard display via Raw HID.

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
from datetime import datetime

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
CMD_MEDIA_UPDATE = 0x02
CMD_DATETIME_UPDATE = 0x03

# Poll interval in seconds
POLL_INTERVAL = 0.1
MEDIA_POLL_INTERVAL = 1.0  # Check media every second


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


def send_datetime_update(device):
    """Send current date/time update to keyboard via Raw HID."""
    # Create HID packet (32 bytes)
    packet = bytearray(HID_PACKET_SIZE)
    packet[0] = CMD_DATETIME_UPDATE  # Command ID

    # Get current date/time
    now = datetime.now()

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
    """Main loop: monitor volume and media, sending updates to keyboard."""
    print("QMK Keyboard Monitor")
    print("=" * 50)
    print("Waiting for keyboard... (Press Ctrl+C to quit)\n")

    device = None
    last_volume = None
    last_media = None
    reconnect_delay = 2.0  # Wait 2 seconds between reconnection attempts
    last_connect_attempt = 0
    first_connection = True  # Track if this is the first connection
    connection_check_interval = 1.0  # Check connection every second
    last_connection_check = 0
    last_media_check = 0  # Last time we checked media
    last_datetime_update = 0  # Last time we sent date/time
    datetime_update_interval = 60.0  # Send time every 60 seconds

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
                                last_media = None
                                continue
                        else:
                            # Reset last_volume to force update on next successful read
                            last_volume = None

                        # Immediately send current media on (re)connect
                        current_media = get_current_media()
                        if current_media:
                            print(f"Syncing media: {current_media}")
                            send_media_update(device, current_media)
                            last_media = current_media
                        else:
                            last_media = None

                        # Immediately send current date/time on (re)connect
                        print(f"Syncing date/time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
                        if send_datetime_update(device):
                            last_datetime_update = current_time
                        else:
                            print("✗ DateTime sync failed")

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
                            last_media = None
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
                            print("Waiting for keyboard to reconnect...\n")
                            try:
                                device.close()
                            except:
                                pass
                            device = None
                            last_volume = None
                            last_media = None
                            continue

                # Periodically send date/time updates (every minute)
                if current_time - last_datetime_update >= datetime_update_interval:
                    if send_datetime_update(device):
                        last_datetime_update = current_time
                    else:
                        # Send failed, likely disconnected
                        print("✗ DateTime send failed, keyboard may be disconnected")
                        print("Waiting for keyboard to reconnect...\n")
                        try:
                            device.close()
                        except:
                            pass
                        device = None
                        last_volume = None
                        last_media = None
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
                last_media = None

    except KeyboardInterrupt:
        print("\n\nStopping keyboard monitor...")
    finally:
        if device is not None:
            try:
                device.close()
            except:
                pass


if __name__ == "__main__":
    main()
