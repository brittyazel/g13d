//
// Created by khampf on 07-05-2020.
//

#ifndef G13_G13_LCD_HPP
#define G13_G13_LCD_HPP

#include <cstring>

namespace G13 {
class G13_Device;

const size_t G13_LCD_BUFFER_SIZE = 0x3c0;
const size_t G13_LCD_COLUMNS = 160;
const size_t G13_LCD_ROWS = 48;
const size_t G13_LCD_BYTES_PER_ROW = G13_LCD_COLUMNS / 8;
const size_t G13_LCD_BUF_SIZE = G13_LCD_ROWS * G13_LCD_BYTES_PER_ROW;
const size_t G13_LCD_TEXT_CHEIGHT = 8;
const size_t G13_LCD_TEXT_ROWS = 160 / G13_LCD_TEXT_CHEIGHT;

class G13_LCD {
public:
  explicit G13_LCD(G13_Device &keypad);

  G13_Device &m_keypad;
  unsigned char image_buf[G13_LCD_BUF_SIZE + 8];
  unsigned cursor_row;
  unsigned cursor_col;
  int text_mode;

  void Image(unsigned char *data, int size);
  void image_send() { Image(image_buf, G13_LCD_BUF_SIZE); }

  // void image_test(int x, int y);
  void image_clear() { memset(image_buf, 0, G13_LCD_BUF_SIZE); }

  static unsigned image_byte_offset(unsigned row, unsigned col) {
    return col + (row / 8) * G13_LCD_BYTES_PER_ROW * 8;
  }

  // void image_setpixel(unsigned row, unsigned col);
  // void image_clearpixel(unsigned row, unsigned col);

  void WriteChar(char c, unsigned int row = -1, unsigned int col = -1);
  void WriteString(const char *str);
  void WritePos(int row, int col);
};
} // namespace G13
#endif // G13_G13_LCD_HPP
