//
// Created by Britt Yazel on 03-16-2025.
//

#include <iomanip>

#include "Objects/action.hpp"
#include "Objects/stickzone.hpp"

namespace G13 {
    G13_StickZone::G13_StickZone(G13_Stick& stick, const std::string& name, const G13_ZoneBounds& b,
                                 const std::shared_ptr<G13_Action>& action) :
        G13_Actionable(stick, name), _bounds(b), _active(false) {
        G13_Actionable::set_action(action); // Call to virtual from ctor!
    }

    void G13_StickZone::dump(std::ostream& out) const {
        out << "   " << std::setw(20) << name() << "   " << _bounds << "  ";
        if (action()) {
            action()->dump(out);
        }
        else {
            out << " (no action)";
        }
    }

    void G13_StickZone::test(const G13_ZoneCoord& loc) {
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

    void G13_StickZone::set_bounds(const G13_ZoneBounds& bounds) {
        _bounds = bounds;
    }
}
