#pragma once

#include <basic.h>

enum struct KeyboardModifiers : u8 {
    MODNONE = 0,
    MODSHIFT = 1 << 1,
    MODCONTROL = 2 << 1,
    MODALT = 3 << 2,
    MODSUPER = 4 << 3,
};

inline constexpr KeyboardModifiers operator|(KeyboardModifiers lhs, KeyboardModifiers rhs) {
    return KeyboardModifiers(u8(lhs) | u8(rhs));
}

inline constexpr KeyboardModifiers operator&(KeyboardModifiers lhs, KeyboardModifiers rhs) {
    return KeyboardModifiers(u8(lhs) & u8(rhs));
}

inline constexpr KeyboardModifiers operator^(KeyboardModifiers lhs, KeyboardModifiers rhs) {
    return KeyboardModifiers(u8(lhs) ^ u8(rhs));
}

// Define compound assignment operators
inline constexpr KeyboardModifiers& operator|=(KeyboardModifiers& lhs, KeyboardModifiers rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline constexpr KeyboardModifiers& operator&=(KeyboardModifiers& lhs, KeyboardModifiers rhs) {
    lhs = lhs & rhs;
    return lhs;
}

inline constexpr KeyboardModifiers& operator^=(KeyboardModifiers& lhs, KeyboardModifiers rhs) {
    lhs = lhs ^ rhs;
    return lhs;
}

const char* keyModifiersToCptr(KeyboardModifiers m);

enum struct MouseButton : u8 {
    NONE,
    LEFT,
    MIDDLE,
    RIGHT,
};

enum struct MouseScrollDirection : u8 {
    NONE,
    UP,
    DOWN,
};

struct PlatformEvent {
    enum struct Type : i32 {
        NOOP,

        WINDOW_CLOSE,
        WINDOW_RESIZE,

        MOUSE_PRESS,
        MOUSE_RELEASE,
        MOUSE_SCROLL_START,
        MOUSE_SCROLL_STOP,
        MOUSE_MOVE,
        MOUSE_ENTER,
        MOUSE_LEAVE,

        KEY_PRESS,
        KEY_RELEASE,

        FOCUS_GAINED,
        FOCUS_LOST,

        UNKNOWN
    };

    Type type = Type::NOOP;
    union {
        struct {
            i32 width;
            i32 height;
        } resize;

        struct {
            MouseButton button;
            i32 x; // FIXME: The different OS UI systems have different coordinate systems! I need to unify the coords.
            i32 y;
        } mouse;

        struct {
            // TODO2: This event might need a dx for scroll speed.
            MouseScrollDirection direction;
            i32 x; // FIXME: I need to unify the coords.
            i32 y;
        } scroll;

         struct {
            i32 vkcode; // virtual key code
            i32 scancode;
            KeyboardModifiers mods;
        } key;
    } data;

    void logTraceEv();
};
