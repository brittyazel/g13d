//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ACTION_KEYS_HPP
#define ACTION_KEYS_HPP

#include <vector>

#include "Objects/Action.hpp"
#include "Device.hpp"
#include "KeyState.hpp"


namespace G13 {
    /// Action to send one or more keystrokes
    class KeyAction final : public Action {
    public:
        KeyAction(Device& keypad, const std::string& keys_string);
        ~KeyAction() override;

        void act(Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::vector<KeyState> _keys;
        std::vector<KeyState> _keys_up;
    };
}

#endif
