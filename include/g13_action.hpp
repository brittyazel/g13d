//
// Created by khampf on 07-05-2020.
//

#ifndef G13_ACTION_HPP
#define G13_ACTION_HPP

#include <memory>
#include <vector>

#include "g13_actionable.hpp"
#include "g13_keys.hpp"
#include "g13_stick.hpp"

namespace G13 {
    class G13_Device;
    class G13_Profile;


    /// Holds potential actions which can be bound to G13 activity
    class G13_Action {
    public:
        explicit G13_Action(G13_Device& keypad) : _keypad(keypad) {}
        virtual ~G13_Action();

        virtual void act(G13_Device&, bool is_down) = 0;
        virtual void dump(std::ostream&) const = 0;

        void act(const bool is_down) {
            act(keypad(), is_down);
        }

        [[nodiscard]] G13_Device& keypad() const {
            return _keypad;
        }

    private:
        G13_Device& _keypad;
    };


    /// Action to send one or more keystrokes
    class G13_Action_Keys final : public G13_Action {
    public:
        G13_Action_Keys(G13_Device& keypad, const std::string& keys_string);
        ~G13_Action_Keys() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::vector<G13_State_Key> _keys;
        std::vector<G13_State_Key> _keysup;
    };


    /// Action to send a string to the output pipe
    class G13_Action_PipeOut final : public G13_Action {
    public:
        G13_Action_PipeOut(G13_Device& keypad, const std::string& out);
        ~G13_Action_PipeOut() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::string _out;
    };

    /// Action to send a command to the g13
    class G13_Action_Command final : public G13_Action {
    public:
        G13_Action_Command(G13_Device& keypad, std::string cmd);
        ~G13_Action_Command() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::string _cmd;
    };


    /// Manages the bindings for a G13 key
    class G13_Key final : public G13_Actionable<G13_Profile> {
    public:
        void dump(std::ostream& o) const;

        [[nodiscard]] G13_KEY_INDEX index() const {
            return _index.index;
        }

        void ParseKey(const unsigned char* byte, G13_Device* g13) const;

    protected:
        struct KeyIndex {
            explicit KeyIndex(const int key) : index(key), offset(key / 8u), mask(1u << (key % 8u)) {}

            int index;
            unsigned char offset;
            unsigned char mask;
        };

        // G13_Profile is the only class able to instantiate G13_Keys
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


    /// Manages the bindings for a G13 stick
    class G13_StickZone final : public G13_Actionable<G13_Stick> {
    public:
        G13_StickZone(G13_Stick&, const std::string& name, const G13_ZoneBounds&, const std::shared_ptr<G13_Action>& = nullptr);

        bool operator==(const G13_StickZone& other) const {
            return _name == other._name;
        }

        void dump(std::ostream&) const;

        void test(const G13_ZoneCoord& loc);

        void set_bounds(const G13_ZoneBounds& bounds) {
            _bounds = bounds;
        }

    protected:
        G13_ZoneBounds _bounds;
        bool _active;
    };

} // namespace G13
#endif // G13_ACTION_HPP
