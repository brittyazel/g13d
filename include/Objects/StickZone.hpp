//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef STICK_ZONE_HPP
#define STICK_ZONE_HPP

#include "Action.hpp"
#include "Stick.hpp"

namespace G13 {

    /// Manages the bindings for a G13 stick
    class StickZone final : public Actionable<Stick> {
    public:
        StickZone(Stick&, const std::string& name, const ZoneBounds&,
                      const std::shared_ptr<Action>& = nullptr);

        void dump(std::ostream&) const;
        void test(const ZoneCoord& loc);
        void set_bounds(const ZoneBounds& bounds);

        //Operator Overload
        bool operator==(const StickZone& other) const {
            return _name == other._name;
        }

    protected:
        ZoneBounds _bounds;
        bool _active;
    };
}

#endif
