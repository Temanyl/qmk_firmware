# Scotto34

![Scotto34](https://i.imgur.com/BkpEBQjh.jpg)

A 33 ortholinear board with a 7u spacebar. Case files available [here](https://github.com/joe-scotto/keyboards.git).

-   Keyboard Maintainer: [Joe Scotto](https://github.com/joe-scotto)
-   Hardware Supported: RP2040
-   Hardware Availability: [Amazon](https://amazon.com)

# Compiling

Make example for this keyboard (after setting up your build environment):

    make handwired/jscotto/scotto34:default

Flashing example for this keyboard:

    make handwired/jscotto/scotto34:default

# Bootloader

Uses [bootmagic](https://github.com/qmk/qmk_firmware/blob/master/docs/feature_bootmagic.md) allowing you to hold the top left key (0, 0) when plugging the board in to enter bootloader mode.

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).
