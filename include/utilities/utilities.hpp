//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include "escaped_string_formatter.hpp"

namespace G13 {

    EscapedStringFormatter formatter(const std::string& new_string);

    struct split_t {
        enum empties_t { empties_ok, no_empties };
    };

    const char* left_trim(const char* string);
    std::string extract_and_advance_token(const char*& source);
    std::string glob_to_regex(const char* glob);

}

#include "utilities.tpp"

#endif