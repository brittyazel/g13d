//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef ACTION_TPP
#define ACTION_TPP

namespace G13 {
    /// Template class to hold a reference to the parent object
    template <class PARENT_T>
    class Actionable {
    public:
        Actionable(PARENT_T& parent_arg, std::string name) : _name(std::move(name)), _parent_ptr(&parent_arg) {}

        virtual ~Actionable() {
            _parent_ptr = nullptr;
        }

        [[nodiscard]] std::shared_ptr<Action> action() const {
            return _action;
        }

        [[nodiscard]] const std::string& name() const {
            return _name;
        }

        virtual void set_action(const std::shared_ptr<Action>& action) {
            _action = action;
        }

    protected:
        std::string _name{};
        std::shared_ptr<Action> _action{};

    private:
        PARENT_T* _parent_ptr;
    };
}

#endif
