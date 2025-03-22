//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ESCAPED_STRING_FORMATTER_HPP
#define ESCAPED_STRING_FORMATTER_HPP

namespace G13 {
    class EscapedStringFormatter {
    public:
        explicit EscapedStringFormatter(std::string str);
        void write_on(std::ostream& output_stream) const;

        friend std::ostream& operator<<(std::ostream& output_stream, const EscapedStringFormatter& string_repr);

    private:
        std::string str;
    };

    inline std::ostream& operator<<(std::ostream& output_stream, const EscapedStringFormatter& string_repr) {
        string_repr.write_on(output_stream);
        return output_stream;
    }

}

#endif