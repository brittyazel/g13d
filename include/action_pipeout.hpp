//
// Created by britt on 3/15/25.
//

#ifndef ACTION_PIPEOUT_HPP
#define ACTION_PIPEOUT_HPP

#include "action.hpp"
#include "device.hpp"

namespace G13 {
    /// Action to send a string to the output pipe
    class G13_Action_PipeOut final : public G13_Action {
    public:
        G13_Action_PipeOut(G13_Device& keypad, const std::string& out);
        ~G13_Action_PipeOut() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::string _out;
    };
}

#endif
