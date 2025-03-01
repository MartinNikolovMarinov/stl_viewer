#include <platform.h>
#include <app_error.h>
#include <vulkan_include.h>

#include <core_assert.h>
#include <core_logger.h>

#include <windows.h>
#include <windowsx.h>

using PlatformError::Type::FAILED_TO_REGISTER_WIN32_WINDOW;
using PlatformError::Type::FAILED_TO_CREATE_WIN32_WINDOW;
using PlatformError::Type::FAILED_TO_CREATE_WIN32_KHR_SURFACE;
using PlatformError::Type::FAILED_TO_POLL_FOR_WIN32_EVENT;

namespace {

LRESULT CALLBACK processWin32Messages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Initialize all callback variables to nullptr
WindowCloseCallback windowCloseCallbackWin32 = nullptr;
WindowResizeCallback windowResizeCallbackWin32 = nullptr;
WindowFocusCallback windowFocusCallbackWin32 = nullptr;

KeyCallback keyCallbackWin32 = nullptr;

MouseClickCallback mouseClickCallbackWin32 = nullptr;
MouseMoveCallback mouseMoveCallbackWin32 = nullptr;
MouseScrollCallback mouseScrollCallbackWin32 = nullptr;
MouseEnterOrLeaveCallback mouseEnterOrLeaveCallbackWin32 = nullptr;

HINSTANCE g_hInstance = nullptr;
HWND g_hwnd = nullptr;
constexpr const char* g_windowClassName = "ApplicationWindowClass";
[[maybe_unused]] bool g_initialized = false; // Probably won't use this in release builds.

} // namespace

AppError Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    g_hInstance = GetModuleHandle(NULL);

    // Register the window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = processWin32Messages;
    wc.hInstance = g_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = g_windowClassName;

    if (!RegisterClassExA(&wc)) {
        return createPltErr(FAILED_TO_REGISTER_WIN32_WINDOW, "Failed to register window class");
    }

    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX);
    RECT rect = {0, 0, windowWidth, windowHeight};
    AdjustWindowRect(&rect, style, FALSE);

    g_hwnd = CreateWindowA(g_windowClassName, windowTitle, style,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           rect.right - rect.left, rect.bottom - rect.top,
                           NULL, NULL, g_hInstance, NULL);
    if (!g_hwnd) {
        return createPltErr(FAILED_TO_CREATE_WIN32_WINDOW, "Failed to create window");
    }

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    g_initialized = true;

    return APP_OK;
}

