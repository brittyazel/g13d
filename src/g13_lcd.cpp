//
// Created by Britt Yazel on 03-16-2025.
//

/*
         pixels are mapped rather strangely for G13 buffer...

          byte 0 contains column 0 / row 0 - 7
          byte 1 contains column 1 / row 0 - 7

         so the masks for each pixel are laid out as below
   (ByteOffset.PixelMask)

         00.01 01.01 02.01 ...
         00.02 01.02 02.02 ...
         00.04 01.04 02.04 ...
         00.08 01.08 02.08 ...
         00.10 01.10 02.10 ...
         00.20 01.20 02.20 ...
         00.40 01.40 02.40 ...
         00.80 01.80 02.80 ...
         A0.01 A1.01 A2.01 ...
 */


#include "g13_main.hpp"
#include "g13_lcd.hpp"
#include "g13_log.hpp"

namespace G13 {
    G13_LCD::G13_LCD(G13_Device& keypad) : m_keypad(keypad) {
        cursor_col = 0;
        cursor_row = 0;
        text_mode = 0;
    }

    void G13_LCD::image_send() const {
        Image(image_buf, G13_LCD_BUF_SIZE);
    }

    void G13_LCD::image_clear() {
        memset(image_buf, 0, G13_LCD_BUF_SIZE);
    }

    void G13_LCD::Image(const unsigned char* data, const int size) const {
        m_keypad.LcdWrite(data, size);
    }

    unsigned G13_LCD::image_byte_offset(const unsigned row, const unsigned col) {
        return col + row / 8 * G13_LCD_BYTES_PER_ROW * 8;
    }

    void G13_LCD::WritePos(const int row, const int col) {
        cursor_row = row;
        cursor_col = col;
        if (cursor_col >= G13_LCD_COLUMNS) {
            cursor_col = 0;
        }
        if (cursor_row >= G13_LCD_TEXT_ROWS) {
            cursor_row = 0;
        }
    }

    void G13_LCD::WriteChar(const char c, unsigned int row, unsigned int col) {
        if (row == static_cast<unsigned int>(-1)) {
            row = cursor_row;
            col = cursor_col;
            cursor_col += m_keypad.getCurrentFontRef().width();
            if (cursor_col >= G13_LCD_COLUMNS) {
                cursor_col = 0;
                if (++cursor_row >= G13_LCD_TEXT_ROWS) {
                    cursor_row = 0;
                }
            }
        }

        const unsigned offset = image_byte_offset(row * G13_LCD_TEXT_CHAR_HEIGHT, col);

        if (text_mode) {
            memcpy(&image_buf[offset], &m_keypad.getCurrentFontRef().char_data(c).bits_inverted,
                   m_keypad.getCurrentFontRef().width());
        }
        else {
            memcpy(&image_buf[offset], &m_keypad.getCurrentFontRef().char_data(c).bits_regular,
                   m_keypad.getCurrentFontRef().width());
        }
    }

    void G13_LCD::WriteString(const char* str) {
        G13_OUT("writing \"" << str << "\"");
        while (*str) {
            if (*str == '\n') {
                cursor_col = 0;
                if (++cursor_row >= G13_LCD_TEXT_ROWS) {
                    cursor_row = 0;
                }
            }
            else if (*str == '\t') {
                cursor_col += 4 - cursor_col % 4;
                if (++cursor_col >= G13_LCD_COLUMNS) {
                    cursor_col = 0;
                    if (++cursor_row >= G13_LCD_TEXT_ROWS) {
                        cursor_row = 0;
                    }
                }
            }
            else {
                WriteChar(*str);
            }
            ++str;
        }
        image_send();
    }
}
