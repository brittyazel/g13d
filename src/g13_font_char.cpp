//
// Created by Britt Yazel on 03-16-2025.
//

#include <memory>

#include "g13_font_char.hpp"
#include "g13_device.hpp"

namespace G13 {
    void G13_FontChar::SetCharacter(const unsigned char* data, const unsigned int width, const unsigned flags) {
        unsigned char* dest = bits_regular;
        memset(dest, 0, CHAR_BUF_SIZE);
        if (flags & FF_ROTATE) {
            for (unsigned int x = 0; x < width; x++) {
                const unsigned char x_mask = static_cast<unsigned char>(1u) << x;
                for (unsigned int y = 0; y < 8; y++) {
                    // unsigned char y_mask = 1 << y;
                    if (data[y] & x_mask) {
                        dest[x] |= 1u << y;
                    }
                }
            }
        }
        else {
            memcpy(dest, data, width);
        }
        for (unsigned int x = 0; x < width; x++) {
            bits_inverted[x] = ~dest[x];
        }
    }
}
