//
// Created by khampf on 13-05-2020.
//

#include <cassert>

#include "key.hpp"
#include "key_tables.hpp"
#include "main.hpp"
#include "profile.hpp"

namespace G13 {
    G13_Profile::G13_Profile(G13_Device& keypad, std::string name_arg) :
        _keypad(keypad), _name(std::move(name_arg)) {
        _init_keys();
    }

    G13_Profile::G13_Profile(const G13_Profile& other, std::string name_arg) :
        _keypad(other._keypad), _keys(other._keys), _name(std::move(name_arg)) {}


    void G13_Profile::_init_keys() {
        // create a G13_Key entry for every key in G13_KEY_STRINGS
        int key_index = 0;
        // std::string str = G13_Key_Tables::G13_KEY_STRINGS[0];

        for (auto symbol = G13_Key_Tables::G13_KEY_STRINGS; *symbol; symbol++) {
            _keys.emplace_back(G13_Key(*this, *symbol, key_index));
            key_index++;
        }
        assert(_keys.size() == G13_NUM_KEYS);

        // now disable testing for keys in G13_NON_PARSED_KEYS
        for (auto symbol = G13_Key_Tables::G13_NON_PARSED_KEYS; *symbol; symbol++) {
            G13_Key* key = FindKey(*symbol);
            key->_should_parse = false;
        }
    }

    void G13_Profile::dump(std::ostream& o) const {
        o << "Profile " << formatter(name()) << std::endl;
        for (auto& key : _keys) {
            if (key.action()) {
                o << "   ";
                key.dump(o);
                o << std::endl;
            }
        }
    }

    void G13_Profile::ParseKeys(const unsigned char* buf) {
        buf += 3;
        for (auto& _key : _keys) {
            if (_key._should_parse) {
                _key.ParseKey(buf, &_keypad);
            }
        }
    }

    G13_Key* G13_Profile::FindKey(const std::string& keyname) {
        if (const auto key = FindG13KeyValue(keyname); static_cast<size_t>(key) < _keys.size()) {
            return &_keys[key];
        }
        return nullptr;
    }

    std::vector<std::string> G13_Profile::FilteredKeyNames(const std::regex& pattern, const bool all) const {
        std::vector<std::string> names;

        for (auto& key : _keys)
            if (all || key.action())
                if (std::regex_match(key.name(), pattern))
                    names.emplace_back(key.name());
        return names;
    }

    const std::string& G13_Profile::name() const {
        return _name;
    }
} // namespace G1pattern
