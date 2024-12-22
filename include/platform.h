#pragma once

// IMPORTANT: The platform layer should not use basic.h or anything from core since there are collisions with some of
//            the platforms internal definitions (like defer).

#include <core_types.h>
#include <core_assert.h>
#include <app_error.h>
#include <user_input.h>

using namespace coretypes;

typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

using WindowCloseCallback = void (*)();
using WindowResizeCallback = void (*)(i32 w, i32 h);
using WindowFocusCallback = void (*)(bool gain);
// TODO: Window drag.

using KeyCallback = void (*)(bool isPress, i32 vkcode, i32 scancode, KeyboardModifiers mods);

using MouseClickCallback = void (*)(bool isPress, MouseButton button, i32 x, i32 y, KeyboardModifiers mods);
using MouseMoveCallback = void (*)(i32 x, i32 y);
using MouseScrollCallback = void (*)(MouseScrollDirection direction, i32 x, i32 y);
using MouseEnterOrLeaveCallback = void (*)(bool enter);

struct Platform {
    static void registerWindowCloseCallback(WindowCloseCallback cb);
    static void registerWindowResizeCallback(WindowResizeCallback cb);
    static void registerWindowFocusCallback(WindowFocusCallback cb);

    static void registerKeyCallback(KeyCallback cb);

    static void registerMouseClickCallback(MouseClickCallback cb);
    static void registerMouseMoveCallback(MouseMoveCallback cb);
    static void registerMouseScrollCallback(MouseScrollCallback cb);
    static void registerMouseEnterOrLeaveCallback(MouseEnterOrLeaveCallback cb);

    [[nodiscard]] static AppError init(const char* windowTitle, i32 windowWidth, i32 windowHeight);
    [[nodiscard]] static AppError pollEvents(bool block = false);
    static void shutdown();

    static void requiredVulkanExtsCount(i32& count);
    static void requiredVulkanExts(const char** extensions);
    [[nodiscard]] static AppError createVulkanSurface(VkInstance instance, VkSurfaceKHR& surface);
};
