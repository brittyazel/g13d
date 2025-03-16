//
// Created by britt on 3/15/25.
//

#ifndef G13_STICKZONE_HPP
#define G13_STICKZONE_HPP

#include "g13_actionable.hpp"
#include "g13_stick.hpp"

namespace G13 {
    /// Manages the bindings for a G13 stick
    class G13_StickZone final : public G13_Actionable<G13_Stick> {
    public:
        G13_StickZone(G13_Stick&, const std::string& name, const G13_ZoneBounds&,
                      const std::shared_ptr<G13_Action>& = nullptr);

        bool operator==(const G13_StickZone& other) const {
            return _name == other._name;
        }

        void dump(std::ostream&) const;

        void test(const G13_ZoneCoord& loc);

        void set_bounds(const G13_ZoneBounds& bounds) {
            _bounds = bounds;
        }

    protected:
        G13_ZoneBounds _bounds;
        bool _active;
    };
} // namespace G13

#endif //G13_STICKZONE_HPP
