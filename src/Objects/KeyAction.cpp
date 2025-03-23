//
// Created by Britt Yazel on 03-16-2025.
//

#include <vector>

#include "Objects/Action.hpp"
#include "Objects/KeyAction.hpp"
#include "Objects/Device.hpp"
#include "Objects/Key.hpp"
#include "log.hpp"
#include "exceptions.hpp"

namespace G13 {
    KeyAction::KeyAction(Device& keypad, const std::string& keys_string) : Action(keypad) {
        auto scan = [](const std::string& in, std::vector<KeyState>& out) {
            for (auto keys = split<std::vector<std::string>>(in, "+"); auto& key : keys) {
                auto keyVal = FindInputKeyValue(key);
                if (keyVal.key() == BAD_KEY_VALUE) {
                    throw CommandException("create action unknown key : " + key);
                }
                out.push_back(keyVal);
            }
        };

        const auto key_down_up = split<std::vector<std::string>>(keys_string, " ");

        scan(key_down_up[0], _keys);

        if (key_down_up.size() > 1) {
            scan(key_down_up[1], _keys_up);
        }
    }

    KeyAction::~KeyAction() = default;

    void KeyAction::act(Device& g13, const bool is_down) {
        auto downkeys = std::vector(InputKeyMax(), false);

        auto send_key = [&](const LINUX_KEY_VALUE key, const bool down) {
            g13.SendEvent(EV_KEY, key, down);
            downkeys[key] = down;
            LOG(log4cpp::Priority::DEBUG << "sending KEY " << (down? "DOWN ": "UP ") << key);
        };

        auto send_keys = [&](const std::vector<KeyState>& keys) {
            for (auto& key : keys) {
                if (key.is_down() && downkeys[key.key()]) {
                    send_key(key.key(), false);
                }
                send_key(key.key(), key.is_down());
            }
        };

        auto release_keys = [&](const std::vector<KeyState>& keys) {
            for (auto i = keys.size(); i--;) {
                if (downkeys[keys[i].key()]) {
                    send_key(keys[i].key(), false);
                }
            }
        };

        if (is_down) {
            send_keys(_keys);
            if (!_keys_up.empty())
                release_keys(_keys);
        }
        else if (_keys_up.empty()) {
            for (auto& key : _keys)
                downkeys[key.key()] = key.is_down();
            release_keys(_keys);
        }
        else {
            send_keys(_keys_up);
            release_keys(_keys_up);
        }
    }

    void KeyAction::dump(std::ostream& out) const {
        out << " SEND KEYS: ";

        for (size_t i = 0; i < _keys.size(); i++) {
            if (i) {
                out << " + ";
            }
            if (!_keys[i].is_down()) {
                out << "-";
            }
            out << FindInputKeyName(_keys[i].key());
        }
    }
}
