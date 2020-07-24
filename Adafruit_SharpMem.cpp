/*********************************************************************
This is an Arduino library for our Monochrome SHARP Memory Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1393

These displays use SPI to communicate, 3 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution

- This library has been optimised for the Nano 33 Sense BLE board by Craig Smith
- It uses Hardware SPI instead of bit bashing in order to achieve screen redraw times about 100 times faster.
- For a 320x240 LCD, redraw time is approx 40ms instead of 4 seconds!
- Completed June 2020
*********************************************************************/

#include "Adafruit_SharpMem.h"
#include "SPI.h"

/* If enabled, this creates a data packet consistaning of screen buffer and all addressing bytes
before sending the entire packet over Hardware SPI.
This increases LCD refresh time by ~20ms?) but requires additional memory */
#define SPI_BUFFER_ENABLED

/* Select SPI clock frequency */
//#define SPI_FREQUENCY 1000000 // 500kHz, 
//#define SPI_FREQUENCY 2000000 // 2MHz, 57ms for full screen update @ 320x240
#define SPI_FREQUENCY 4000000 // 4MHz, 38ms for full screen update @ 320x240


#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif
#ifndef _swap_uint16_t
#define _swap_uint16_t(a, b)                                                   \
  {                                                                            \
    uint16_t t = a;                                                            \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

/**************************************************************************
Arduino Nano 33 BLE dedicated pins must be used:

// SPI
#define PIN_SPI_MISO  (12u)
#define PIN_SPI_MOSI  (11u)
#define PIN_SPI_SCK   (13u)
#define PIN_SPI_SS    (10u)

 **************************************************************************/

/**************************************************************************
    Sharp Memory Display Connector
    -----------------------------------------------------------------------
    Pin   Function        Notes
    ===   ==============  ===============================
      1   VIN             3.3-5.0V (into LDO supply)
      2   3V3             3.3V out
      3   GND
      4   SCLK            Serial Clock
      5   MOSI            Serial Data Input
      6   CS              Serial Chip Select
      9   EXTMODE         COM Inversion Select (Low = SW clock/serial)
      7   EXTCOMIN        External COM Inversion Signal
      8   DISP            Display On(High)/Off(Low)

 **************************************************************************/

#define SHARPMEM_BIT_WRITECMD (0x80)
#define SHARPMEM_BIT_VCOM (0x40)
#define SHARPMEM_BIT_CLEAR (0x20)
#define TOGGLE_VCOM                                                            \
  do {                                                                         \
    _sharpmem_vcom = _sharpmem_vcom ? 0x00 : SHARPMEM_BIT_VCOM;                \
  } while (0);

#define TOGGLE_VCOM                                                            \
  do {                                                                         \
    _sharpmem_vcom = _sharpmem_vcom ? 0x00 : SHARPMEM_BIT_VCOM;                \
  } while (0);


byte *sharpmem_buffer;
byte *sharpmem_spi_buffer;

uint32_t lcd_width;
uint32_t lcd_height

byte lineNumberLookup[] = 
{
    0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112,
    240, 8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184,
    120, 248, 4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52,
    180, 116, 244, 12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220,
    60, 188, 124, 252, 2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82,
    210, 50, 178, 114, 242, 10, 138, 74, 202, 42, 170, 106, 234, 26, 154,
    90, 218, 58, 186, 122, 250, 6, 134, 70, 198, 38, 166, 102, 230, 22,
    150, 86, 214, 54, 182, 118, 246, 14, 142, 78, 206, 46, 174, 110, 238,
    30, 158, 94, 222, 62, 190, 126, 254, 1, 129, 65, 193, 33, 161, 97,
    225, 17, 145, 81, 209, 49, 177, 113, 241, 9, 137, 73, 201, 41, 169,
    105, 233, 25, 153, 89, 217, 57, 185, 121, 249, 5, 133, 69, 197, 37,
    165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245, 13, 141, 77, 205,
    45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253, 3, 131, 67,
    195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243, 11, 139,
    75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251, 7,
    135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
    15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127,
    255
};

/**
 * @brief Construct a new Adafruit_SharpMem object with software SPI
 *
 * @param clk The clock pin
 * @param mosi The MOSI pin
 * @param cs The display chip select pin - **NOTE** this is ACTIVE HIGH!
 * @param width The display width
 * @param height The display height
 * @param freq The SPI clock frequency desired (unlikely to be that fast in soft
 * spi mode!)
 */
Adafruit_SharpMem::Adafruit_SharpMem(uint8_t clk, uint8_t mosi, uint8_t cs,
                                     uint16_t width, uint16_t height,
                                     uint32_t freq)
    : Adafruit_GFX(width, height) {
  _clk = clk;
  _mosi = mosi;
  _ss = ss;
  lcd_width = width;
  lcd_height = height;
}

/**
 * @brief Construct a new Adafruit_SharpMem object with hardware SPI
 *
 * @param theSPI Pointer to hardware SPI device you want to use
 * @param cs The display chip select pin - **NOTE** this is ACTIVE HIGH!
 * @param width The display width
 * @param height The display height
 * @param freq The SPI clock frequency desired
 */
Adafruit_SharpMem::Adafruit_SharpMem(SPIClass *theSPI, uint8_t cs,
                                     uint16_t width, uint16_t height,
                                     uint32_t freq)
    : Adafruit_GFX(width, height) {
  _cs = cs;
  if (spidev) {
    delete spidev;
  }
  spidev = new Adafruit_SPIDevice(cs, freq, SPI_BITORDER_LSBFIRST, SPI_MODE0,
                                  theSPI);
}

/**
 * @brief Start the driver object, setting up pins and configuring a buffer for
 * the screen contents
 *
 * @return boolean true: success false: failure
 */
boolean Adafruit_SharpMem::begin(void) {
  // Set pin state before direction to make sure they start this way (no
  // glitching)
 
  // Set the vcom bit to a defined state
  _sharpmem_vcom = SHARPMEM_BIT_VCOM;

  sharpmem_buffer = (byte *)malloc((WIDTH * HEIGHT) / 8);

#ifdef SPI_BUFFER_ENABLED  
  sharpmem_spi_buffer = (byte *)malloc(((WIDTH * HEIGHT) / 8)+3+2*HEIGHT+240);
#endif
  if (!sharpmem_buffer)
    return false;

  setRotation(0);
  
  // set up SPI peripheral
  SPI.begin();

  return true;
}

// 1<<n is a costly operation on AVR -- table usu. smaller & faster
static const uint8_t PROGMEM set[] = {1, 2, 4, 8, 16, 32, 64, 128},
                             clr[] = {(uint8_t)~1,  (uint8_t)~2,  (uint8_t)~4,
                                      (uint8_t)~8,  (uint8_t)~16, (uint8_t)~32,
                                      (uint8_t)~64, (uint8_t)~128};

/**************************************************************************/
/*!
    @brief Draws a single pixel in image buffer

    @param[in]  x
                The x position (0 based)
    @param[in]  y
                The y position (0 based)
    @param color The color to set:
    * **0**: Black
    * **1**: White
*/
/**************************************************************************/
void Adafruit_SharpMem::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
    return;

  switch (rotation) {
  case 1:
    _swap_int16_t(x, y);
    x = WIDTH - 1 - x;
    break;
  case 2:
    x = WIDTH - 1 - x;
    y = HEIGHT - 1 - y;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - 1 - y;
    break;
  }

  if (color) {
    sharpmem_buffer[(y * WIDTH + x) / 8] |= pgm_read_byte(&set[x & 7]);
  } else {
    sharpmem_buffer[(y * WIDTH + x) / 8] &= pgm_read_byte(&clr[x & 7]);
  }
}

