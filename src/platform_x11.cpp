#include <core_logger.h>
#include <platform.h>
#include <vulkan_include.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

using PlatformError::Type::FAILED_TO_CREATE_X11_DISPLAY;
using PlatformError::Type::FAILED_TO_CREATE_X11_WINDOW;
using PlatformError::Type::FAILED_TO_CREATE_X11_KHR_XLIB_SURFACE;

namespace {

Display* g_display = nullptr;
Window g_window = 0;
Atom g_wmDeleteWindow;
[[maybe_unused]] bool g_initialized = false; // Probably won't use this in release builds.

} // namespace

AppError Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    // Open a connection to the X server, which manages the display (i.e., the screen).
    g_display = XOpenDisplay(nullptr);
    if (!g_display) {
        return createPltErr(FAILED_TO_CREATE_X11_DISPLAY, "Failed to open X display");
    }

    i32 screen = DefaultScreen(g_display); // TODO2: [MULTI_MONITOR] This won't work great on a multi-monitor setup.
    // The root window is the main top-level window managed by the X server for that screen. New windows are often
    // created as children of the root window.
    Window root = RootWindow(g_display, screen);

    // Creates a basic top-level window.
    g_window = XCreateSimpleWindow(g_display,
                                    root, // Parent window is the root
                                    10, 10, // The x and y coordinates of the window’s position on the screen. TODO: Manage these through config parameters.
                                    windowWidth, windowHeight, // Initial width and height
                                    1, // Border width in pixels.
                                    BlackPixel(g_display, screen), // Border color.
                                    WhitePixel(g_display, screen)); // Background color.
    if (!g_window) {
        return createPltErr(FAILED_TO_CREATE_X11_WINDOW, "Failed to create X11 window");
    }

    // Requests that the X server report the events associated with the specified event mask.
    long eventMask = ExposureMask |       // Receive expose events (when a portion of the window needs to be redrawn).
                     KeyPressMask |       // Receive keyboard press events.
                     KeyReleaseMask |     // Key release events.
                     ButtonPressMask |    // Mouse button press.
                     ButtonReleaseMask |  // Mouse button release.
                     PointerMotionMask |  // Mouse movement events.
                     EnterWindowMask |    // Mouse enters the window.
                     LeaveWindowMask |    // Mouse leaves the window.
                     FocusChangeMask |    // Window gains or loses focus.
                     StructureNotifyMask; ;// Window structure changes (resize, close, etc.).
    XSelectInput(g_display, g_window, eventMask);

    // Set the window’s title (visible in the title bar).
    XStoreName(g_display, g_window, windowTitle);

    // Registers the WM_DELETE_WINDOW atom, which is used to handle the window manager’s close button.
    // Atom is basically an ID for a string used by the window manager to handle events.
    g_wmDeleteWindow = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    // Informs the window manager that the application wants to handle the WM_DELETE_WINDOW message.
    XSetWMProtocols(g_display, g_window, &g_wmDeleteWindow, 1);

    // Maps the window on the screen, making it visible.
    XMapWindow(g_display, g_window);
    // Flushes all pending requests to the X server.
    XFlush(g_display);

    g_initialized = true;
    return APP_OK;
}

