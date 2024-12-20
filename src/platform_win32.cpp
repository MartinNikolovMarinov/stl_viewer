#include <platform.h>
#include <app_error.h>
#include <vulkan_include.h>
#include <core_logger.h>

#include <windows.h>
#include <windowsx.h>

using PlatformError::Type::FAILED_TO_REGISTER_WIN32_WINDOW;
using PlatformError::Type::FAILED_TO_CREATE_WIN32_WINDOW;
using PlatformError::Type::FAILED_TO_CREATE_WIN32_KHR_SURFACE;
using PlatformError::Type::FAILED_TO_POLL_FOR_WIN32_EVENT;

namespace {

HINSTANCE g_hInstance = nullptr;
HWND g_hwnd = nullptr;
const char* g_windowClassName = "ApplicationWindowClass";
[[maybe_unused]] bool g_initialized = false; // Probably won't use this in release builds.

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

} // namespace

AppError Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    g_hInstance = GetModuleHandle(NULL);

    // Register the window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = g_windowClassName;

    if (!RegisterClassExA(&wc)) {
        return createPltErr(FAILED_TO_REGISTER_WIN32_WINDOW, "Failed to register window class.");
    }

    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX);
    RECT rect = {0, 0, windowWidth, windowHeight};
    AdjustWindowRect(&rect, style, FALSE);

    g_hwnd = CreateWindowA(g_windowClassName, windowTitle, style,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           rect.right - rect.left, rect.bottom - rect.top,
                           NULL, NULL, g_hInstance, NULL);

    if (!g_hwnd) {
        return createPltErr(FAILED_TO_CREATE_WIN32_WINDOW, "Failed to create window.");
    }

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    g_initialized = true;

    return APP_OK;
}

