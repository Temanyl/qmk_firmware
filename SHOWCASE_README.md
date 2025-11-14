# Display Showcase Scripts

This directory contains two Python scripts for controlling and showcasing the chocmanyl36 keyboard display.

## Scripts

### 1. `display_showcase_full.py` - Comprehensive Feature Showcase

A comprehensive automated showcase that demonstrates all display features in sequence.

**What it showcases:**

1. **Sun Movement** (Autumn sunny day, dawn to dusk)
   - Shows sun position from 6 AM to 8 PM
   - Demonstrates realistic sun arc across the sky

2. **Moon Phases** (Autumn clear nights)
   - All 8 lunar phases across the month
   - New Moon → Waxing Crescent → First Quarter → Waxing Gibbous → Full Moon → Waning Gibbous → Last Quarter → Waning Crescent

3. **All 4 Seasons with All Weather Conditions**
   - Winter: Sunny, Cloudy, Overcast, Light/Medium/Heavy Snow
   - Spring: Sunny, Cloudy, Overcast, Light/Medium/Heavy Rain
   - Summer: Sunny, Cloudy, Overcast, Light/Medium/Heavy Rain
   - Fall: Sunny, Cloudy, Overcast, Light/Medium/Heavy Rain

4. **Halloween** (Medium Rain)
   - Spooky rainy Halloween night

5. **Christmas** (Heavy Snow)
   - White Christmas with heavy snowfall

6. **Easter** (All Weather Conditions)
   - Easter scenes with all 6 weather types

7. **Special Events at Different Times**
   - Halloween: Dawn, Noon, Dusk, Midnight
   - Christmas: Morning, Noon, Evening, Night

**Duration:** Approximately 3-4 minutes

**Usage:**
```bash
python3 display_showcase_full.py
```

**Timing:**
- Quick transitions: 3 seconds per frame
- Standard segments: 8 seconds per scene

---

### 2. `display_config.py` - Manual Configuration Tool

A flexible configuration tool that lets you manually set any display parameter. All parameters are optional - only send what you specify.

**Features:**
- Set date (YYYY-MM-DD format)
- Set time (HH:MM or HH:MM:SS format)
- Set weather (9 different conditions with intensities)
- Set media text (custom message on display)

**Usage Examples:**

```bash
# Set date only (Easter)
python3 display_config.py --date 2025-04-10

# Set date and time (Halloween midnight)
python3 display_config.py --date 2025-10-31 --time 23:59

# Set weather only (heavy snow)
python3 display_config.py --weather snow-heavy

# Set everything at once (Snowy Halloween!)
python3 display_config.py --date 2025-10-31 --time 20:00 --weather snow-heavy --text "Spooky Snow Halloween!"

# White Christmas!
python3 display_config.py --date 2025-12-25 --time 14:00 --weather snow-medium --text "White Christmas!"

# Just change the text
python3 display_config.py --text "Now Playing: Thriller"

# Clear media text
python3 display_config.py --text ""

# List all available weather options
python3 display_config.py --list-weather
```

**Available Weather Options:**
- `sunny` - Clear sky
- `rain-light` - Light rain/drizzle
- `rain`, `rain-medium` - Medium rain (default)
- `rain-heavy` - Heavy rain/storm
- `snow-light` - Light snowfall
- `snow`, `snow-medium` - Medium snow (default)
- `snow-heavy` - Heavy snowfall/blizzard
- `cloudy`, `partly-cloudy` - Partly cloudy (2 clouds)
- `overcast` - Overcast sky (5 clouds)

---

## Requirements

Both scripts require the `hidapi` library:

```bash
pip3 install hidapi
```

If you encounter import errors:
```bash
pip3 uninstall hid hidapi
pip3 install hidapi
```

---

## USB Configuration

Both scripts use these default USB identifiers:
- **Vendor ID:** `0xFEED`
- **Product ID:** `0x0000`
- **Usage Page:** `0xFF60`
- **Usage:** `0x0061`

If your keyboard uses different VID/PID, edit the constants at the top of each script.

---

## Special Event Dates

The firmware recognizes these special event periods:

- **Halloween:** October 28 - November 3
- **Christmas:** December 15 - December 31
- **Easter:** Specific date (example: April 10)

---

## Testing Specific Scenes

Use `display_config.py` to set up specific test scenarios:

### Halloween Scenes
```bash
# Sunny Halloween
python3 display_config.py --date 2025-10-31 --time 20:00 --weather sunny

# Rainy Halloween
python3 display_config.py --date 2025-10-31 --time 20:00 --weather rain-medium

# Snowy Halloween
python3 display_config.py --date 2025-10-31 --time 20:00 --weather snow-heavy
```

### Christmas Scenes
```bash
# Sunny Christmas
python3 display_config.py --date 2025-12-25 --time 14:00 --weather sunny

# White Christmas (light snow)
python3 display_config.py --date 2025-12-25 --time 14:00 --weather snow-light

# White Christmas (heavy snow)
python3 display_config.py --date 2025-12-25 --time 14:00 --weather snow-heavy
```

### Seasonal Scenes
```bash
# Winter morning
python3 display_config.py --date 2025-01-15 --time 08:00 --weather snow-medium

# Spring afternoon with butterflies
python3 display_config.py --date 2025-04-15 --time 14:00 --weather sunny

# Summer evening (fireflies at 8 PM)
python3 display_config.py --date 2025-07-20 --time 20:00 --weather sunny

# Fall rainy day
python3 display_config.py --date 2025-10-10 --time 16:00 --weather rain-medium
```

---

## Related Scripts

- `keyboard_monitor.py` - Continuously monitors system volume, media playback, and sends live weather updates
- `display_showcase.py` - Original showcase script (simpler version)

---

## Notes

- The keyboard firmware uses **hard-coded date override** mode for testing. Check `display/display.h` for the `HARDCODE_DATE_TIME` setting.
- When `HARDCODE_DATE_TIME` is enabled in firmware, HID time updates from these scripts may be ignored depending on the `IGNORE_HID_TIME_UPDATES` setting.
- Weather and media text updates always work regardless of date override settings.
