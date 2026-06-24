#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8x8lib.h>

// SH1106 128x64 OLED via u8x8 (5x8 font -> 16 cols x 8 rows)
class Display {
public:
  Display() : u8x8(U8X8_PIN_NONE) {}

  void begin() {
    u8x8.begin();
    u8x8.setPowerSave(0);
    u8x8.setFont(u8x8_font_5x8_f);
    u8x8.clearDisplay();
  }

  void clear() {
    u8x8.clearDisplay();
  }

  void setCursor(uint8_t col, uint8_t row) {
    if (col > 15) {
      col = 15;
    }
    if (row > 7) {
      row = 7;
    }
    u8x8.setCursor(col, row);
  }

  void print(const char *str) {
    u8x8.print(str);
  }

  void print(char c) {
    char buf[2] = {c, '\0'};
    u8x8.print(buf);
  }

  void print(int n) {
    u8x8.print(n);
  }

  void print(unsigned int n) {
    u8x8.print(n);
  }

  void print(long n) {
    u8x8.print(n);
  }

  void print(unsigned long n) {
    u8x8.print(n);
  }

  void print(const String &s) {
    u8x8.print(s.c_str());
  }

private:
  U8X8_SH1106_128X64_NONAME_HW_I2C u8x8;
};

#endif