AppError Platform::pollEvent(PlatformEvent& ev, bool block) {
    Assert(g_initialized, "Platform Layer needs to be initialized");

    using EvType = PlatformEvent::Type;

    MSG msg = {};
    BOOL res;
    if (block) {
        // Blocking: waits for a message
        res = GetMessage(&msg, NULL, 0, 0);
        if (res <= -1) {
            // Error retrieving message
            ev.type = EvType::UNKNOWN;
            return createPltErr(FAILED_TO_POLL_FOR_WIN32_EVENT, "GetMessage failed");
        }
        else if (res == 0) {
            // WM_QUIT received
            ev.type = EvType::WINDOW_CLOSE;
            return APP_OK;
        }
    }
    else {
        // Non-blocking: peek for messages
        res = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (res == 0) {
            // No messages available
            ev.type = EvType::NOOP;
            return APP_OK;
        }
        else if (msg.message == WM_QUIT) {
            ev.type = EvType::WINDOW_CLOSE;
            return APP_OK;
        }
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);

    auto handleMousePressMsg = [](const MSG& win32Msg, PlatformEvent& ev) {
        POINT pt = { GET_X_LPARAM(win32Msg.lParam), GET_Y_LPARAM(win32Msg.lParam) };
        ScreenToClient(g_hwnd, &pt);
        ev.data.mouse.x = pt.x;
        ev.data.mouse.y = pt.y;

        switch (win32Msg.message) {
            case WM_LBUTTONDOWN:
                ev.type = EvType::MOUSE_PRESS;
                ev.data.mouse.button = MouseButton::LEFT;
                break;
            case WM_LBUTTONUP:
                ev.type = EvType::MOUSE_RELEASE;
                ev.data.mouse.button = MouseButton::LEFT;
                break;
            case WM_RBUTTONDOWN:
                ev.type = EvType::MOUSE_PRESS;
                ev.data.mouse.button = MouseButton::RIGHT;
                break;
            case WM_RBUTTONUP:
                ev.type = EvType::MOUSE_RELEASE;
                ev.data.mouse.button = MouseButton::RIGHT;
                break;
            case WM_MBUTTONDOWN:
                ev.type = EvType::MOUSE_PRESS;
                ev.data.mouse.button = MouseButton::MIDDLE;
                break;
            case WM_MBUTTONUP:
                ev.type = EvType::MOUSE_RELEASE;
                ev.data.mouse.button = MouseButton::MIDDLE;
                break;
        }
    };

    auto handleScrollMsg = [](const MSG& win32Msg, PlatformEvent& ev) -> void {
        POINT pt = { GET_X_LPARAM(win32Msg.lParam), GET_Y_LPARAM(win32Msg.lParam) };
        // Convert screen coords to client coords
        ScreenToClient(g_hwnd, &pt);
        ev.data.scroll.x = pt.x;
        ev.data.scroll.y = pt.y;
        ev.type = EvType::MOUSE_SCROLL;

        short delta = GET_WHEEL_DELTA_WPARAM(win32Msg.wParam);
        if (delta > 0)      ev.data.scroll.direction = MouseScrollDirection::UP;
        else if (delta < 0) ev.data.scroll.direction = MouseScrollDirection::DOWN;
        else                ev.data.scroll.direction = MouseScrollDirection::NONE;
    };

    auto convertWin32Modifiers = []() -> KeyboardModifiers {
        KeyboardModifiers mods = KeyboardModifiers::MODNONE;

        if (GetKeyState(VK_SHIFT) & 0x8000)   mods |= KeyboardModifiers::MODSHIFT;
        if (GetKeyState(VK_CONTROL) & 0x8000) mods |= KeyboardModifiers::MODCONTROL;
        if (GetKeyState(VK_MENU) & 0x8000)    mods |= KeyboardModifiers::MODALT;
        // For Super (Windows key), check either left or right Win key:
        if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000))
            mods |= KeyboardModifiers::MODSUPER;

        return mods;
    };

    // FIXME: Alt key modifier does not work. This code is buggy, fix it!
    // FIXME: Resize event is not handled!

    switch (msg.message) {
        case WM_CLOSE:
        case WM_QUIT:
            ev.type = EvType::WINDOW_CLOSE;
            return APP_OK;

        case WM_SIZE: {
            ev.type = EvType::WINDOW_RESIZE;
            ev.data.resize.width = LOWORD(msg.lParam);
            ev.data.resize.height = HIWORD(msg.lParam);
            return APP_OK;
        }

        case WM_MOUSEWHEEL:
            handleScrollMsg(msg, ev);
            return APP_OK;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            handleMousePressMsg(msg, ev);
            return APP_OK;

        case WM_KEYDOWN: {
            ev.type = EvType::KEY_PRESS;
            ev.data.key.scancode = i32((msg.lParam >> 16) & 0xFF); // Extract scancode from lParam
            ev.data.key.vkcode = i32(msg.wParam); // Virtual-key code
            ev.data.key.mods = convertWin32Modifiers();
            return APP_OK;
        }

        case WM_KEYUP: {
            ev.type = EvType::KEY_RELEASE;
            ev.data.key.scancode = i32((msg.lParam >> 16) & 0xFF);
            ev.data.key.vkcode = i32(msg.wParam);
            ev.data.key.mods = convertWin32Modifiers();
            return APP_OK;
        }

        default:
            break;
    }

    ev.type = EvType::UNKNOWN;
    return APP_OK;
}

void Platform::shutdown() {
    g_initialized = false;

    if (g_hwnd) {
        DestroyWindow(g_hwnd);
        g_hwnd = nullptr;
    }

    if (g_hInstance) {
        UnregisterClassA(g_windowClassName, g_hInstance);
        g_hInstance = nullptr;
    }
}

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char** extensions) {
    extensions[0] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

AppError Platform::createVulkanSurface(VkInstance instance, VkSurfaceKHR& outSurface) {
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = g_hInstance;
    createInfo.hwnd = g_hwnd;

    if (VkResult vres = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &outSurface); vres != VK_SUCCESS) {
        logFatal("Failed to create Instance. Error Code: %d", vres);
        return createPltErr(FAILED_TO_CREATE_WIN32_KHR_SURFACE, "Failed to create Win32 Vulkan surface.");
    }

    return APP_OK;
}
