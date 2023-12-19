#include <platform/platform.h>
#include <application/events.h>
#include <application/logger.h>
#include <application/input.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace stlv {

namespace {

struct GlfwPlatformState {
    GLFWwindow* glfwWindow;
};

inline void allocGlfwPltState(PlatformState& pstate) {
    GlfwPlatformState* gflwPltState = reinterpret_cast<GlfwPlatformState*>(PlatformAllocator::alloc(sizeof(GlfwPlatformState)));
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

    glfwSetCursorEnterCallback(glfwState->glfwWindow, [](GLFWwindow*, i32 entered) {
        Event ev;
        ev.data._bool[0] = entered == GL_TRUE;
        eventFire(EventCode::APP_MOUSE_ENTER, ev);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set cursor enter callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetWindowFocusCallback(glfwState->glfwWindow, [](GLFWwindow*, i32 focused) {
        Event ev;
        ev.data._bool[0] = focused == GL_TRUE;
        eventFire(EventCode::APP_WINDOW_FOCUS, ev);
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set window focus callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    glfwSetWindowIconifyCallback(glfwState->glfwWindow, [](GLFWwindow*, i32 iconified) {
        Event ev;
        ev.data._bool[0] = iconified == GL_FALSE; // false means it was restored, true means it was minimized.
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
    PlatformAllocator::free(glfwState);
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

f64 pltGetMonotinicTime() {
    return glfwGetTime();
}

bool pltGetKey(i32 pltKeyCode, KeyboardKey& key) {
    switch (pltKeyCode) {
        case GLFW_KEY_SPACE:      key.info = KeyInfo::KEY_SPACE;      break;
        case GLFW_KEY_APOSTROPHE: key.info = KeyInfo::KEY_APOSTROPHE; break;
        case GLFW_KEY_COMMA:      key.info = KeyInfo::KEY_COMMA;      break;
        case GLFW_KEY_MINUS:      key.info = KeyInfo::KEY_MINUS;      break;
        case GLFW_KEY_PERIOD:     key.info = KeyInfo::KEY_PERIOD;     break;
        case GLFW_KEY_SLASH:      key.info = KeyInfo::KEY_SLASH;      break;
        case GLFW_KEY_SEMICOLON:  key.info = KeyInfo::KEY_SEMICOLON;  break;
        case GLFW_KEY_EQUAL:      key.info = KeyInfo::KEY_EQUAL;      break;

        case GLFW_KEY_0: key.info = KeyInfo::KEY_0; break;
        case GLFW_KEY_1: key.info = KeyInfo::KEY_1; break;
        case GLFW_KEY_2: key.info = KeyInfo::KEY_2; break;
        case GLFW_KEY_3: key.info = KeyInfo::KEY_3; break;
        case GLFW_KEY_4: key.info = KeyInfo::KEY_4; break;
        case GLFW_KEY_5: key.info = KeyInfo::KEY_5; break;
        case GLFW_KEY_6: key.info = KeyInfo::KEY_6; break;
        case GLFW_KEY_7: key.info = KeyInfo::KEY_7; break;
        case GLFW_KEY_8: key.info = KeyInfo::KEY_8; break;
        case GLFW_KEY_9: key.info = KeyInfo::KEY_9; break;

        case GLFW_KEY_A: key.info = KeyInfo::KEY_A; break;
        case GLFW_KEY_B: key.info = KeyInfo::KEY_B; break;
        case GLFW_KEY_C: key.info = KeyInfo::KEY_C; break;
        case GLFW_KEY_D: key.info = KeyInfo::KEY_D; break;
        case GLFW_KEY_E: key.info = KeyInfo::KEY_E; break;
        case GLFW_KEY_F: key.info = KeyInfo::KEY_F; break;
        case GLFW_KEY_G: key.info = KeyInfo::KEY_G; break;
        case GLFW_KEY_H: key.info = KeyInfo::KEY_H; break;
        case GLFW_KEY_I: key.info = KeyInfo::KEY_I; break;
        case GLFW_KEY_J: key.info = KeyInfo::KEY_J; break;
        case GLFW_KEY_K: key.info = KeyInfo::KEY_K; break;
        case GLFW_KEY_L: key.info = KeyInfo::KEY_L; break;
        case GLFW_KEY_M: key.info = KeyInfo::KEY_M; break;
        case GLFW_KEY_N: key.info = KeyInfo::KEY_N; break;
        case GLFW_KEY_O: key.info = KeyInfo::KEY_O; break;
        case GLFW_KEY_P: key.info = KeyInfo::KEY_P; break;
        case GLFW_KEY_Q: key.info = KeyInfo::KEY_Q; break;
        case GLFW_KEY_R: key.info = KeyInfo::KEY_R; break;
        case GLFW_KEY_S: key.info = KeyInfo::KEY_S; break;
        case GLFW_KEY_T: key.info = KeyInfo::KEY_T; break;
        case GLFW_KEY_U: key.info = KeyInfo::KEY_U; break;
        case GLFW_KEY_V: key.info = KeyInfo::KEY_V; break;
        case GLFW_KEY_W: key.info = KeyInfo::KEY_W; break;
        case GLFW_KEY_X: key.info = KeyInfo::KEY_X; break;
        case GLFW_KEY_Y: key.info = KeyInfo::KEY_Y; break;
        case GLFW_KEY_Z: key.info = KeyInfo::KEY_Z; break;

        case GLFW_KEY_LEFT_BRACKET:  key.info = KeyInfo::KEY_LEFT_BRACKET;  break;
        case GLFW_KEY_BACKSLASH:     key.info = KeyInfo::KEY_BACKSLASH;     break;
        case GLFW_KEY_RIGHT_BRACKET: key.info = KeyInfo::KEY_RIGHT_BRACKET; break;
        case GLFW_KEY_GRAVE_ACCENT:  key.info = KeyInfo::KEY_GRAVE_ACCENT;  break;
        case GLFW_KEY_ESCAPE:        key.info = KeyInfo::KEY_ESCAPE;        break;
        case GLFW_KEY_ENTER:         key.info = KeyInfo::KEY_ENTER;         break;
        case GLFW_KEY_TAB:           key.info = KeyInfo::KEY_TAB;           break;
        case GLFW_KEY_BACKSPACE:     key.info = KeyInfo::KEY_BACKSPACE;     break;
        case GLFW_KEY_INSERT:        key.info = KeyInfo::KEY_INSERT;        break;
        case GLFW_KEY_DELETE:        key.info = KeyInfo::KEY_DELETE;        break;
        case GLFW_KEY_RIGHT:         key.info = KeyInfo::KEY_RIGHT;         break;
        case GLFW_KEY_LEFT:          key.info = KeyInfo::KEY_LEFT;          break;
        case GLFW_KEY_DOWN:          key.info = KeyInfo::KEY_DOWN;          break;
        case GLFW_KEY_UP:            key.info = KeyInfo::KEY_UP;            break;
        case GLFW_KEY_PAGE_UP:       key.info = KeyInfo::KEY_PAGE_UP;       break;
        case GLFW_KEY_PAGE_DOWN:     key.info = KeyInfo::KEY_PAGE_DOWN;     break;
        case GLFW_KEY_HOME:          key.info = KeyInfo::KEY_HOME;          break;
        case GLFW_KEY_END:           key.info = KeyInfo::KEY_END;           break;
        case GLFW_KEY_CAPS_LOCK:     key.info = KeyInfo::KEY_CAPS_LOCK;     break;
        case GLFW_KEY_SCROLL_LOCK:   key.info = KeyInfo::KEY_SCROLL_LOCK;   break;
        case GLFW_KEY_NUM_LOCK:      key.info = KeyInfo::KEY_NUM_LOCK;      break;
        case GLFW_KEY_PRINT_SCREEN:  key.info = KeyInfo::KEY_PRINT_SCREEN;  break;
        case GLFW_KEY_PAUSE:         key.info = KeyInfo::KEY_PAUSE;         break;

        case GLFW_KEY_F1:  key.info = KeyInfo::KEY_F1;  break;
        case GLFW_KEY_F2:  key.info = KeyInfo::KEY_F2;  break;
        case GLFW_KEY_F3:  key.info = KeyInfo::KEY_F3;  break;
        case GLFW_KEY_F4:  key.info = KeyInfo::KEY_F4;  break;
        case GLFW_KEY_F5:  key.info = KeyInfo::KEY_F5;  break;
        case GLFW_KEY_F6:  key.info = KeyInfo::KEY_F6;  break;
        case GLFW_KEY_F7:  key.info = KeyInfo::KEY_F7;  break;
        case GLFW_KEY_F8:  key.info = KeyInfo::KEY_F8;  break;
        case GLFW_KEY_F9:  key.info = KeyInfo::KEY_F9;  break;
        case GLFW_KEY_F10: key.info = KeyInfo::KEY_F10; break;
        case GLFW_KEY_F11: key.info = KeyInfo::KEY_F11; break;
        case GLFW_KEY_F12: key.info = KeyInfo::KEY_F12; break;
        case GLFW_KEY_F13: key.info = KeyInfo::KEY_F13; break;
        case GLFW_KEY_F14: key.info = KeyInfo::KEY_F14; break;
        case GLFW_KEY_F15: key.info = KeyInfo::KEY_F15; break;
        case GLFW_KEY_F16: key.info = KeyInfo::KEY_F16; break;
        case GLFW_KEY_F17: key.info = KeyInfo::KEY_F17; break;
        case GLFW_KEY_F18: key.info = KeyInfo::KEY_F18; break;
        case GLFW_KEY_F19: key.info = KeyInfo::KEY_F19; break;
        case GLFW_KEY_F20: key.info = KeyInfo::KEY_F20; break;
        case GLFW_KEY_F21: key.info = KeyInfo::KEY_F21; break;
        case GLFW_KEY_F22: key.info = KeyInfo::KEY_F22; break;
        case GLFW_KEY_F23: key.info = KeyInfo::KEY_F23; break;
        case GLFW_KEY_F24: key.info = KeyInfo::KEY_F24; break;
        case GLFW_KEY_F25: key.info = KeyInfo::KEY_F25; break;

        case GLFW_KEY_KP_0: key.info = KeyInfo::KEY_KP_0; break;
        case GLFW_KEY_KP_1: key.info = KeyInfo::KEY_KP_1; break;
        case GLFW_KEY_KP_2: key.info = KeyInfo::KEY_KP_2; break;
        case GLFW_KEY_KP_3: key.info = KeyInfo::KEY_KP_3; break;
        case GLFW_KEY_KP_4: key.info = KeyInfo::KEY_KP_4; break;
        case GLFW_KEY_KP_5: key.info = KeyInfo::KEY_KP_5; break;
        case GLFW_KEY_KP_6: key.info = KeyInfo::KEY_KP_6; break;
        case GLFW_KEY_KP_7: key.info = KeyInfo::KEY_KP_7; break;
        case GLFW_KEY_KP_8: key.info = KeyInfo::KEY_KP_8; break;
        case GLFW_KEY_KP_9: key.info = KeyInfo::KEY_KP_9; break;

        case GLFW_KEY_KP_DECIMAL:    key.info = KeyInfo::KEY_KP_DECIMAL;    break;
        case GLFW_KEY_KP_DIVIDE:     key.info = KeyInfo::KEY_KP_DIVIDE;     break;
        case GLFW_KEY_KP_MULTIPLY:   key.info = KeyInfo::KEY_KP_MULTIPLY;   break;
        case GLFW_KEY_KP_SUBTRACT:   key.info = KeyInfo::KEY_KP_SUBTRACT;   break;
        case GLFW_KEY_KP_ADD:        key.info = KeyInfo::KEY_KP_ADD;        break;
        case GLFW_KEY_KP_ENTER:      key.info = KeyInfo::KEY_KP_ENTER;      break;
        case GLFW_KEY_KP_EQUAL:      key.info = KeyInfo::KEY_KP_EQUAL;      break;
        case GLFW_KEY_LEFT_SHIFT:    key.info = KeyInfo::KEY_LEFT_SHIFT;    break;
        case GLFW_KEY_LEFT_CONTROL:  key.info = KeyInfo::KEY_LEFT_CONTROL;  break;
        case GLFW_KEY_LEFT_ALT:      key.info = KeyInfo::KEY_LEFT_ALT;      break;
        case GLFW_KEY_LEFT_SUPER:    key.info = KeyInfo::KEY_LEFT_SUPER;    break;
        case GLFW_KEY_RIGHT_SHIFT:   key.info = KeyInfo::KEY_RIGHT_SHIFT;   break;
        case GLFW_KEY_RIGHT_CONTROL: key.info = KeyInfo::KEY_RIGHT_CONTROL; break;
        case GLFW_KEY_RIGHT_ALT:     key.info = KeyInfo::KEY_RIGHT_ALT;     break;
        case GLFW_KEY_RIGHT_SUPER:   key.info = KeyInfo::KEY_RIGHT_SUPER;   break;
        case GLFW_KEY_MENU:          key.info = KeyInfo::KEY_MENU;          break;

        case GLFW_KEY_WORLD_1: [[fallthrough]];
        case GLFW_KEY_WORLD_2: [[fallthrough]];
        case GLFW_KEY_UNKNOWN: [[fallthrough]];
        default:
            key.info = KeyInfo::UNKNOWN;
            break;
    }

    return true;
}

bool pltGetKey(i32 pltKeyCode, bool& isLeft, bool& isMiddle, bool& isRight) {
    isLeft = false;
    isMiddle = false;
    isRight = false;

    switch (pltKeyCode) {
        case GLFW_MOUSE_BUTTON_LEFT:   isLeft   = true; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: isMiddle = true; break;
        case GLFW_MOUSE_BUTTON_RIGHT:  isRight  = true; break;
    }

    return true;
}

void pltGetRequiredExtensionNames(ExtensionNames& names) {
    u32 glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (u32 i = 0; i < glfwExtensionCount; ++i) {
        names.append(glfwExtensions[i]);
    }
}

} // namespace stlv
