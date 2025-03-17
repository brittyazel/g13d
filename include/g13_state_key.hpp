//
// Created by britt on 3/15/25.
//

#ifndef G13_STATE_KEY_HPP
#define G13_STATE_KEY_HPP

namespace G13 {
    typedef int LINUX_KEY_VALUE;
    constexpr LINUX_KEY_VALUE BAD_KEY_VALUE = -1;

    /// A key code with up/down indicator
    class G13_State_Key {
    public:
        explicit G13_State_Key(LINUX_KEY_VALUE key = 0, bool down = true);
        [[nodiscard]] LINUX_KEY_VALUE key() const;
        [[nodiscard]] bool is_down() const;

    private:
        LINUX_KEY_VALUE _key;
        bool _down;
    };
}

#endif //G13_STATE_KEY_HPP
