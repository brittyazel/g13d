//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ACTIONABLE_HPP
#define ACTIONABLE_HPP

#include <memory>
#include <string>

#include "action.hpp"

namespace G13 {
    class G13_Action;

    /// Template class to hold a reference to the parent object
    template <class PARENT_T>
    class G13_Actionable {
    public:
        G13_Actionable(PARENT_T& parent_arg, std::string name) : _name(std::move(name)), _parent_ptr(&parent_arg) {}

        virtual ~G13_Actionable() {
            _parent_ptr = nullptr;
        }

        [[nodiscard]] std::shared_ptr<G13_Action> action() const {
            return _action;
        }

        [[nodiscard]] const std::string& name() const {
            return _name;
        }

        virtual void set_action(const std::shared_ptr<G13_Action>& action) {
            _action = action;
        }

    protected:
        std::string _name;
        std::shared_ptr<G13_Action> _action;

    private:
        PARENT_T* _parent_ptr;
    };
}

#endif
