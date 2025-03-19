//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ACTION_KEYS_HPP
#define ACTION_KEYS_HPP

#include <vector>

#include "action.hpp"
#include "device.hpp"
#include "key_state.hpp"


namespace G13 {
    /// Action to send one or more keystrokes
    class G13_Action_Keys final : public G13_Action {
    public:
        G13_Action_Keys(G13_Device& keypad, const std::string& keys_string);
        ~G13_Action_Keys() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::vector<G13_Key_State> _keys;
        std::vector<G13_Key_State> _keys_up;
    };
}

#endif
