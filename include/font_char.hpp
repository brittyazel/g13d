//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef FONT_CHAR_HPP
#define FONT_CHAR_HPP

#include <cstring>

namespace G13 {
    class G13_FontChar {
    public:
        static constexpr int CHAR_BUF_SIZE = 8;

        enum FONT_FLAGS { FF_ROTATE = 0x01 };

        G13_FontChar() {
            memset(bits_regular, 0, CHAR_BUF_SIZE);
            memset(bits_inverted, 0, CHAR_BUF_SIZE);
        }

        void SetCharacter(const unsigned char* data, unsigned int width, unsigned flags);
        unsigned char bits_regular[CHAR_BUF_SIZE]{};
        unsigned char bits_inverted[CHAR_BUF_SIZE]{};
    };
}
#endif
