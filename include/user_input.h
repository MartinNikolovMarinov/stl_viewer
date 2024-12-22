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
