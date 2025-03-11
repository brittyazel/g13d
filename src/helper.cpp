/*
 *  helper.cpp
 */

/*
 * Copyright (c) 2015, James Fowler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <functional>
#include "helper.hpp"

// *************************************************************************

namespace Helper {
    void string_repr_out::write_on(std::ostream& o) const {
        o << "\"";
        const char* cp = s.c_str();
        const char* end = cp + s.size();

        while (cp < end) {
            switch (*cp) {
            case '\n':
                o << "\\n";
                break;
            case '\r':
                o << "\\r";
                break;
            case '\0':
                o << "\\0";
                break;
            case '\t':
                o << "\\t";
                break;
            case '\\':
            case '\'':
            case '\"':
                o << "\\" << *cp;
                break;
            default:
                if (const unsigned char c = *cp; c < 32) {
                    const unsigned char hi = '0' + (c & 0x0fu);
                    const unsigned char lo = '0' + (static_cast<unsigned char>(c >> 4u) & 0x0fu);
                    o << "\\x" << hi << lo;
                }
                else {
                    o << c;
                }
            }
            cp++;
        }

        o << "\"";
    }


    // Translate a glob pattern into a regular expression.
    std::string glob2regex(const char* glob) {
        std::string regex("^");
        constexpr char lparent = '(';
        constexpr char rparent = ')';
        constexpr char lbracket = '[';
        constexpr char rbracket = ']';
        constexpr char lbrace = '{';
        constexpr char rbrace = '}';

        // Translate wildcards '*', '**' and '?'.
        auto wildcard = [&]() {
            unsigned int min(0);
            bool nomax(false);

            if (*glob == '*' && glob[1] == '*') {
                // Match descendant?
                regex += ".";
                for (glob += 2; *glob == '*'; glob++);
            }
            else {
                regex += "[^/]"; // Do not match descendant.
                // Optimize consecutive wildcards gathering min and max counts.
                for (;; glob++) {
                    if (*glob == '?') {
                        min++;
                    }
                    else if (*glob == '*') {
                        if (glob[1] == '*') {
                            break; // Stop on descendant matchmark.
                        }
                        nomax = true;
                    }
                    else {
                        break;
                    }
                }
            }

            // Generate repetition counts.
            if (!min) {
                regex += '*';
            }
            else if (min == 1) {
                if (nomax) {
                    regex += '+';
                }
            }
            else {
                regex += std::string("{") + std::to_string(min);
                if (nomax) {
                    regex += ',';
                }
                regex += rbrace;
            }
        };

        // Translate [] sets.
        auto set = [&]() {
            regex += *glob++;
            if (*glob == '^' || *glob == '!') {
                // Handle negation mark.
                regex += "^";
                glob++;
            }
            // Convert set content.
            while (auto c = *glob) {
                glob++;
                if (c == rbracket) {
                    break;
                }
                if (c != '-') {
                    if (c == '\\' && *glob) {
                        c = *glob++;
                    }
                    if (strchr("]\\-", c)) {
                        regex += "\\";
                    }
                }
                regex += c;
            }
            regex += rbracket;
        };

        // Translate {,} groups.
        std::function<void(bool)> terms;
        auto group = [&]() {
            regex += lparent;
            for (glob++; *glob;) {
                if (*glob == ',') {
                    regex += '|';
                    glob++;
                }
                else if (*glob == rbrace) {
                    glob++;
                    break;
                }
                else {
                    terms(true);
                }
            }
            regex += rparent;
        };

        // Translate a sequence of terms.
        terms = [&](const bool ingroup) {
            while (*glob) {
                if (ingroup && (*glob == ',' || *glob == rbrace)) {
                    break;
                }
                if (*glob == lbracket) {
                    set();
                }
                else if (*glob == lbrace) {
                    group();
                }
                else if (*glob == '?' || *glob == '*') {
                    wildcard();
                }
                else {
                    if (*glob == '\\' && glob[1]) {
                        glob++;
                    }
                    if (strchr("$^+*?.=!|\\()[]{}", *glob)) {
                        regex += '\\';
                    }
                    regex += *glob++;
                }
            }
        };

        while (*glob) {
            terms(false);
        }
        return regex + '$';
    }
} // namespace Helper

// *************************************************************************
