//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef STRING_FORMATTER_HPP
#define STRING_FORMATTER_HPP

namespace G13 {
    class StringFormatter {
    public:
        explicit StringFormatter(std::string str);
        void write_on(std::ostream& output_stream) const;

        friend std::ostream& operator<<(std::ostream& output_stream, const StringFormatter& string_repr);

    private:
        std::string str;
    };

    inline std::ostream& operator<<(std::ostream& output_stream, const StringFormatter& string_repr) {
        string_repr.write_on(output_stream);
        return output_stream;
    }
}

#endif
