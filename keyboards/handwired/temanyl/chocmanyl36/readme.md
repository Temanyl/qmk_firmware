# Chocmanyl36

A 36-key ortholinear keyboard with ST7789 SPI display featuring seasonal animations, interactive games, and a modular object-based rendering system.

## Quick Start

```bash
# Build and flash firmware
make handwired/temanyl/chocmanyl36:default:flash

# Run companion script for full functionality
python3 keyboard_monitor.py
```

## Features

- **Seasonal Animations**: Dynamic scenes that change throughout the year
- **Special Events**: Halloween, Christmas, New Year's Eve with unique animations
- **Interactive Games**: Doodle Jump and Tetris with high score tracking
- **Live Display**: System volume, media info, and date/time
- **Modular Architecture**: Object-oriented design for easy customization

## Documentation

Complete documentation is available in `keymaps/default/docs/`:

- **[README.md](keymaps/default/docs/README.md)** - Complete architecture and system overview
- **[SEASONS.md](keymaps/default/docs/SEASONS.md)** - How to add new seasons and seasonal events
- **[ANIMATIONS.md](keymaps/default/docs/ANIMATIONS.md)** - Creating smooth animations
- **[OBJECTS.md](keymaps/default/docs/OBJECTS.md)** - Creating reusable drawable objects
- **[HIGHSCORE_README.md](keymaps/default/HIGHSCORE_README.md)** - Game high score system

## Hardware

- **MCU**: RP2040
- **Display**: ST7789 SPI (240x135 pixels)
- **Backlight**: PWM controlled
- **Layout**: 36-key ortholinear with Choc switches

## License

GPL v2 or later. See individual files for copyright details.
