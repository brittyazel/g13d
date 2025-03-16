//
// Created by britt on 3/15/25.
//

#include "g13_action_pipeout.hpp"
#include "g13_key.hpp"

namespace G13 {
    G13_Action_PipeOut::G13_Action_PipeOut(G13_Device& keypad, const std::string& out) : G13_Action(keypad),
        _out(out + "\n") {}

    G13_Action_PipeOut::~G13_Action_PipeOut() = default;

    void G13_Action_PipeOut::act(G13_Device& kp, const bool is_down) {
        if (is_down) {
            kp.OutputPipeWrite(_out);
        }
    }

    void G13_Action_PipeOut::dump(std::ostream& o) const {
        o << "WRITE PIPE : " << repr(_out);
    }
} // namespace G13
