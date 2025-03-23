//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <exception>

namespace G13 {
    class CommandException final : public std::exception {
    public:
        explicit CommandException(std::string reason) : reason(std::move(reason)) {}

        [[nodiscard]] const char* what() const noexcept override {
            return reason.c_str();
        };

    private:
        std::string reason;
    };

    class NotFoundException final : public std::exception {
    public:
        [[nodiscard]] const char* what() const noexcept override {
            return "Element not found";
        }
    };
}

#endif
