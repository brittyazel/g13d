//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef LCD_HPP
#define LCD_HPP

#include <string>

namespace G13 {
    class G13_Device;

    constexpr size_t G13_LCD_BUFFER_SIZE = 0x3c0;
    constexpr size_t G13_LCD_COLUMNS = 160;
    constexpr size_t G13_LCD_ROWS = 48;
    constexpr size_t G13_LCD_BYTES_PER_ROW = G13_LCD_COLUMNS / 8;
    constexpr size_t G13_LCD_BUF_SIZE = G13_LCD_ROWS * G13_LCD_BYTES_PER_ROW;
    constexpr size_t G13_LCD_TEXT_CHAR_HEIGHT = 8;
    constexpr size_t G13_LCD_TEXT_ROWS = 160 / G13_LCD_TEXT_CHAR_HEIGHT;

    class G13_LCD {
    public:
        explicit G13_LCD(G13_Device& keypad);

        // Image handling
        void Image(const unsigned char* data, int size) const;
        void image_send() const;
        void image_clear();
        static unsigned image_byte_offset(unsigned row, unsigned col);

        // Text handling
        void WriteChar(char c, unsigned int row = -1, unsigned int col = -1);
        void WriteString(const char* str);
        void WritePos(int row, int col);

        // File handling
        void LcdWrite(const unsigned char* data, size_t size) const;
        void LcdWriteFile(const std::string& filename) const;

        // Setters
        void setTextMode(int new_text_mode);

    private:
        G13_Device& m_keypad;
        unsigned char image_buf[G13_LCD_BUF_SIZE + 8]{};
        unsigned cursor_row;
        unsigned cursor_col;
        int text_mode;
    };
}

#endif // LCD_HPP
