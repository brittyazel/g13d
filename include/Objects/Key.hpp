//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef KEY_HPP
#define KEY_HPP

#include <map>
#include <string>

#include "Action.hpp"
#include "KeyState.hpp"

namespace G13 {
    typedef int KEY_INDEX;

    /// Manages the bindings for a G13 key
    class Key final : public Actionable<Profile> {
    public:
        void dump(std::ostream& o) const;
        [[nodiscard]] KEY_INDEX index() const;
        void ParseKey(const unsigned char* byte, Device* g13) const;

    protected:
        struct KeyIndex {
            int index;
            unsigned char offset;
            unsigned char mask;

            explicit KeyIndex(const int key) : index(key), offset(key / 8u), mask(1u << key % 8u) {}
        };

        // Profile is the only class able to instantiate Key
        friend class Profile;

        Key(Profile& mode, const std::string& name, int index);
        Key(Profile& mode, const Key& key);

        KeyIndex _index;
        bool _should_parse;
    };

    static std::map<KEY_INDEX, std::string> key_to_name;
    static std::map<std::string, KEY_INDEX> name_to_key;
    static std::map<LINUX_KEY_VALUE, std::string> input_key_to_name;
    static std::map<std::string, LINUX_KEY_VALUE> input_name_to_key;

    LINUX_KEY_VALUE InputKeyMax();
    int FindG13KeyValue(const std::string& keyname);
    std::string FindG13KeyName(int v);
    KeyState FindInputKeyValue(const std::string& keyname, bool down = true);
    std::string FindInputKeyName(LINUX_KEY_VALUE v);
    void DisplayKeys();
    void InitKeynames();
}

#endif
