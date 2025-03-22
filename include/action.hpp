//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ACTION_HPP
#define ACTION_HPP

#include "device.hpp"

namespace G13 {
    class G13_Device;  // Forward declaration

    /// Holds potential actions which can be bound to G13 activity
    class G13_Action {
    public:
        explicit G13_Action(G13_Device& keypad) : _keypad(keypad) {}
        virtual ~G13_Action() = default;

        virtual void act(G13_Device&, bool is_down) = 0;
        virtual void dump(std::ostream&) const = 0;

        void act(const bool is_down) {
            act(keypad(), is_down);
        }

        [[nodiscard]] G13_Device& keypad() const {
            return _keypad;
        }

    private:
        G13_Device& _keypad;
    };

}

#include "action.tpp"

#endif
