//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ACTION_COMMAND_HPP
#define ACTION_COMMAND_HPP

#include "action.hpp"

namespace G13 {
    /// Action to send a command to the g13
    class G13_Action_Command final : public G13_Action {
    public:
        G13_Action_Command(G13_Device& keypad, std::string cmd);
        ~G13_Action_Command() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::string _cmd;
    };
}

#endif
