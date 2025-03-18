//
// Created by Britt Yazel on 03-16-2025.
//

#include <string>

#include "g13_font.hpp"

namespace G13 {
    G13_Font::G13_Font() : m_name("default"), m_width(8) {}
    G13_Font::G13_Font(std::string name, const unsigned int width) : m_name(std::move(name)), m_width(width) {}

    const std::string& G13_Font::name() const {
        return m_name;
    }

    unsigned int G13_Font::width() const {
        return m_width;
    }

    const G13_FontChar& G13_Font::char_data(const unsigned int x) const {
        return m_chars[x];
    }
}
