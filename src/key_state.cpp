//
// Created by Britt Yazel on 03-16-2025.
//

#include "key_state.hpp"

namespace G13 {
    G13_Key_State::G13_Key_State(const LINUX_KEY_VALUE key, const bool down) {
        _key = key;
        _down = down;
    }

    LINUX_KEY_VALUE G13_Key_State::key() const {
        return _key;
    }

    bool G13_Key_State::is_down() const {
        return _key;
    }
}