/**************************************************************************/
/*!
    @brief Gets the value (1 or 0) of the specified pixel from the buffer

    @param[in]  x
                The x position (0 based)
    @param[in]  y
                The y position (0 based)

    @return     1 if the pixel is enabled, 0 if disabled
*/
/**************************************************************************/
uint8_t Adafruit_SharpMem::getPixel(uint16_t x, uint16_t y) {
  if ((x >= _width) || (y >= _height))
    return 0; // <0 test not needed, unsigned

  switch (rotation) {
  case 1:
    _swap_uint16_t(x, y);
    x = WIDTH - 1 - x;
    break;
  case 2:
    x = WIDTH - 1 - x;
    y = HEIGHT - 1 - y;
    break;
  case 3:
    _swap_uint16_t(x, y);
    y = HEIGHT - 1 - y;
    break;
  }

  return sharpmem_buffer[(y * WIDTH + x) / 8] & pgm_read_byte(&set[x & 7]) ? 1
                                                                           : 0;
}

/**************************************************************************/
/*!
    @brief Clears the buffer.
*/
/**************************************************************************/
void Adafruit_SharpMem::clearDisplay() {
  // just clear the buffer, and rely on user doing manual refresh when they want to update the display!
  memset(sharpmem_buffer, 0xff, (WIDTH * HEIGHT) / 8);  
  this.refresh();
}

