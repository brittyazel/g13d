//
// Created by Britt Yazel on 03-16-2025.
//

#include "Objects/PipeOutAction.hpp"
#include "Objects/Key.hpp"


namespace G13 {
    PipeOutAction::PipeOutAction(Device& keypad, const std::string& out) : Action(keypad),
        _out(out + "\n") {}

    PipeOutAction::~PipeOutAction() = default;

    void PipeOutAction::act(Device& kp, const bool is_down) {
        if (is_down) {
            kp.OutputPipeWrite(_out);
        }
    }

    void PipeOutAction::dump(std::ostream& o) const {
        o << "WRITE PIPE : " << formatter(_out);
    }
}
