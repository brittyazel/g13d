//
// Created by britt on 3/15/25.
//

#include <iomanip>

#include "g13_action.hpp"
#include "g13_stickzone.hpp"

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
                // cout << "exit stick zone " << m_name << std::endl;
                _action->act(false);
            }
        }
        else {
            // cout << "in stick zone " << m_name << std::endl;
            _action->act(true);
        }
    }

    void G13_StickZone::set_bounds(const G13_ZoneBounds& bounds) {
        _bounds = bounds;
    }
} // namespace G13
