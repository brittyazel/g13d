//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ACTION_HPP
#define ACTION_HPP

#include "Device.hpp"

namespace G13 {
    class Device; // Forward declaration

    /// Holds potential actions which can be bound to G13 activity
    class Action {
    public:
        explicit Action(Device& keypad) : _keypad(keypad) {}
        virtual ~Action() = default;

        virtual void act(Device&, bool is_down) = 0;
        virtual void dump(std::ostream&) const = 0;

        void act(const bool is_down) {
            act(keypad(), is_down);
        }

        [[nodiscard]] Device& keypad() const {
            return _keypad;
        }

    private:
        Device& _keypad;
    };
}

#include "Action.tpp"

#endif
