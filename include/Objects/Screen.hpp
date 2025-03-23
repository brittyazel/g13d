//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef Screen_HPP
#define Screen_HPP

#include <string>

namespace G13 {
    class Device;

    constexpr size_t SCREEN_BUFFER_SIZE = 0x3c0;
    constexpr size_t SCREEN_COLUMNS = 160;
    constexpr size_t SCREEN_ROWS = 48;
    constexpr size_t SCREEN_BYTES_PER_ROW = SCREEN_COLUMNS / 8;
    constexpr size_t SCREEN_BUF_SIZE = SCREEN_ROWS * SCREEN_BYTES_PER_ROW;
    constexpr size_t SCREEN_TEXT_CHAR_HEIGHT = 8;
    constexpr size_t SCREEN_TEXT_ROWS = 160 / SCREEN_TEXT_CHAR_HEIGHT;

    class Screen {
    public:
        explicit Screen(Device& keypad);

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
        void ScreenWrite(const unsigned char* data, size_t size) const;
        void ScreenWriteFile(const std::string& filename) const;

        // Setters
        void setTextMode(int new_text_mode);

    private:
        Device& m_keypad;
        unsigned char image_buf[SCREEN_BUF_SIZE + 8]{};
        unsigned cursor_row;
        unsigned cursor_col;
        int text_mode;
    };
}

#endif // Screen_HPP
