//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef STICKZONE_HPP
#define STICKZONE_HPP

#include "action.hpp"
#include "stick.hpp"

namespace G13 {
    /// Manages the bindings for a G13 stick
    class G13_StickZone final : public G13_Actionable<G13_Stick> {
    public:
        G13_StickZone(G13_Stick&, const std::string& name, const G13_ZoneBounds&,
                      const std::shared_ptr<G13_Action>& = nullptr);

        void dump(std::ostream&) const;
        void test(const G13_ZoneCoord& loc);
        void set_bounds(const G13_ZoneBounds& bounds);

        //Operator Overload
        bool operator==(const G13_StickZone& other) const {
            return _name == other._name;
        }

    protected:
        G13_ZoneBounds _bounds;
        bool _active;
    };
}

#endif
