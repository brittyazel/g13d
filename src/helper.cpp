#include <cstring>
#include <functional>

#include "helper.hpp"

namespace G13 {
    string_repr_out::string_repr_out(std::string str) : s(std::move(str)) {}

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
                    const unsigned char hi = '0' + (c & 0x0F);
                    const unsigned char lo = '0' + (static_cast<unsigned char>(c >> 4) & 0x0F);
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

    std::ostream& operator<<(std::ostream& o, const string_repr_out& sro) {
        sro.write_on(o);
        return o;
    }

    string_repr_out repr(const std::string& s) {
        return string_repr_out(s);
    }

    const char* NotFoundException::what() const noexcept {
        return "Element not found";
    }

    const char* ltrim(const char* string, const char* ws) {
        return string + strspn(string, ws);
    }

    const char* advance_ws(const char*& source, std::string& dest) {
        source = ltrim(source);
        const size_t l = strcspn(source, "# \t");
        dest = std::string(source, l);
        source = !source[l] || source[l] == '#' ? "" : source + l + 1;
        return source;
    }

    void IGUR(...) {}

    std::string glob2regex(const char* glob) {
        std::string regex("^");
        constexpr char lparent = '(';
        constexpr char rparent = ')';
        constexpr char lbracket = '[';
        constexpr char rbracket = ']';
        constexpr char lbrace = '{';
        constexpr char rbrace = '}';

        auto wildcard = [&] {
            unsigned int min(0);
            bool nomax(false);

            if (*glob == '*' && glob[1] == '*') {
                regex += ".";
                for (glob += 2; *glob == '*'; glob++);
            }
            else {
                regex += "[^/]";
                for (;; glob++) {
                    if (*glob == '?') {
                        min++;
                    }
                    else if (*glob == '*') {
                        if (glob[1] == '*') {
                            break;
                        }
                        nomax = true;
                    }
                    else {
                        break;
                    }
                }
            }

            if (!min) {
                regex += '*';
            }
            else if (min == 1) {
                if (nomax) {
                    regex += '+';
                }
            }
            else {
                regex += "{" + std::to_string(min);
                if (nomax) {
                    regex += ',';
                }
                regex += '}';
            }
        };

        auto set = [&] {
            regex += *glob++;
            if (*glob == '^' || *glob == '!') {
                regex += "^";
                glob++;
            }
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

        std::function<void(bool)> terms;
        auto group = [&] {
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
} // namespace G13
