//
// Created by Britt Yazel on 03-16-2025.
//

#include <vector>
#include <regex>

#include "g13_action_keys.hpp"
#include "g13_log.hpp"
#include "g13_stick.hpp"
#include "g13_stickzone.hpp"

namespace G13 {
    G13_Stick::G13_Stick(G13_Device& keypad) : _keypad(keypad), m_bounds(0, 0, 255, 255),
                                               m_center_pos(127, 127), m_north_pos(127, 0) {
        m_stick_mode = STICK_KEYS;

        auto add_zone = [this, &keypad](const std::string& name, const double x1, const double y1, const double x2,
                                        const double y2) {
            m_zones.emplace_back(*this, "STICK_" + name, G13_ZoneBounds(x1, y1, x2, y2),
                                 std::static_pointer_cast<G13_Action>(
                                     std::make_shared<G13_Action_Keys>(keypad, "KEY_" + name)));
        };

        // The joystick is inverted, so UP is the bottom of the stick facing the user.
        // Zone boundary coordinates are based on a floating point value from 0.0 (top/left) to 1.0 (bottom/right).
        add_zone("UP", 0.0, 0.0, 1.0, 0.3);
        add_zone("DOWN", 0.0, 0.7, 1.0, 1.0);
        add_zone("LEFT", 0.0, 0.0, 0.3, 1.0);
        add_zone("RIGHT", 0.7, 0.0, 1.0, 1.0);
    }

    G13_StickZone* G13_Stick::zone(const std::string& name, const bool create) {
        for (auto& zone : m_zones) {
            if (zone.name() == name) {
                return &zone;
            }
        }
        if (create) {
            m_zones.emplace_back(*this, name, G13_ZoneBounds(0.0, 0.0, 0.0, 0.0));
            return &m_zones.back();
        }
        return nullptr;
    }

    std::vector<std::string> G13_Stick::FilteredZoneNames(const std::regex& pattern) const {
        std::vector<std::string> names;

        for (const auto& zone : m_zones) {
            if (std::regex_match(zone.name(), pattern)) {
                names.emplace_back(zone.name());
            }
        }
        return names;
    }

    void G13_Stick::set_mode(const stick_mode_t m) {
        if (m == m_stick_mode) {
            return;
        }
        if (m_stick_mode == STICK_CALIB_CENTER || m_stick_mode == STICK_CALIB_BOUNDS || m_stick_mode == STICK_CALIB_NORTH) {
            RecalcCalibrated();
        }
        m_stick_mode = m;
        switch (m_stick_mode) {
        case STICK_CALIB_BOUNDS:
            m_bounds.tl = G13_StickCoord(255, 255);
            m_bounds.br = G13_StickCoord(0, 0);
            break;
        case STICK_ABSOLUTE:
        case STICK_KEYS:
        case STICK_CALIB_CENTER:
        case STICK_CALIB_NORTH:
            break;
        }
    }

    void G13_Stick::RecalcCalibrated() {}

    void G13_Stick::RemoveZone(const G13_StickZone& zone) {
        const G13_StickZone& target(zone);
        std::erase(m_zones, target);
    }

    void G13_Stick::dump(std::ostream& out) const {
        for (auto& zone : m_zones) {
            zone.dump(out);
            out << std::endl;
        }
    }

    void G13_Stick::ParseJoystick(const unsigned char* buf) {
        m_current_pos.x = buf[1];
        m_current_pos.y = buf[2];

        // update targets if we're in calibration mode
        switch (m_stick_mode) {
        case STICK_CALIB_CENTER:
            m_center_pos = m_current_pos;
            return;

        case STICK_CALIB_NORTH:
            m_north_pos = m_current_pos;
            return;

        case STICK_CALIB_BOUNDS:
            m_bounds.expand(m_current_pos);
            return;

        case STICK_ABSOLUTE:
        case STICK_KEYS:
            break;
        }

        // determine our normalized position
        double dx; // = 0.5
        if (m_current_pos.x <= m_center_pos.x) {
            dx = m_current_pos.x - m_bounds.tl.x;
            dx /= (m_center_pos.x - m_bounds.tl.x) * 2;
        }
        else {
            dx = m_bounds.br.x - m_current_pos.x;
            dx /= (m_bounds.br.x - m_center_pos.x) * 2;
            dx = 1.0 - dx;
        }
        double dy; // = 0.5;
        if (m_current_pos.y <= m_center_pos.y) {
            dy = m_current_pos.y - m_bounds.tl.y;
            dy /= (m_center_pos.y - m_bounds.tl.y) * 2;
        }
        else {
            dy = m_bounds.br.y - m_current_pos.y;
            dy /= (m_bounds.br.y - m_center_pos.y) * 2;
            dy = 1.0 - dy;
        }

        G13_DBG("x=" << m_current_pos.x << " y=" << m_current_pos.y << " dx=" << dx << " dy=" << dy);
        const G13_ZoneCoord jpos(dx, dy);

        if (m_stick_mode == STICK_ABSOLUTE) {
            _keypad.SendEvent(EV_ABS, ABS_X, m_current_pos.x);
            _keypad.SendEvent(EV_ABS, ABS_Y, m_current_pos.y);
        }
        else if (m_stick_mode == STICK_KEYS) {
            for (auto& zone : m_zones) {
                zone.test(jpos);
            }
        }
        else {
            /*    send_event(g13->uinput_file, EV_REL, REL_X, stick_x/16 - 8);
             SendEvent(g13->uinput_file, EV_REL, REL_Y, stick_y/16 - 8);*/
        }
    }
}
