//
// Created by britt on 3/15/25.
//

#ifndef G13_KEY_HPP
#define G13_KEY_HPP

#include <map>

#include "g13_action.hpp"
#include "g13_actionable.hpp"
#include "g13_state_key.hpp"

namespace G13 {
    typedef int G13_KEY_INDEX;

    /// Manages the bindings for a G13 key
    class G13_Key final : public G13_Actionable<G13_Profile> {
    public:
        void dump(std::ostream& o) const;

        [[nodiscard]] G13_KEY_INDEX index() const;

        void ParseKey(const unsigned char* byte, G13_Device* g13) const;

    protected:
        struct KeyIndex {
            explicit KeyIndex(const int key) : index(key), offset(key / 8u), mask(1u << key % 8u) {}

            int index;
            unsigned char offset;
            unsigned char mask;
        };

        // G13_Profile is the only class able to instantiate G13_Key
        friend class G13_Profile;

        G13_Key(G13_Profile& mode, const std::string& name, const int index) : G13_Actionable(mode, name),
                                                                               _index(index), _should_parse(true) {}

        G13_Key(G13_Profile& mode, const G13_Key& key) : G13_Actionable(mode, key.name()), _index(key._index),
                                                         _should_parse(key._should_parse) {
            // TODO: do not invoke virtual member function from ctor
            G13_Actionable::set_action(key.action());
        }

        KeyIndex _index;
        bool _should_parse;
    };

    static std::map<G13_KEY_INDEX, std::string> g13_key_to_name;
    static std::map<std::string, G13_KEY_INDEX> g13_name_to_key;
    static std::map<LINUX_KEY_VALUE, std::string> input_key_to_name;
    static std::map<std::string, LINUX_KEY_VALUE> input_name_to_key;

    LINUX_KEY_VALUE InputKeyMax();
    int FindG13KeyValue(const std::string& keyname);
    std::string FindG13KeyName(int v);
    G13_State_Key FindInputKeyValue(const std::string& keyname, bool down = true);
    std::string FindInputKeyName(LINUX_KEY_VALUE v);
    void DisplayKeys();
    void InitKeynames();

} // namespace G13

#endif //G13_KEY_HPP
