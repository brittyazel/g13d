//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ACTION_PIPEOUT_HPP
#define ACTION_PIPEOUT_HPP

#include "Action.hpp"
#include "Device.hpp"

namespace G13 {
    /// Action to send a string to the output pipe
    class PipeOutAction final : public Action {
    public:
        PipeOutAction(Device& keypad, const std::string& out);
        ~PipeOutAction() override;

        void act(Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::string _out;
    };
}

#endif
