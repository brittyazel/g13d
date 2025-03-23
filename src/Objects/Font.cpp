//
// Created by Britt Yazel on 03-16-2025.
//

#include <string>

#include "Objects/Font.hpp"

namespace G13 {
    Font::Font(std::string name, const unsigned int width) : m_name(std::move(name)), m_width(width) {}

    const std::string& Font::name() const {
        return m_name;
    }

    unsigned int Font::width() const {
        return m_width;
    }

    const FontCharacter& Font::char_data(const unsigned int x) const {
        return m_chars[x];
    }
}
