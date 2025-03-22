//
// Created by Britt Yazel on 03-16-2025.
//

#include "font.hpp"
#include "main.hpp"
#include "lcd.hpp"
#include "log.hpp"

#include <fstream>

namespace G13 {
    G13_LCD::G13_LCD(G13_Device& keypad) : m_keypad(keypad), cursor_row(0), cursor_col(0), text_mode(0) {}

    void G13_LCD::setTextMode(const int new_text_mode) {
        text_mode = new_text_mode;
    }

    // Image handling
    void G13_LCD::Image(const unsigned char* data, const int size) const {
        LcdWrite(data, size);
    }

    void G13_LCD::image_send() const {
        Image(image_buf, G13_LCD_BUF_SIZE);
    }

    void G13_LCD::image_clear() {
        memset(image_buf, 0, G13_LCD_BUF_SIZE);
    }

    unsigned G13_LCD::image_byte_offset(const unsigned row, const unsigned col) {
        return col + row / 8 * G13_LCD_BYTES_PER_ROW * 8;
    }

    // Text handling
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

    // File handling
    void G13_LCD::LcdWrite(const unsigned char* data, const size_t size) const {
        if (size != G13_LCD_BUFFER_SIZE) {
            G13_LOG(
                log4cpp::Priority::ERROR << "Invalid LCD data size " << size << ", should be " << G13_LCD_BUFFER_SIZE);
            return;
        }

        unsigned char buffer[G13_LCD_BUFFER_SIZE + 32] = {};
        buffer[0] = 0x03;
        memcpy(buffer + 32, data, G13_LCD_BUFFER_SIZE);

        int transferred = 0;
        const int error = libusb_interrupt_transfer(m_keypad.getHandlePtr(), LIBUSB_ENDPOINT_OUT | G13_LCD_ENDPOINT,
                                                    buffer, G13_LCD_BUFFER_SIZE + 32, &transferred, 1000);

        if (error) {
            G13_LOG(
                log4cpp::Priority::ERROR << "Error when transferring image: " << G13_Device::DescribeLibusbErrorCode(
                    error) << ", " << transferred << " bytes written");
        }
    }

    void G13_LCD::LcdWriteFile(const std::string& filename) const {
        std::ifstream filestr(filename, std::ios::binary);
        if (!filestr) {
            G13_ERR("Failed to open file: " << filename);
            return;
        }

        filestr.seekg(0, std::ios::end);
        const std::streamsize size = filestr.tellg();
        filestr.seekg(0, std::ios::beg);

        if (size != G13_LCD_BUFFER_SIZE) {
            G13_ERR("Invalid file size: " << size << ", should be " << G13_LCD_BUFFER_SIZE);
            return;
        }

        std::vector<char> buffer(size);
        filestr.read(buffer.data(), size);
        filestr.close();

        LcdWrite(reinterpret_cast<const unsigned char*>(buffer.data()), size);
    }
}