/**************************************************************************/
/*!
    @brief Renders the contents of the pixel buffer on the LCD
*/
/**************************************************************************/
void Adafruit_SharpMem::refresh(void) {
   uint16_t i; 

#ifdef SPI_BUFFER_ENABLED 
  
  // generate SPI buffer - takes 1.7ms (320x240)  
  sharpmem_spi_buffer[0] = (SHARPMEM_BIT_WRITECMD | _sharpmem_vcom);
  for (i = 0; i < HEIGHT; i++) {
    sharpmem_spi_buffer[(i*2+1)+i*WIDTH/8] = lineNumberLookup[i+1];
    memcpy(sharpmem_spi_buffer+(i*2)+2+(i*WIDTH/8),sharpmem_buffer+(i*(WIDTH/8)), WIDTH/8);
  }
  
  // dump completed array to SPI - at 4MHz, 320x240 it takes 23.3ms to transfer data.
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));  // note: MSBFIRST has no effect, cannot do LSBFIRST
  digitalWrite(_ss, HIGH);
  //SPI.transfer(sharpmem_spi_buffer, (uint16_t)(((i*2+3)+((i+1)*(WIDTH/8)))));
  SPI.transfer(sharpmem_spi_buffer, (uint16_t)(2+2*HEIGHT+(WIDTH*HEIGHT/8)));
  digitalWrite(_ss, LOW); // 
  SPI.endTransaction();
  TOGGLE_VCOM;  
  
#else
  digitalWrite(_ss, HIGH);
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));  // note: MSBFIRST has no effect, cannot do LSBFIRST
  SPI.transfer(SHARPMEM_BIT_WRITECMD | _sharpmem_vcom); // command byte, 8 clocks

  // send line by line
  for (i = 0; i < HEIGHT; i++) {
	SPI.transfer(lineNumberLookup[i+1]); // line address number, 8 clocks - confirmed to be correct
	SPI.transfer(sharpmem_buffer+(i*(WIDTH/8)),WIDTH/8); // send whole line.. data should already be in correct format (LSB first)
	SPI.transfer(0x00);  // dummy data 8 clocks
	
	// for a speedup, could save 240*20us = 5ms if combine linenumber+dummy byte into single transaction.  maybe not worth it?
  }
  
  SPI.transfer(0x00); // dummy data 8 clocks 
  TOGGLE_VCOM;
  SPI.endTransaction();
  digitalWrite(_ss, LOW); 
#endif
}

/**************************************************************************/
/*!
    @brief Clears the display buffer without outputting to the display
*/
/**************************************************************************/
void Adafruit_SharpMem::clearDisplayBuffer() {
  memset(sharpmem_buffer, 0xff, (WIDTH * HEIGHT) / 8);
}
