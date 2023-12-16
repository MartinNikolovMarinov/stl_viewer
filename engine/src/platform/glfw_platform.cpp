#include <platform/platform.h>
#include <application/events.h>
#include <application/logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace stlv {

namespace {

struct GlfwPlatformState {
    GLFWwindow* glfwWindow;
};

inline void allocGlfwPltState(PlatformState& pstate) {
    GlfwPlatformState* gflwPltState = reinterpret_cast<GlfwPlatformState*>(memAlloc(sizeof(GlfwPlatformState)));
    pstate.internal = reinterpret_cast<void*>(gflwPltState);
}

inline GlfwPlatformState* toGlfwPltState(PlatformState& pstate) {
    GlfwPlatformState* gflwPltState = reinterpret_cast<GlfwPlatformState*>(pstate.internal);
    return gflwPltState;
}

} // namespace

bool initPlt(PlatformState& pstate) {
    pstate = {};
    allocGlfwPltState(pstate);

    if (i32 ret = glfwInit(); ret != GLFW_TRUE) {
        logFatal("Failed to initialize GLFW: %d", ret);
        return false;
    }

    // GLFW Global Hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // hints that the application is not using OpenGL

    return true;
}

bool startPlt(const PlatformStartInfo& pstartInfo, PlatformState& pstate) {
    GlfwPlatformState* glfwState = toGlfwPltState(pstate);

    // Create GLFW window
    auto w = pstartInfo.windowWidth;
    auto h = pstartInfo.windowHeight;
    auto title = pstartInfo.windowTitle;
    glfwState->glfwWindow = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!glfwState->glfwWindow) {
        logFatal("Failed to create GLFW window.");
        return false;
    }

     const char* errDesc;

    glfwSetKeyCallback(glfwState->glfwWindow, [](GLFWwindow* window, i32 key, i32 scancode, i32 action, i32) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            eventFire(EventCode::APP_QUIT, Event{});
            glfwSetWindowShouldClose(window, GLFW_TRUE); // Probably not necessary, but still.
        }

        if (action == GLFW_PRESS) {
            Event ev;
            ev.data._i32[0] = key;
            ev.data._i32[1] = scancode;
            eventFire(EventCode::APP_KEY_DOWN, ev);
        }
        else if (action == GLFW_RELEASE) {
            Event ev;
            ev.data._i32[0] = key;
            ev.data._i32[1] = scancode;
            eventFire(EventCode::APP_KEY_UP, ev);
        }

        // Ignore GLFW_REPEAT. It's handled by the application statekeeping.
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set key callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetMouseButtonCallback(glfwState->glfwWindow, [](GLFWwindow*, i32 button, i32 action, i32) {
        if (action == GLFW_PRESS) {
            Event ev;
            ev.data._i32[0] = button;
            eventFire(EventCode::APP_MOUSE_DOWN, ev);
        }
        else if (action == GLFW_RELEASE) {
            Event ev;
            ev.data._i32[0] = button;
            eventFire(EventCode::APP_MOUSE_UP, ev);
        }
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set mouse button callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetScrollCallback(glfwState->glfwWindow, [](GLFWwindow*, f64 xoffset, f64 yoffset) {
        Event ev;
        ev.data._f64[0] = xoffset;
        ev.data._f64[1] = yoffset;
        eventFire(EventCode::APP_MOUSE_SCROLL, ev);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set scroll callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetCursorPosCallback(glfwState->glfwWindow, [](GLFWwindow*, f64 xpos, f64 ypos) {
        Event ev;
        ev.data._f64[0] = xpos;
        ev.data._f64[1] = ypos;
        eventFire(EventCode::APP_MOUSE_MOVE, ev);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set cursor position callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetWindowCloseCallback(glfwState->glfwWindow, [](GLFWwindow* window) {
        eventFire(EventCode::APP_QUIT, Event{});
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set window close callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetFramebufferSizeCallback(glfwState->glfwWindow, [](GLFWwindow*, i32 width, i32 height) {
        Event ev;
        ev.data._i32[0] = width;
        ev.data._i32[1] = height;
        eventFire(EventCode::APP_WINDOW_RESIZE, ev);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set framebuffer size callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetWindowPosCallback(glfwState->glfwWindow, [](GLFWwindow*, i32 x, i32 y) {
        Event ev;
        ev.data._i32[0] = x;
        ev.data._i32[1] = y;
        eventFire(EventCode::APP_WINDOW_MOVE, ev);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set window position callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetWindowFocusCallback(glfwState->glfwWindow, [](GLFWwindow*, i32 focused) {
        Event ev;
        ev.data._i32[0] = focused;
        eventFire(EventCode::APP_WINDOW_FOCUS, ev);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set window focus callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetWindowIconifyCallback(glfwState->glfwWindow, [](GLFWwindow*, i32 iconified) {
        Event ev;
        ev.data._i32[0] = iconified == 0;
        eventFire(EventCode::APP_WINDOW_HIDDEN, ev);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set window focus callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    // TODO: Events I should handle at some point:
    // * glfwGetWindowContentScale - content scale is the ratio between the current DPI and the platform's default DPI.
    //                               This is importnat for ui elemnets to be scaled correctly on different monitors.
    // * glfwSetWindowMaximizeCallback - called when the window is maximized or restored.
    // * glfwSetCharCallback - Sets the Unicode character callback. Important for text input.
    // * glfwSetDropCallback - Sets the file drop callback. Important for drag and drop.
    // * glfwSetMonitorCallback - Sets the monitor configuration callback.

    return true;
}

void shutdownPlt(PlatformState& pstate) {
    GlfwPlatformState* glfwState = toGlfwPltState(pstate);
    glfwDestroyWindow(glfwState->glfwWindow);
    glfwTerminate();
    memFree(glfwState);
}

bool pltPollEvents(PlatformState& pstate, f64 timeoutSeconds) {
    GlfwPlatformState* glfwState = toGlfwPltState(pstate);
    if (timeoutSeconds > 0) {
        glfwWaitEventsTimeout(timeoutSeconds);
    }
    else {
        // glfwPollEvents(); // TODO: Uncomment this when swapchaining is implemented.
        glfwWaitEventsTimeout(0.7);
    }
    bool shouldQuit = !glfwWindowShouldClose(glfwState->glfwWindow);
    return shouldQuit;
}

} // namespace stlv
