//
// Created by khampf on 07-05-2020.
//

#ifndef STICK_HPP
#define STICK_HPP

#include <vector>
#include <regex>

#include "Utils/utilities.hpp"

namespace G13 {

    class StickZone; // Forward declaration

    typedef Coord<int> StickCoord;
    typedef Bounds<int> StickBounds;
    typedef Coord<double> ZoneCoord;
    typedef Bounds<double> ZoneBounds;

    // *************************************************************************

    enum stick_mode_t {
        STICK_ABSOLUTE,
        STICK_KEYS,
        STICK_CALIB_CENTER,
        STICK_CALIB_BOUNDS,
        STICK_CALIB_NORTH
    };

    class Stick {
    public:
        explicit Stick(Device& keypad);

        void ParseJoystick(const unsigned char* buf);

        void set_mode(stick_mode_t);
        StickZone* zone(const std::string&, bool create = false);
        [[nodiscard]] std::vector<std::string> FilteredZoneNames(const std::regex& pattern) const;
        void RemoveZone(const StickZone& zone);

        void dump(std::ostream&) const;

    protected:
        static void RecalcCalibrated();

        Device& _keypad;
        std::vector<StickZone> m_zones;

        StickBounds m_bounds;
        StickCoord m_center_pos;
        StickCoord m_north_pos;

        StickCoord m_current_pos;

        stick_mode_t m_stick_mode;
    };
}

#endif
