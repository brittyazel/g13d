//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef G13_LCD_HPP
#define G13_LCD_HPP

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
        G13_Device& m_keypad;
        unsigned char image_buf[G13_LCD_BUF_SIZE + 8]{};
        unsigned cursor_row;
        unsigned cursor_col;
        int text_mode;

        explicit G13_LCD(G13_Device& keypad);

        void Image(const unsigned char* data, int size) const;
        void image_send() const;
        void image_clear();
        static unsigned image_byte_offset(unsigned row, unsigned col);
        void WriteChar(char c, unsigned int row = -1, unsigned int col = -1);
        void WriteString(const char* str);
        void WritePos(int row, int col);
    };
}
#endif // G13_LCD_HPP