AppError Platform::pollEvent(PlatformEvent& ev, bool block) {
    Assert(g_initialized, "Platform Layer needs to be initialized");

    using EvType = PlatformEvent::Type;

    if (!block && !XPending(g_display)) {
        ev.type = EvType::NOOP;
        return APP_OK;
    }

    auto convertX11Modifiers = [](u32 m) -> KeyboardModifiers {
        KeyboardModifiers ret;

        if (m & ShiftMask)   ret |= KeyboardModifiers::MODSHIFT;
        if (m & ControlMask) ret |= KeyboardModifiers::MODCONTROL;
        if (m & Mod1Mask)    ret |= KeyboardModifiers::MODALT;
        if (m & Mod4Mask)    ret |= KeyboardModifiers::MODSUPER;

        return ret;
    };

    XEvent xevent;
    XNextEvent(g_display, &xevent);

    auto handleMouseEvent = [](const XEvent& xevent, PlatformEvent& ev, bool isPress) -> AppError {
        switch (xevent.xbutton.button) {
            case Button1:
                ev.type = isPress ? EvType::MOUSE_PRESS : EvType::MOUSE_RELEASE;
                ev.data.mouse.button = MouseButton::LEFT;
                break;
            case Button2:
                ev.type = isPress ? EvType::MOUSE_PRESS : EvType::MOUSE_RELEASE;
                ev.data.mouse.button = MouseButton::MIDDLE;
                break;
            case Button3:
                ev.type = isPress ? EvType::MOUSE_PRESS : EvType::MOUSE_RELEASE;
                ev.data.mouse.button = MouseButton::RIGHT;
                break;
            case Button4:
                ev.type = isPress ? EvType::MOUSE_SCROLL_START : EvType::MOUSE_SCROLL_STOP;
                ev.data.scroll.direction = MouseScrollDirection::UP;
                break;
            case Button5:
                ev.type = isPress ? EvType::MOUSE_SCROLL_START : EvType::MOUSE_SCROLL_STOP;
                ev.data.scroll.direction = MouseScrollDirection::DOWN;
                break;
            default: [[unlikely]]
                logDebug("Unknown Mouse Button.");
                ev.type = EvType::UNKNOWN;
                return APP_OK; // Early return
        }

        ev.data.mouse.x = xevent.xbutton.x;
        ev.data.mouse.y = xevent.xbutton.y;
        return APP_OK;
    };

    switch (xevent.type) {
        case ClientMessage: {
            if (Atom(xevent.xclient.data.l[0]) == g_wmDeleteWindow) {
                ev.type = EvType::WINDOW_CLOSE;
                return APP_OK;
            }
            break;
        }

        case ConfigureNotify:
            ev.type = EvType::WINDOW_RESIZE;
            ev.data.resize.width = xevent.xconfigure.width;
            ev.data.resize.height = xevent.xconfigure.height;
            return APP_OK;

        case ButtonPress:
            return handleMouseEvent(xevent, ev, true);

        case ButtonRelease:
            return handleMouseEvent(xevent, ev, false);

        case KeyPress:
            ev.type = EvType::KEY_PRESS;
            ev.data.key.scancode = xevent.xkey.keycode;
            ev.data.key.vkcode = XLookupKeysym(&xevent.xkey, 0);
            ev.data.key.mods = convertX11Modifiers(xevent.xkey.state);
            return APP_OK;

        case KeyRelease:
            ev.type = EvType::KEY_RELEASE;
            ev.data.key.scancode = xevent.xkey.keycode;
            ev.data.key.vkcode = XLookupKeysym(&xevent.xkey, 0);
            ev.data.key.mods = convertX11Modifiers(xevent.xkey.state);
            return APP_OK;

        case MotionNotify:
            ev.type = EvType::MOUSE_MOVE;
            ev.data.mouse.button = MouseButton::NONE;
            ev.data.mouse.x = xevent.xmotion.x;
            ev.data.mouse.y = xevent.xmotion.y;
            return APP_OK;

        case EnterNotify:
            ev.type = EvType::MOUSE_ENTER;
            ev.data.mouse.button = MouseButton::NONE;
            ev.data.mouse.x = xevent.xcrossing.x;
            ev.data.mouse.y = xevent.xcrossing.y;
            return APP_OK;

        case LeaveNotify:
            ev.type = EvType::MOUSE_LEAVE;
            ev.data.mouse.button = MouseButton::NONE;
            ev.data.mouse.x = xevent.xcrossing.x;
            ev.data.mouse.y = xevent.xcrossing.y;
            return APP_OK;

        case FocusIn:
            ev.type = EvType::FOCUS_GAINED;
            return APP_OK;

        case FocusOut:
            ev.type = EvType::FOCUS_LOST;
            return APP_OK;

        default:
            break;
    }

    ev.type = EvType::UNKNOWN;
    return APP_OK;
}

void Platform::shutdown() {
    g_initialized = false; // Mark the platform as uninitialized

    if (g_window) {
        XDestroyWindow(g_display, g_window); // Destroy the X11 window
        g_window = 0; // Reset the global window ID
    }

    if (g_display) {
        XCloseDisplay(g_display); // Close the X11 display connection
        g_display = nullptr; // Reset the global display pointer
    }
}

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char** extensions) {
    extensions[0] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

AppError Platform::createVulkanSurface(VkInstance instance, VkSurfaceKHR& outSurface) {
    VkXlibSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = g_display;
    createInfo.window = g_window;

    VkResult result = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &outSurface);
    if (result != VK_SUCCESS) {
        // This could technically be a render error as well.
        return createPltErr(FAILED_TO_CREATE_X11_KHR_XLIB_SURFACE, "Failed to create Xlib Vulkan surface.");
    }

    return APP_OK;
}
