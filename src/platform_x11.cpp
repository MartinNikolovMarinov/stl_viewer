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

// Initialize all callback variables to nullptr
WindowCloseCallback windowCloseCallbackX11 = nullptr;
WindowResizeCallback windowResizeCallbackX11 = nullptr;
WindowFocusCallback windowFocusCallbackX11 = nullptr;

KeyCallback keyCallbackX11 = nullptr;

MouseClickCallback mouseClickCallbackX11 = nullptr;
MouseMoveCallback mouseMoveCallbackX11 = nullptr;
MouseScrollCallback mouseScrollCallbackX11 = nullptr;
MouseEnterOrLeaveCallback mouseEnterOrLeaveCallbackX11 = nullptr;

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

AppError Platform::pollEvents(bool block) {
    Assert(g_initialized, "Platform Layer needs to be initialized");

    if (!block && !XPending(g_display)) {
        return APP_OK;
    }

    auto getModifiers = [](u32 m) -> KeyboardModifiers {
        KeyboardModifiers ret;

        if (m & ShiftMask)   ret |= KeyboardModifiers::MODSHIFT;
        if (m & ControlMask) ret |= KeyboardModifiers::MODCONTROL;
        if (m & Mod1Mask)    ret |= KeyboardModifiers::MODALT;
        if (m & Mod4Mask)    ret |= KeyboardModifiers::MODSUPER;

        return ret;
    };

    XEvent xevent;
    XNextEvent(g_display, &xevent);

    auto handleMouseClickEvent = [&getModifiers](XEvent& xevent, bool isPress) -> void {
        KeyboardModifiers mods = getModifiers(xevent.xbutton.state);
        i32 x = i32(xevent.xbutton.x);
        i32 y = i32(xevent.xbutton.y);

        if (mouseScrollCallbackX11) {
            if (xevent.xbutton.button == Button4) {
                mouseScrollCallbackX11(MouseScrollDirection::UP, x, y);
                return;
            }
            else if (xevent.xbutton.button == Button5) {
                mouseScrollCallbackX11(MouseScrollDirection::DOWN, x, y);
                return;
            }
        }

        if (mouseClickCallbackX11) {
            if (xevent.xbutton.button == Button1) {
                mouseClickCallbackX11(isPress, MouseButton::LEFT, x, y, mods);
                return;
            }
            else if (xevent.xbutton.button == Button2) {
                mouseClickCallbackX11(isPress, MouseButton::MIDDLE, x, y, mods);
                return;
            }
            else if (xevent.xbutton.button == Button3) {
                mouseClickCallbackX11(isPress, MouseButton::RIGHT, x, y, mods);
                return;
            }
            else {
                mouseClickCallbackX11(isPress, MouseButton::NONE, x, y, mods);
                logDebug("Unknown Mouse Button.");
                return;
            }
        }
    };

    auto handleKeyEvent = [&getModifiers](XEvent& xevent, bool isPress) -> void {
        if (keyCallbackX11) {
            u32 vkcode = XLookupKeysym(&xevent.xkey, 0);
            u32 scancode = xevent.xkey.keycode;
            KeyboardModifiers mods = getModifiers(xevent.xkey.state);
            keyCallbackX11(isPress, vkcode, scancode, mods);
        }
    };

    switch (xevent.type) {
        case ClientMessage: {
            if (Atom(xevent.xclient.data.l[0]) == g_wmDeleteWindow) {
                if (windowCloseCallbackX11) windowCloseCallbackX11();
                return APP_OK;
            }
            break;
        }

        case ConfigureNotify:
            if (windowResizeCallbackX11) {
                i32 w = i32(xevent.xconfigure.width);
                i32 h = i32(xevent.xconfigure.height);
                windowResizeCallbackX11(w, h);
            }
            return APP_OK;

        case ButtonPress:
            handleMouseClickEvent(xevent, true);
            return APP_OK;

        case ButtonRelease:
            handleMouseClickEvent(xevent, false);
            return APP_OK;

        case KeyPress:
            handleKeyEvent(xevent, true);
            return APP_OK;

        case KeyRelease:
            handleKeyEvent(xevent, false);
            return APP_OK;

        case MotionNotify: {
            if (mouseMoveCallbackX11) {
                i32 x = i32(xevent.xmotion.x);
                i32 y = i32(xevent.xmotion.y);
                mouseMoveCallbackX11(x, y);
            }
            return APP_OK;
        }

        case EnterNotify: {
            if (mouseEnterOrLeaveCallbackX11) {
                // i32 x = xevent.xcrossing.x;
                // i32 y = xevent.xcrossing.y;
                mouseEnterOrLeaveCallbackX11(true);
            }
            return APP_OK;
        }

        case LeaveNotify: {
            if (mouseEnterOrLeaveCallbackX11) {
                // i32 x = xevent.xcrossing.x;
                // i32 y = xevent.xcrossing.y;
                mouseEnterOrLeaveCallbackX11(false);
            }
            return APP_OK;
        }

        case FocusIn:
            if (windowFocusCallbackX11) windowFocusCallbackX11(true);
            return APP_OK;

        case FocusOut:
            if (windowFocusCallbackX11) windowFocusCallbackX11(false);
            return APP_OK;

        default:
            break;
    }

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

void Platform::registerWindowCloseCallback(WindowCloseCallback cb) { windowCloseCallbackX11 = cb; }
void Platform::registerWindowResizeCallback(WindowResizeCallback cb) { windowResizeCallbackX11 = cb; }
void Platform::registerWindowFocusCallback(WindowFocusCallback cb) { windowFocusCallbackX11 = cb; }

void Platform::registerKeyCallback(KeyCallback cb) { keyCallbackX11 = cb; }

void Platform::registerMouseClickCallback(MouseClickCallback cb) { mouseClickCallbackX11 = cb; }
void Platform::registerMouseMoveCallback(MouseMoveCallback cb) { mouseMoveCallbackX11 = cb; }
void Platform::registerMouseScrollCallback(MouseScrollCallback cb) { mouseScrollCallbackX11 = cb; }
void Platform::registerMouseEnterOrLeaveCallback(MouseEnterOrLeaveCallback cb) { mouseEnterOrLeaveCallbackX11 = cb; }

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char** extensions) {
    extensions[0] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

AppError Platform::createVulkanSurface(VkInstance instance, VkSurfaceKHR& outSurface) {
    Assert(g_initialized, "Platform Layer needs to be initialized");

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
