//
// Created by britt on 3/15/25.
//

#include "g13_state_key.hpp"

namespace G13 {
    G13_State_Key::G13_State_Key(const LINUX_KEY_VALUE key, const bool down) {
        _key = key;
        _down = down;
    }

    LINUX_KEY_VALUE G13_State_Key::key() const {
        return _key;
    }

    bool G13_State_Key::is_down() const {
        return _key;
    }
} // namespace G13
