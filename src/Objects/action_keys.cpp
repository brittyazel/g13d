//
// Created by Britt Yazel on 03-16-2025.
//

#include <vector>

#include "Objects/action.hpp"
#include "Objects/action_keys.hpp"
#include "Objects/device.hpp"
#include "Objects/key.hpp"
#include "log.hpp"
#include "exceptions.hpp"

namespace G13 {
    G13_Action_Keys::G13_Action_Keys(G13_Device& keypad, const std::string& keys_string) : G13_Action(keypad) {
        auto scan = [](const std::string& in, std::vector<G13_Key_State>& out) {
            for (auto keys = split<std::vector<std::string>>(in, "+"); auto& key : keys) {
                auto keyVal = FindInputKeyValue(key);
                if (keyVal.key() == BAD_KEY_VALUE) {
                    throw G13_CommandException("create action unknown key : " + key);
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

    G13_Action_Keys::~G13_Action_Keys() = default;

    void G13_Action_Keys::act(G13_Device& g13, const bool is_down) {
        auto downkeys = std::vector(InputKeyMax(), false);

        auto send_key = [&](const LINUX_KEY_VALUE key, const bool down) {
            g13.SendEvent(EV_KEY, key, down);
            downkeys[key] = down;
            G13_LOG(log4cpp::Priority::DEBUG << "sending KEY " << (down? "DOWN ": "UP ") << key);
        };

        auto send_keys = [&](const std::vector<G13_Key_State>& keys) {
            for (auto& key : keys) {
                if (key.is_down() && downkeys[key.key()]) {
                    send_key(key.key(), false);
                }
                send_key(key.key(), key.is_down());
            }
        };

        auto release_keys = [&](const std::vector<G13_Key_State>& keys) {
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

    void G13_Action_Keys::dump(std::ostream& out) const {
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
