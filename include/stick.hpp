//
// Created by khampf on 07-05-2020.
//

#ifndef STICK_HPP
#define STICK_HPP

#include <vector>
#include <regex>

#include "utilities.hpp"

namespace G13 {
    class G13_Device;

    typedef Coord<int> G13_StickCoord;
    typedef Bounds<int> G13_StickBounds;
    typedef Coord<double> G13_ZoneCoord;
    typedef Bounds<double> G13_ZoneBounds;

    // *************************************************************************

    class G13_StickZone;

    enum stick_mode_t {
        STICK_ABSOLUTE,
        STICK_KEYS,
        STICK_CALIB_CENTER,
        STICK_CALIB_BOUNDS,
        STICK_CALIB_NORTH
    };

    class G13_Stick {
    public:
        explicit G13_Stick(G13_Device& keypad);

        void ParseJoystick(const unsigned char* buf);

        void set_mode(stick_mode_t);
        G13_StickZone* zone(const std::string&, bool create = false);
        [[nodiscard]] std::vector<std::string> FilteredZoneNames(const std::regex& pattern) const;
        void RemoveZone(const G13_StickZone& zone);

        void dump(std::ostream&) const;

    protected:
        static void RecalcCalibrated();

        G13_Device& _keypad;
        std::vector<G13_StickZone> m_zones;

        G13_StickBounds m_bounds;
        G13_StickCoord m_center_pos;
        G13_StickCoord m_north_pos;

        G13_StickCoord m_current_pos;

        stick_mode_t m_stick_mode;
    };
}

#endif
