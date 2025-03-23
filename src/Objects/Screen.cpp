//
// Created by Britt Yazel on 03-16-2025.
//

#include "Objects/Font.hpp"
#include "main.hpp"
#include "Objects/Screen.hpp"
#include "log.hpp"

#include <fstream>

namespace G13 {
    Screen::Screen(Device& keypad) : m_keypad(keypad), cursor_row(0), cursor_col(0), text_mode(0) {}

    void Screen::setTextMode(const int new_text_mode) {
        text_mode = new_text_mode;
    }

    // Image handling
    void Screen::Image(const unsigned char* data, const int size) const {
        ScreenWrite(data, size);
    }

    void Screen::image_send() const {
        Image(image_buf, SCREEN_BUF_SIZE);
    }

    void Screen::image_clear() {
        memset(image_buf, 0, SCREEN_BUF_SIZE);
    }

    unsigned Screen::image_byte_offset(const unsigned row, const unsigned col) {
        return col + row / 8 * SCREEN_BYTES_PER_ROW * 8;
    }

    // Text handling
    void Screen::WriteChar(const char c, unsigned int row, unsigned int col) {
        if (row == static_cast<unsigned int>(-1)) {
            row = cursor_row;
            col = cursor_col;
            cursor_col += m_keypad.getCurrentFontRef().width();
            if (cursor_col >= SCREEN_COLUMNS) {
                cursor_col = 0;
                if (++cursor_row >= SCREEN_TEXT_ROWS) {
                    cursor_row = 0;
                }
            }
        }

        const unsigned offset = image_byte_offset(row * SCREEN_TEXT_CHAR_HEIGHT, col);

        if (text_mode) {
            memcpy(&image_buf[offset], &m_keypad.getCurrentFontRef().char_data(c).bits_inverted,
                   m_keypad.getCurrentFontRef().width());
        }
        else {
            memcpy(&image_buf[offset], &m_keypad.getCurrentFontRef().char_data(c).bits_regular,
                   m_keypad.getCurrentFontRef().width());
        }
    }

    void Screen::WriteString(const char* str) {
        OUT("writing \"" << str << "\"");
        while (*str) {
            if (*str == '\n') {
                cursor_col = 0;
                if (++cursor_row >= SCREEN_TEXT_ROWS) {
                    cursor_row = 0;
                }
            }
            else if (*str == '\t') {
                cursor_col += 4 - cursor_col % 4;
                if (++cursor_col >= SCREEN_COLUMNS) {
                    cursor_col = 0;
                    if (++cursor_row >= SCREEN_TEXT_ROWS) {
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

    void Screen::WritePos(const int row, const int col) {
        cursor_row = row;
        cursor_col = col;
        if (cursor_col >= SCREEN_COLUMNS) {
            cursor_col = 0;
        }
        if (cursor_row >= SCREEN_TEXT_ROWS) {
            cursor_row = 0;
        }
    }

    // File handling
    void Screen::ScreenWrite(const unsigned char* data, const size_t size) const {
        if (size != SCREEN_BUFFER_SIZE) {
            LOG(
                log4cpp::Priority::ERROR << "Invalid screen data size " << size << ", should be " << SCREEN_BUFFER_SIZE);
            return;
        }

        unsigned char buffer[SCREEN_BUFFER_SIZE + 32] = {};
        buffer[0] = 0x03;
        memcpy(buffer + 32, data, SCREEN_BUFFER_SIZE);

        int transferred = 0;
        const int error = libusb_interrupt_transfer(m_keypad.getHandlePtr(), LIBUSB_ENDPOINT_OUT | SCREEN_ENDPOINT,
                                                    buffer, SCREEN_BUFFER_SIZE + 32, &transferred, 1000);

        if (error) {
            LOG(
                log4cpp::Priority::ERROR << "Error when transferring image: " << Device::DescribeLibusbErrorCode(
                    error) << ", " << transferred << " bytes written");
        }
    }

    void Screen::ScreenWriteFile(const std::string& filename) const {
        std::ifstream filestr(filename, std::ios::binary);
        if (!filestr) {
            ERR("Failed to open file: " << filename);
            return;
        }

        filestr.seekg(0, std::ios::end);
        const std::streamsize size = filestr.tellg();
        filestr.seekg(0, std::ios::beg);

        if (size != SCREEN_BUFFER_SIZE) {
            ERR("Invalid file size: " << size << ", should be " << SCREEN_BUFFER_SIZE);
            return;
        }

        std::vector<char> buffer(size);
        filestr.read(buffer.data(), size);
        filestr.close();

        ScreenWrite(reinterpret_cast<const unsigned char*>(buffer.data()), size);
    }
}
