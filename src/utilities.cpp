//
// Created by Britt Yazel on 03-16-2025.
//

#include <cstring>
#include <iomanip>
#include <functional>
#include <sstream>

#include "utilities.hpp"

namespace G13 {
    string_repr_out::string_repr_out(std::string str) : str(std::move(str)) {}

    void string_repr_out::write_on(std::ostream& output_stream) const {
        output_stream << "\"";
        for (const char character : str) {
            switch (character) {
            case '\n': output_stream << "\\n";
                break;
            case '\r': output_stream << "\\r";
                break;
            case '\0': output_stream << "\\0";
                break;
            case '\t': output_stream << "\\t";
                break;
            case '\\':
            case '\'':
            case '\"': output_stream << "\\" << character;
                break;
            default:
                if (character < 32 || character > 126) {
                    output_stream << "\\x" << std::hex << std::setw(2) << std::setfill('0') <<
                        static_cast<int>(static_cast<unsigned char>(character));
                }
                else {
                    output_stream << character;
                }
            }
        }
        output_stream << "\"";
    }

    std::ostream& operator<<(std::ostream& output_stream, const string_repr_out& string_repr) {
        string_repr.write_on(output_stream);
        return output_stream;
    }

    const char* NotFoundException::what() const noexcept {
        return "Element not found";
    }

    /**
    * @brief Trims leading spaces and tabs from a string.
    *
    * This function skips over any leading spaces and tabs in the input string
    * and returns a pointer to the first non-whitespace character.
    *
    * @param string The input string to be trimmed.
    * @return A pointer to the first non-whitespace character in the input string.
    */
    const char* left_trim(const char* string) {
        // Skip over leading spaces and tabs
        while (*string == ' ' || *string == '\t') {
            ++string;
        }
        // Return pointer to the first non-whitespace character
        return string;
    }

    /**
     * @brief Extracts the next token from the source string and advances the source pointer.
     *
     * This function trims leading whitespace from the source string, extracts the next token
     * (delimited by whitespace or a hash character), and advances the source pointer past the token.
     *
     * @param source A reference to the source string pointer. The pointer will be advanced past the extracted token.
     * @return The extracted token.
     */
    std::string extract_and_advance_token(const char*& source) {
        std::istringstream stream(source);
        std::string token;
        char delimiter;

        // Extract the next token delimited by whitespace or a hash character
        while (stream.get(delimiter)) {
            if (delimiter == ' ' || delimiter == '#') {
                break;
            }
            token += delimiter;
        }

        // Update the source pointer to the remaining string
        source += stream.tellg();
        return token;
    }

    /**
     * @brief Converts a glob pattern to a regular expression pattern.
     *
     * This function takes a glob pattern (commonly used for filename matching) and converts it into a
     * regular expression (regex) pattern. The resulting regex pattern can be used for advanced and
     * flexible string matching operations.
     *
     * The function handles the following glob pattern elements:
     * - `*` : Matches any sequence of characters (except for the directory separator `/`).
     * - `**` : Matches any sequence of characters, including directory separators.
     * - `?` : Matches any single character.
     * - `[...]` : Matches any one of the enclosed characters.
     * - `{...}` : Matches any of the comma-separated alternatives.
     *
     * @param glob The input glob pattern as a C-style string.
     * @return A string containing the equivalent regular expression pattern.
     */
    std::string glob_to_regex(const char* glob) {
        // Initialize the regex string with the start anchor
        std::string regex("^");

        // Define constants for various characters used in the function
        constexpr char left_parent = '(';
        constexpr char right_parent = ')';
        constexpr char left_bracket = '[';
        constexpr char right_bracket = ']';
        constexpr char left_brace = '{';
        constexpr char right_brace = '}';

        // Lambda function to handle wildcard characters (* and ?)
        auto wildcard = [&] {
            unsigned int min = 0;
            bool no_max = false;

            // Handle double asterisk (**) for matching any number of directories
            if (*glob == '*' && glob[1] == '*') {
                regex += ".*";
                glob += 2;
                while (*glob == '*') {
                    ++glob;
                }
            }
            else {
                // Handle single asterisk (*) and question mark (?)
                regex += "[^/]*";
                while (*glob == '?' || *glob == '*') {
                    if (*glob == '?') {
                        ++min;
                    }
                    else if (*glob == '*') {
                        no_max = true;
                    }
                    ++glob;
                }
            }

            // Add quantifiers to the regex based on the number of wildcards
            if (min > 1) {
                regex += "{" + std::to_string(min);
                if (no_max) {
                    regex += ',';
                }
                regex += '}';
            }
            else if (min == 1 && no_max) {
                regex += '+';
            }
        };

        // Lambda function to handle character sets ([...])
        auto set = [&] {
            regex += *glob++;
            if (*glob == '^' || *glob == '!') {
                regex += '^';
                ++glob;
            }
            while (*glob && *glob != right_bracket) {
                if (*glob == '-' && glob[1] != right_bracket) {
                    regex += '-';
                    ++glob;
                }
                else {
                    if (*glob == '\\' && glob[1]) {
                        regex += '\\';
                        ++glob;
                    }
                    regex += *glob++;
                }
            }
            if (*glob == right_bracket) {
                regex += right_bracket;
                ++glob;
            }
        };

        // Lambda function to handle groups ({...})
        std::function<void(bool)> terms;
        auto group = [&] {
            regex += left_parent;
            ++glob;
            while (*glob && *glob != right_brace) {
                if (*glob == ',') {
                    regex += '|';
                    ++glob;
                }
                else {
                    terms(true);
                }
            }
            if (*glob == right_brace) {
                ++glob;
            }
            regex += right_parent;
        };

        // Lambda function to handle terms within the glob pattern
        terms = [&](const bool in_group) {
            while (*glob && (!in_group || (*glob != ',' && *glob != right_brace))) {
                if (*glob == left_bracket) {
                    set();
                }
                else if (*glob == left_brace) {
                    group();
                }
                else if (*glob == '?' || *glob == '*') {
                    wildcard();
                }
                else {
                    if (*glob == '\\' && glob[1]) {
                        regex += '\\';
                        ++glob;
                    }
                    if (strchr("$^+*?.=!|\\()[]{}", *glob)) {
                        regex += '\\';
                    }
                    regex += *glob++;
                }
            }
        };

        // Process the entire glob pattern
        while (*glob) {
            terms(false);
        }
        // Add the end anchor to the regex string
        return regex + '$';
    }
}
