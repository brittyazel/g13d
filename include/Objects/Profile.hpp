//
// Created by khampf on 13-05-2020.
//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef PROFILE_HPP
#define PROFILE_HPP

#include <regex>

namespace G13 {
    class Key;
    /*!
     * Represents a set of configured key mappings
     *
     * This allows a keypad to have multiple configured
     * profiles and switch between them easily
     */
    class Profile {
    public:
        Profile(Device& keypad, std::string name_arg);
        Profile(const Profile& other, std::string name_arg);

        // search key by G13 keyname
        Key* FindKey(const std::string& keyname);
        [[nodiscard]] std::vector<std::string> FilteredKeyNames(const std::regex& pattern, bool all = false) const;
        void dump(std::ostream& o) const;
        void ParseKeys(const unsigned char* buf);
        [[nodiscard]] const std::string& name() const;

    protected:
        Device& _keypad;
        std::vector<Key> _keys;
        std::string _name;

        void _init_keys();
    };
}

#endif
