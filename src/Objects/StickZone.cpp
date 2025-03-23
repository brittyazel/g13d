//
// Created by Britt Yazel on 03-16-2025.
//

#include <iomanip>

#include "Objects/Action.hpp"
#include "Objects/StickZone.hpp"

namespace G13 {
    StickZone::StickZone(Stick& stick, const std::string& name, const ZoneBounds& b,
                                 const std::shared_ptr<Action>& action) :
        Actionable(stick, name), _bounds(b), _active(false) {
        Actionable::set_action(action); // Call to virtual from ctor!
    }

    void StickZone::dump(std::ostream& out) const {
        out << "   " << std::setw(20) << name() << "   " << _bounds << "  ";
        if (action()) {
            action()->dump(out);
        }
        else {
            out << " (no action)";
        }
    }

    void StickZone::test(const ZoneCoord& loc) {
        if (!_action)
            return;
        const bool prior_active = _active;
        _active = _bounds.contains(loc);
        if (!_active) {
            if (prior_active) {
                _action->act(false);
            }
        }
        else {
            _action->act(true);
        }
    }

    void StickZone::set_bounds(const ZoneBounds& bounds) {
        _bounds = bounds;
    }
}
