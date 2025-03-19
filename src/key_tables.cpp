//
// Created by Britt Yazel on 03-16-2025.
//

#include "key_tables.hpp"

namespace G13 {
    const char* G13_Key_Tables::G13_KEY_STRINGS[] = {
            /* byte 3 */
            "G1", "G2", "G3", "G4", "G5", "G6", "G7", "G8",
            /* byte 4 */
            "G9", "G10", "G11", "G12", "G13", "G14", "G15", "G16",
            /* byte 5 */
            "G17", "G18", "G19", "G20", "G21", "G22", "UNDEF1", "LIGHT_STATE",
            /* byte 6 */
            "BD", "L1", "L2", "L3", "L4", "M1", "M2", "M3",
            /* byte 7 */
            "MR", "LEFT", "DOWN", "TOP", "UNDEF3", "LIGHT", "LIGHT2", "MISC_TOGGLE",
            nullptr
        };

    const char* G13_Key_Tables::G13_NON_PARSED_KEYS[] = {
            "UNDEF1", "LIGHT_STATE", "UNDEF3", "LIGHT",
            "LIGHT2", "UNDEF3", "MISC_TOGGLE",
            nullptr
        };

    const char* G13_Key_Tables::G13_BTN_SEQ[] = {
            "LEFT", "RIGHT", "MIDDLE", "SIDE", "EXTRA",
            nullptr
        };
}
