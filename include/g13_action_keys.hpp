//
// Created by britt on 3/15/25.
//

#ifndef G13_ACTION_KEYS_HPP
#define G13_ACTION_KEYS_HPP

#include <vector>

#include "g13_action.hpp"
#include "g13_device.hpp"
#include "g13_state_key.hpp"

namespace G13 {
    /// Action to send one or more keystrokes
    class G13_Action_Keys final : public G13_Action {
    public:
        G13_Action_Keys(G13_Device& keypad, const std::string& keys_string);
        ~G13_Action_Keys() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::vector<G13_State_Key> _keys;
        std::vector<G13_State_Key> _keysup;
    };
} // namespace G13

#endif //G13_ACTION_KEYS_HPP