AppError Platform::pollEvents(bool block) {
    Assert(g_initialized, "Platform Layer needs to be initialized");

    MSG msg = {};
    BOOL res;
    if (block) {
        // Blocking: waits for a message
        res = GetMessage(&msg, NULL, 0, 0);
        if (res <= -1) {
            // Error retrieving message
            return createPltErr(FAILED_TO_POLL_FOR_WIN32_EVENT, "GetMessage failed");
        }
        else if (res == 0) {
            // WM_QUIT received
            return APP_OK;
        }
    }
    else {
        // Non-blocking: peek for messages
        res = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (res == 0) {
            // No messages available
            return APP_OK;
        }
        else if (msg.message == WM_QUIT) {
            DispatchMessage(&msg);
            return APP_OK;
        }
    }

    static bool g_isTrackingMouse = false;

    auto getModifiers = []() -> KeyboardModifiers {
        KeyboardModifiers mods = KeyboardModifiers::MODNONE;

        if (GetKeyState(VK_SHIFT) & 0x8000)   mods |= KeyboardModifiers::MODSHIFT;
        if (GetKeyState(VK_CONTROL) & 0x8000) mods |= KeyboardModifiers::MODCONTROL;
        if (GetKeyState(VK_MENU) & 0x8000)    mods |= KeyboardModifiers::MODALT;
        // For Super (Windows key), check either left or right Win key:
        if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000))
            mods |= KeyboardModifiers::MODSUPER;

        return mods;
    };

    auto handleMouseMessage  = [&getModifiers](const MSG& win32Msg, bool isPress) -> void {
        if (!mouseClickCallbackWin32) return;

        POINT pt = { GET_X_LPARAM(win32Msg.lParam), GET_Y_LPARAM(win32Msg.lParam) };
        auto x = pt.x;
        auto y = pt.y;
        MouseButton button = MouseButton::NONE;
        KeyboardModifiers mods = getModifiers();

        switch (win32Msg.message) {
            case WM_LBUTTONDOWN: button = MouseButton::LEFT; break;
            case WM_LBUTTONUP:   button = MouseButton::LEFT; break;
            case WM_RBUTTONDOWN: button = MouseButton::RIGHT; break;
            case WM_RBUTTONUP:   button = MouseButton::RIGHT; break;
            case WM_MBUTTONDOWN: button = MouseButton::MIDDLE; break;
            case WM_MBUTTONUP:   button = MouseButton::MIDDLE; break;
        }

        mouseClickCallbackWin32(isPress, button, x, y, mods);
    };

    auto handleScrollMessage = [](const MSG& win32Msg) -> void {
        if (!mouseScrollCallbackWin32) return;

        POINT pt = { GET_X_LPARAM(win32Msg.lParam), GET_Y_LPARAM(win32Msg.lParam) };
        auto x = pt.x;
        auto y = pt.y;
        MouseScrollDirection direction = MouseScrollDirection::NONE;

        short delta = GET_WHEEL_DELTA_WPARAM(win32Msg.wParam);
        if (delta > 0)      direction = MouseScrollDirection::UP;
        else if (delta < 0) direction = MouseScrollDirection::DOWN;

        mouseScrollCallbackWin32(direction, x, y);
    };

    auto handleKeyMessage = [&getModifiers](const MSG& win32Msg, bool isPress) -> void {
        if (!keyCallbackWin32) return;

        u32 vkcode = u32(win32Msg.wParam); // Virtual-key code
        u32 scancode = u32((win32Msg.lParam >> 16) & 0xFF); // Extract scancode from lParam
        KeyboardModifiers mods = getModifiers();

        keyCallbackWin32(isPress, vkcode, scancode, mods);
    };

    switch (msg.message) {
        case WM_CLOSE: [[fallthrough]];
        case WM_QUIT:
            break;

        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam) };
            if (g_isTrackingMouse) {
                if (mouseMoveCallbackWin32) mouseMoveCallbackWin32(pt.x, pt.y);
            }
            else {
                // Mouse tracking needs to be enabled again.

                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = g_hwnd;
                if (TrackMouseEvent(&tme)) {
                    g_isTrackingMouse = true;
                }
                else {
                    logWarn("Failed to enable mouse tracking. This might have been caused by missed a leave event");
                }

                if (mouseEnterOrLeaveCallbackWin32) mouseEnterOrLeaveCallbackWin32(true);
            }

            return APP_OK;
        }

        case WM_MOUSELEAVE:
            // TODO: What happens if mouse leave is missed?
            g_isTrackingMouse = false;
            if (mouseEnterOrLeaveCallbackWin32) mouseEnterOrLeaveCallbackWin32(false);
            return APP_OK;

        case WM_LBUTTONDOWN: [[fallthrough]];
        case WM_RBUTTONDOWN: [[fallthrough]];
        case WM_MBUTTONDOWN:
            handleMouseMessage(msg, true);
            return APP_OK;

        case WM_LBUTTONUP: [[fallthrough]];
        case WM_RBUTTONUP: [[fallthrough]];
        case WM_MBUTTONUP:
            handleMouseMessage(msg, false);
            return APP_OK;

        case WM_MOUSEWHEEL:
            handleScrollMessage(msg);
            return APP_OK;

        case WM_SYSKEYDOWN: [[fallthrough]];
        case WM_KEYDOWN:
            handleKeyMessage(msg, true);
            return APP_OK;

        case WM_SYSKEYUP: [[fallthrough]];
        case WM_KEYUP:
            handleKeyMessage(msg, false);
            return APP_OK;
    }

    // TranslateMessage(&msg);
    DispatchMessage(&msg);
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
    Assert(g_initialized, "Platform Layer needs to be initialized");

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = g_hInstance;
    createInfo.hwnd = g_hwnd;

    if (VkResult vres = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &outSurface); vres != VK_SUCCESS) {
        logFatal("Failed to create Instance. Error Code: {}", vres);
        return createPltErr(FAILED_TO_CREATE_WIN32_KHR_SURFACE, "Failed to create Win32 Vulkan surface");
    }

    return APP_OK;
}

bool Platform::getFrameBufferSize(u32& width, u32& height) {
    Assert(g_initialized, "Platform Layer needs to be initialized");

    RECT area;
    if (!GetClientRect(g_hwnd, &area)) {
        width = 0;
        height = 0;
        logErr("Failed to get client rectangle for the window");
        return false;
    }

    width = u32(area.right);
    height = u32(area.bottom);
    return true;
}

void Platform::registerWindowCloseCallback(WindowCloseCallback cb) { windowCloseCallbackWin32 = cb; }
void Platform::registerWindowResizeCallback(WindowResizeCallback cb) { windowResizeCallbackWin32 = cb; }
void Platform::registerWindowFocusCallback(WindowFocusCallback cb) { windowFocusCallbackWin32 = cb; }

void Platform::registerKeyCallback(KeyCallback cb) { keyCallbackWin32 = cb; }

void Platform::registerMouseClickCallback(MouseClickCallback cb) { mouseClickCallbackWin32 = cb; }
void Platform::registerMouseMoveCallback(MouseMoveCallback cb) { mouseMoveCallbackWin32 = cb; }
void Platform::registerMouseScrollCallback(MouseScrollCallback cb) { mouseScrollCallbackWin32 = cb; }
void Platform::registerMouseEnterOrLeaveCallback(MouseEnterOrLeaveCallback cb) { mouseEnterOrLeaveCallbackWin32 = cb; }

namespace {

LRESULT CALLBACK processWin32Messages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        // case WM_NCACTIVATE: [[fallthrough]];
        case WM_ACTIVATE: {
            if (windowFocusCallbackWin32) {
                bool focused = (LOWORD(wParam) != WA_INACTIVE);
                windowFocusCallbackWin32(focused);
            }
            return 0;
        }

        case WM_SIZE:
            if (windowResizeCallbackWin32) {
                auto w = LOWORD(lParam);
                auto h = HIWORD(lParam);
                windowResizeCallbackWin32(i32(w), i32(h));
            }
            return 0;

        case WM_CLOSE:
            if (windowCloseCallbackWin32) windowCloseCallbackWin32();
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

} // namespace
