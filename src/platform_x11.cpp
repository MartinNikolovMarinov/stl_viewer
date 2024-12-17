#include <platform.h>
#include <vulkan_include.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>

static Display* g_display = nullptr;
static Window g_window = 0;
static Atom g_wmDeleteWindow;

Platform::Error Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    g_display = XOpenDisplay(nullptr);
    if (!g_display) {
        std::cerr << "Failed to open X display.\n";
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    int screen = DefaultScreen(g_display);
    Window root = RootWindow(g_display, screen);

    // Create a simple X11 window
    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
    g_window = XCreateSimpleWindow(g_display, root, 10, 10, windowWidth, windowHeight, 1,
                                   BlackPixel(g_display, screen), WhitePixel(g_display, screen));
    if (!g_window) {
        std::cerr << "Failed to create X11 window.\n";
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    // Set window title
    XStoreName(g_display, g_window, windowTitle);

    // To handle window close events
    g_wmDeleteWindow = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_display, g_window, &g_wmDeleteWindow, 1);

    XMapWindow(g_display, g_window);
    XFlush(g_display);

    return Platform::Error::SUCCESS;
}

Platform::Error Platform::start() {
    XEvent event;
    while (true) {
        XNextEvent(g_display, &event);
        if (event.type == ClientMessage && (Atom)event.xclient.data.l[0] == g_wmDeleteWindow) {
            // Window close button pressed
            break;
        }
        // Handle other events (resize, keypress, etc.) if needed
    }

    // Cleanup
    XDestroyWindow(g_display, g_window);
    XCloseDisplay(g_display);
    g_window = 0;
    g_display = nullptr;

    return Platform::Error::SUCCESS;
}

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char** extensions) {
    extensions[0] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

Platform::Error Platform::createVulkanSurface(VkInstance instance, VkSurfaceKHR& outSurface) {
    VkXlibSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = g_display;
    createInfo.window = g_window;

    VkResult result = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &outSurface);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Xlib Vulkan surface.\n";
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    return Platform::Error::SUCCESS;
}
