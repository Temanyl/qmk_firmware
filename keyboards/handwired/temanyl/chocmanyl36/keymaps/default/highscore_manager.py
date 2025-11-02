#!/usr/bin/env python3
"""
High Score Manager for Doodle Jump QMK Game

This script communicates with the keyboard via Raw HID to manage high scores.
It stores scores in a JSON file and sends responses back to the keyboard.

Requirements:
  pip install hid

Usage:
  python3 highscore_manager.py
"""

import hid
import json
import os
import sys
import time
from pathlib import Path

# HID Configuration - Update these to match your keyboard
VENDOR_ID = 0xFEED  # From keyboards/handwired/temanyl/chocmanyl36/info.json
PRODUCT_ID = 0x1805  # From keyboards/handwired/temanyl/chocmanyl36/info.json
USAGE_PAGE = 0xFF60  # QMK Raw HID usage page
USAGE = 0x61         # QMK Raw HID usage

# Message types (0x10-0x13 to avoid conflict with display HID messages)
MSG_SCORE_SUBMIT = 0x10  # Keyboard sends score
MSG_ENTER_NAME = 0x11    # Computer asks for name (score made top 10)
MSG_SHOW_SCORES = 0x12   # Computer sends top 10 list
MSG_NAME_SUBMIT = 0x13   # Keyboard sends name + score

# File paths
SCRIPT_DIR = Path(__file__).parent
SCORES_FILE = SCRIPT_DIR / "highscores.json"

class HighScoreManager:
    def __init__(self):
        self.device = None
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
                print(f"Error loading scores: {e}")
                return []
        return []

    def save_scores(self):
        """Save high scores to JSON file"""
        try:
            with open(SCORES_FILE, 'w') as f:
                json.dump(self.scores, f, indent=2)
            print(f"Saved {len(self.scores)} scores to {SCORES_FILE}")
        except Exception as e:
            print(f"Error saving scores: {e}")

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

    def connect(self):
        """Connect to the keyboard via Raw HID"""
        print(f"Looking for keyboard (VID: 0x{VENDOR_ID:04X}, PID: 0x{PRODUCT_ID:04X})...")

        try:
            # Find the Raw HID interface
            devices = hid.enumerate(VENDOR_ID, PRODUCT_ID)

            for dev_info in devices:
                # QMK Raw HID typically uses usage_page 0xFF60
                if dev_info.get('usage_page') == USAGE_PAGE or dev_info.get('usage') == USAGE:
                    print(f"Found Raw HID interface: {dev_info['path']}")
                    self.device = hid.Device(path=dev_info['path'])
                    print("Connected!")
                    return True

            # Fallback: try opening first available interface
            if not self.device and devices:
                print("Raw HID interface not found by usage_page, trying first interface...")
                self.device = hid.Device(VENDOR_ID, PRODUCT_ID)
                print("Connected!")
                return True

            print("No suitable HID interface found.")
            return False

        except Exception as e:
            print(f"Error connecting: {e}")
            return False

    def send_enter_name(self, rank):
        """Send ENTER_NAME message to keyboard"""
        if not self.device:
            return

        data = [0] * 32  # Raw HID reports are 32 bytes
        data[0] = MSG_ENTER_NAME
        data[1] = rank

        try:
            self.device.write(data)
            print(f"Sent ENTER_NAME (rank: {rank})")
        except Exception as e:
            print(f"Error sending ENTER_NAME: {e}")

    def send_show_scores(self):
        """Send SHOW_SCORES message with top 10 list"""
        if not self.device:
            return

        data = [0] * 32
        data[0] = MSG_SHOW_SCORES

        # Pack up to 10 scores (each: 3 chars + 2 bytes score = 5 bytes)
        offset = 1
        for i, entry in enumerate(self.scores[:10]):
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
            self.device.write(data)
            print(f"Sent SHOW_SCORES ({len(self.scores)} entries)")
        except Exception as e:
            print(f"Error sending SHOW_SCORES: {e}")

    def process_message(self, data):
        """Process incoming message from keyboard"""
        if len(data) == 0:
            return

        msg_type = data[0]

        if msg_type == MSG_SCORE_SUBMIT:
            # Parse score (2 bytes, big-endian)
            score = (data[1] << 8) | data[2]
            print(f"\n=== Score Received: {score} ===")

            # Check if it makes top 10
            rank = self.check_score(score)

            if rank >= 0:
                print(f"Score made top 10! Rank: {rank + 1}")
                self.send_enter_name(rank)
            else:
                print("Score didn't make top 10")
                self.send_show_scores()

        elif msg_type == MSG_NAME_SUBMIT:
            # Parse name (3 chars) and score (2 bytes)
            name = ''.join(chr(data[i]) for i in range(1, 4))
            score = (data[4] << 8) | data[5]
            print(f"\n=== Name + Score Received: {name} - {score} ===")

            # Add to scores
            rank = self.add_score(name, score)
            print(f"Added to high scores at rank {rank + 1}")

            # Send updated scores
            self.send_show_scores()

    def run(self):
        """Main loop"""
        if not self.connect():
            print("Failed to connect to keyboard")
            print("\nMake sure:")
            print("1. Keyboard is plugged in")
            print("2. RAW_ENABLE = yes in rules.mk")
            print("3. Firmware has Raw HID callbacks implemented")
            return

        print("\n=== High Score Manager Running ===")
        print("Waiting for scores from keyboard...")
        print("Press Ctrl+C to quit\n")

        try:
            while True:
                # Read from device (blocking with timeout)
                data = self.device.read(32, timeout_ms=100)

                if data:
                    self.process_message(data)

        except KeyboardInterrupt:
            print("\nShutting down...")
        except Exception as e:
            print(f"\nError: {e}")
        finally:
            if self.device:
                self.device.close()

def main():
    manager = HighScoreManager()

    # Print current high scores
    if manager.scores:
        print("Current High Scores:")
        print("-" * 30)
        for i, entry in enumerate(manager.scores):
            print(f"{i+1:2d}. {entry['name']:3s}  {entry['score']:5d}")
        print("-" * 30)
        print()
    else:
        print("No high scores yet!\n")

    manager.run()

if __name__ == '__main__':
    main()
