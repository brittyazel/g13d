//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef KEY_STATE_HPP
#define KEY_STATE_HPP

namespace G13 {
    typedef int LINUX_KEY_VALUE;
    constexpr LINUX_KEY_VALUE BAD_KEY_VALUE = -1;

    /// A key code with up/down indicator
    class KeyState {
    public:
        explicit KeyState(LINUX_KEY_VALUE key = 0, bool down = true);
        [[nodiscard]] LINUX_KEY_VALUE key() const;
        [[nodiscard]] bool is_down() const;

    private:
        LINUX_KEY_VALUE _key;
        bool _down;
    };
}

#endif
