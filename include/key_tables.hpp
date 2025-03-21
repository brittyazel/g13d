//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef KEY_TABLES_HPP
#define KEY_TABLES_HPP

namespace G13 {
    /// Various static key tables
    class G13_Key_Tables {
    public:
        /*! sequence containing the G13 keys. The order is very specific, with the position of each
         * item corresponding to a specific bit in the G13's USB message
         * format.  Do NOT remove or insert items in this list.
         */
        static const char* G13_KEY_STRINGS[];

        /*! sequence containing the
         * G13 keys that shouldn't be tested input.  These aren't actually keys,
         * but they are in the bitmap defined by G13_KEY_STRINGS.
         */
        static const char* G13_NON_PARSED_KEYS[];

        /*! G13_BTN_SEQ is a Boost Preprocessor sequence containing the
         * names of button events we can send through binding actions.
         * These correspond to BTN_xxx value definitions in <linux/input.h>,
         * i.e. LEFT is BTN_LEFT, RIGHT is BTN_RIGHT, etc.
         *
         * The binding names have prefix M to avoid naming conflicts.
         * e.g. LEFT keyboard button and LEFT mouse button
         * i.e. LEFT mouse button is named MLEFT, MIDDLE mouse button is MMIDDLE
         */
        static const char* G13_BTN_SEQ[];
    };
}
#endif
