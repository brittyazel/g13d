//
// Created by Britt Yazel on 03-16-2025.
//

#include "Objects/KeyState.hpp"

namespace G13 {
    KeyState::KeyState(const LINUX_KEY_VALUE key, const bool down) {
        _key = key;
        _down = down;
    }

    LINUX_KEY_VALUE KeyState::key() const {
        return _key;
    }

    bool KeyState::is_down() const {
        return _key;
    }
}
