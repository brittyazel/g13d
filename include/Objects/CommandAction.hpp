//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef COMMAND_ACTION_HPP
#define COMMAND_ACTION_HPP

#include "Action.hpp"

namespace G13 {
    /// Action to send a command to the g13
    class CommandAction final : public Action {
    public:
        CommandAction(Device& keypad, std::string cmd);
        ~CommandAction() override;

        void act(Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::string _cmd;
    };
}

#endif
