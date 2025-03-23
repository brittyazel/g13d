//
// Created by khampf on 13-05-2020.
//

#include <cassert>

#include "Objects/Key.hpp"
#include "Assets/key_tables.hpp"
#include "main.hpp"
#include "Objects/Profile.hpp"

namespace G13 {
    Profile::Profile(Device& keypad, std::string name_arg) :
        _keypad(keypad), _name(std::move(name_arg)) {
        _init_keys();
    }

    Profile::Profile(const Profile& other, std::string name_arg) :
        _keypad(other._keypad), _keys(other._keys), _name(std::move(name_arg)) {}


    void Profile::_init_keys() {
        // create a Key entry for every key in KEY_STRINGS
        int key_index = 0;
        // std::string str = KEY_STRINGS[0];

        for (auto symbol = KEY_STRINGS; *symbol; symbol++) {
            _keys.emplace_back(Key(*this, *symbol, key_index));
            key_index++;
        }
        assert(_keys.size() == NUM_KEYS);

        // now disable testing for keys in NON_PARSED_KEYS
        for (auto symbol = NON_PARSED_KEYS; *symbol; symbol++) {
            Key* key = FindKey(*symbol);
            key->_should_parse = false;
        }
    }

    void Profile::dump(std::ostream& o) const {
        o << "Profile " << formatter(name()) << std::endl;
        for (auto& key : _keys) {
            if (key.action()) {
                o << "   ";
                key.dump(o);
                o << std::endl;
            }
        }
    }

    void Profile::ParseKeys(const unsigned char* buf) {
        buf += 3;
        for (auto& _key : _keys) {
            if (_key._should_parse) {
                _key.ParseKey(buf, &_keypad);
            }
        }
    }

    Key* Profile::FindKey(const std::string& keyname) {
        if (const auto key = FindG13KeyValue(keyname); static_cast<size_t>(key) < _keys.size()) {
            return &_keys[key];
        }
        return nullptr;
    }

    std::vector<std::string> Profile::FilteredKeyNames(const std::regex& pattern, const bool all) const {
        std::vector<std::string> names;

        for (auto& key : _keys)
            if (all || key.action())
                if (std::regex_match(key.name(), pattern))
                    names.emplace_back(key.name());
        return names;
    }

    const std::string& Profile::name() const {
        return _name;
    }
} // namespace G1pattern
