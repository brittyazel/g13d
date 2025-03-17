//
// Created by britt on 3/15/25.
//

#include "g13_action_command.hpp"
#include "g13_key.hpp"

namespace G13 {
    G13_Action_Command::G13_Action_Command(G13_Device& keypad, std::string cmd) : G13_Action(keypad),
        _cmd(std::move(cmd)) {}

    G13_Action_Command::~G13_Action_Command() = default;

    void G13_Action_Command::act(G13_Device& kp, const bool is_down) {
        if (is_down) {
            keypad().Command(_cmd.c_str());
        }
    }

    void G13_Action_Command::dump(std::ostream& o) const {
        o << "COMMAND : " << repr(_cmd);
    }
}
