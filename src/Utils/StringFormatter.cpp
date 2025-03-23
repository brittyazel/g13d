//
// Created by Britt Yazel on 03-16-2025.
//

#include <cstring>
#include <iomanip>
#include <functional>

#include "Utils/StringFormatter.hpp"

namespace G13 {
    StringFormatter::StringFormatter(std::string str) : str(std::move(str)) {}

    void StringFormatter::write_on(std::ostream& output_stream) const {
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
}
