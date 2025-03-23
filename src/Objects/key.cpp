//
// Created by Britt Yazel on 03-16-2025.
//

#include <cstring>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <ranges>

#include "Objects/key.hpp"
#include "Assets/key_tables.hpp"
#include "log.hpp"
#include "main.hpp"

namespace G13 {
    //definitions
    LINUX_KEY_VALUE input_key_max;

    //Constructors
    G13_Key::G13_Key(G13_Profile& mode, const std::string& name, const int index) : G13_Actionable(mode, name),
        _index(index), _should_parse(true) {}

    G13_Key::G13_Key(G13_Profile& mode, const G13_Key& key) : G13_Actionable(mode, key.name()), _index(key._index),
                                                              _should_parse(key._should_parse) {
        // TODO: do not invoke virtual member function from ctor
        G13_Actionable::set_action(key.action());
    }

    void G13_Key::dump(std::ostream& o) const {
        o << FindG13KeyName(index()) << "(" << index() << ") : ";
        if (action()) {
            action()->dump(o);
        }
        else {
            o << "(no action)";
        }
    }

    G13_KEY_INDEX G13_Key::index() const {
        return _index.index;
    }

    void G13_Key::ParseKey(const unsigned char* byte, G13_Device* g13) const {
        // state = true if key is pressed
        if (const bool state = byte[_index.offset] & _index.mask;
            g13->updateKeyState(_index.index, state) && _action) {
            // If the key state has changed and if we have an action, execute the action
            _action->act(*g13, state);
        }
    }

    /*************************************************/

    LINUX_KEY_VALUE InputKeyMax() {
        return input_key_max;
    }

    LINUX_KEY_VALUE FindG13KeyValue(const std::string& keyname) {
        const auto i = g13_name_to_key.find(keyname);
        if (i == g13_name_to_key.end()) {
            return BAD_KEY_VALUE;
        }
        return i->second;
    }

    G13_Key_State FindInputKeyValue(const std::string& keyname, bool down) {
        std::string modified_keyname = keyname;

        // If this is a release action, reverse sense
        if (!strncmp(keyname.c_str(), "-", 1)) {
            modified_keyname = keyname.c_str() + 1;
            down = !down;
        }

        // if there is a KEY_ prefix, strip it off
        if (!strncmp(modified_keyname.c_str(), "KEY_", 4)) {
            modified_keyname = modified_keyname.c_str() + 4;
        }

        const auto i = input_name_to_key.find(modified_keyname);
        if (i == input_name_to_key.end()) {
            return G13_Key_State(BAD_KEY_VALUE);
        }
        return G13_Key_State(i->second, down);
    }

    std::string FindInputKeyName(const LINUX_KEY_VALUE v) {
        try {
            return find_or_throw(input_key_to_name, v);
        }
        catch (...) {
            return "(unknown linux key)";
        }
    }

    std::string FindG13KeyName(const G13_KEY_INDEX v) {
        try {
            return find_or_throw(g13_key_to_name, v);
        }
        catch (...) {
            return "(unknown G13 key)";
        }
    }

    void DisplayKeys() {
        /* Display the know keys for the G13 */
        std::ostringstream output; // Create a stream to hold the output

        // First line should be on its own
        output << "Known keys on the Logitech G13:" << "\n";
        output << map_keys_out(g13_name_to_key);
        // Print the final output
        G13_OUT(output.str() << "\n");

        /***********************************/

        /* Display the know keys that we can map to */
        // There are a lot of keys, so we need to break them up into lines
        constexpr int max_line_length = 250;
        unsigned int current_length = 0;
        output.str(""); // Clear the stream

        // First line should be on its own
        output << "Known keys to map to:" << "\n";

        // Loop through the keys, add each to the output stream, insert a newline if we exceed the max line length
        for (const auto& key : input_name_to_key | std::views::keys) {
            if (current_length + key.length() + 1 > max_line_length) { // +1 for the space
                output << "\n";
                current_length = 0;
            }
            output << key << " ";
            current_length += key.length() + 1; // +1 for the space
        }

        // Print the final output
        G13_OUT(output.str() << "\n");
    }

    void InitKeynames() {
        int key_index = 0;

        // setup maps to let us convert between strings and G13 key names
        for (auto name = G13_KEY_STRINGS; *name; name++) {
            g13_key_to_name[key_index] = *name;
            g13_name_to_key[*name] = key_index;
            G13_DBG("mapping G13 " << *name << " = " << key_index);
            key_index++;
        }

        // setup maps to let us convert between strings and linux key names
        input_key_max = libevdev_event_type_get_max(EV_KEY) + 1;
        for (auto code = 0; code < input_key_max; code++) {
            if (const auto keystroke = libevdev_event_code_get_name(EV_KEY, code); keystroke && !strncmp(
                keystroke, "KEY_", 4)) {
                input_key_to_name[code] = keystroke + 4;
                input_name_to_key[keystroke + 4] = code;
                G13_DBG("mapping " << keystroke + 4 << " " << keystroke << "=" << code);
            }
        }

        // setup maps to let us convert between strings and linux button names
        for (auto symbol = G13_BTN_SEQ; *symbol; symbol++) {
            auto name = std::string("M" + std::string(*symbol));
            auto keyname = std::string("BTN_" + std::string(*symbol));
            if (int code = libevdev_event_code_from_name(EV_KEY, keyname.c_str()); code < 0) {
                G13_ERR("No input event code found for " << keyname);
            }
            else {
                input_key_to_name[code] = name;
                input_name_to_key[name] = code;
                G13_DBG("mapping " << name << " " << keyname << "=" << code);
            }
        }
    }
}
