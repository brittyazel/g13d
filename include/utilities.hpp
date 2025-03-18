//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <exception>
#include <string>

namespace G13 {
    class string_repr_out {
    public:
        explicit string_repr_out(std::string str);
        void write_on(std::ostream& output_stream) const;
        std::string str;
    };

    inline string_repr_out repr(const std::string& new_string) {
        return string_repr_out(new_string);
    }

    std::ostream& operator<<(std::ostream& output_stream, const string_repr_out& string_repr);

    class NotFoundException final : public std::exception {
    public:
        [[nodiscard]] const char* what() const noexcept override;
    };

    struct split_t {
        enum empties_t { empties_ok, no_empties };
    };

    inline void IGUR(...) {}

    const char* left_trim(const char* string);
    std::string extract_and_advance_token(const char*& source);
    std::string glob_to_regex(const char* glob);

}

#include "utilities.tpp"

#endif