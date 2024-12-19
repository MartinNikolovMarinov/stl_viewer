#pragma once

// IMPORTANT: The platform layer should not use basic.h or anything from core since there are collisions with some of
//            the platforms internal definitions (like defer).

#include <core_types.h>
#include <core_assert.h>
#include <app_error.h>

using namespace coretypes;

typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

enum struct KeyboardModifiers : u8 {
    MOD_NONE = 0,
    MOD_SHIFT = 1 << 1,
    MOD_CONTROL = 2 << 1,
    MOD_ALT = 3 << 2,
    MOD_SUPER = 4 << 3,
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
    LEFT,
    MIDDLE,
    RIGHT,
};

enum struct MouseScrollDirection : u8 {
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
        MOUSE_SCROLL,
        KEY_PRESS,
        KEY_RELEASE,

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
            i32 x;
            i32 y;
        } mouse;

        struct {
            MouseScrollDirection direction;
            i32 x;
            i32 y;
        } scroll;

         struct {
            i32 raw;
            i32 scancode;
            KeyboardModifiers mods;
        } key;
    } data;

    void logTraceEv();
};

struct Platform {
    [[nodiscard]] static AppError init(const char* windowTitle, i32 windowWidth, i32 windowHeight);
    [[nodiscard]] static AppError pollEvent(PlatformEvent& ev, bool block = false);

    static void requiredVulkanExtsCount(i32& count);
    static void requiredVulkanExts(const char** extensions);
    [[nodiscard]] static AppError createVulkanSurface(VkInstance instance, VkSurfaceKHR& surface);
};
