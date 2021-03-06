# Adafruit SHARP Memory Display - Fast Hardware SPI [![Build Status](https://github.com/adafruit/Adafruit_SHARP_Memory_Display/workflows/Arduino%20Library%20CI/badge.svg)](https://github.com/adafruit/Adafruit_SHARP_Memory_Display/actions)

This is the Adafruit SHARP Memory Display Arduino Library for Arduino

- It has been modified and optimized for the Arduino Nano 33 Sense BLE.
- It uses Hardware SPI (Arduino libraries) instead of bit bashed SPI.
- It has been tested and achieves ~40ms LCD refresh time instead of ~4 seconds of the original code.
- Modifed by Craig Smith, June 2020


Tested and works great with the Adafruit SHARP Memory Display Breakout Board. Pick one up today in the adafruit shop!
 http://www.adafruit.com/products/1393

Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!

# Dependencies
  These displays use SPI to communicate, 3 pins are required to  
interface

* [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)

# Contributing

Contributions are welcome! Please read our [Code of Conduct](https://github.com/adafruit/Adafruit_SHARP_Memory_Display/blob/master/CODE_OF_CONDUCT.md>)
before contributing to help this project stay welcoming.

## Documentation and doxygen
Documentation is produced by doxygen. Contributions should include documentation for any new code added.

Some examples of how to use doxygen can be found in these guide pages:

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen-tips

Written by Limor Fried & Kevin Townsend for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution

To install, use the Arduino Library Manager and search for "Adafruit SHARP Memory Display" and install the library.
