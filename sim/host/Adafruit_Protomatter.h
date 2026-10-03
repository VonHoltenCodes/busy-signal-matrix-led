// Host stand-in: gBlit() is never called off the board.
#pragma once
#include <stdint.h>
class Adafruit_Protomatter {
 public:
  void drawPixel(int16_t, int16_t, uint16_t) {}
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
};
