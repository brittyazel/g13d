//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef G13_FONT_HPP
#define G13_FONT_HPP

#include <string>

#include "g13_font_char.hpp"

namespace G13 {
    class G13_Font {
    public:
        G13_Font();
        explicit G13_Font(std::string name, unsigned int width = 8);

        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] unsigned int width() const;
        [[nodiscard]] const G13_FontChar& char_data(unsigned int x) const;

        template <typename T, size_t size>
        static size_t GetFontCharacterCount(T (&)[size]) {
            return size;
        }

        template <class ARRAY_T, class FLAGST>
        void InstallFont(ARRAY_T& data, FLAGST flags, const int first) {
            for (size_t i = 0; i < GetFontCharacterCount(data); i++) {
                m_chars[i + first].SetCharacter(&data[i][0], m_width, flags);
            }
        }

    protected:
        std::string m_name;
        unsigned int m_width;
        G13_FontChar m_chars[256];
    };
}
#endif
