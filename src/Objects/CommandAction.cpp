//
// Created by Britt Yazel on 03-16-2025.
//

#include "Objects/CommandAction.hpp"
#include "Objects/Device.hpp"
#include "Objects/Key.hpp"

namespace G13 {
    CommandAction::CommandAction(Device& keypad, std::string cmd) : Action(keypad),
        _cmd(std::move(cmd)) {}

    CommandAction::~CommandAction() = default;

    void CommandAction::act(Device& kp, const bool is_down) {
        if (is_down) {
            keypad().Command(_cmd.c_str());
        }
    }

    void CommandAction::dump(std::ostream& o) const {
        o << "COMMAND : " << formatter(_cmd);
    }
}
