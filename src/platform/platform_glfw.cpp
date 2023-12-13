#include <fwd_decl.h>
#include <platform/platform.h>
#include <application/events.h>
#include <application/logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace stlv {

struct GlfwPlatformState {
    GLFWwindow* glfwWindow;
};

bool initPlatform(PlatformState& state, const char* appName, i32 x, i32 y, i32 width, i32 height) {
    GlfwPlatformState* glfwState = reinterpret_cast<GlfwPlatformState*>(memAlloc(sizeof(GlfwPlatformState)));
    state.internal = reinterpret_cast<void*>(glfwState);

    if (i32 ret = glfwInit(); ret != GLFW_TRUE) {
        logFatal("Failed to initialize GLFW: %d", ret);
        return false;
    }

    // Hint to GLFW that the application is not using OpenGL:
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwState->glfwWindow = glfwCreateWindow(width, height, appName, nullptr, nullptr);
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
            eventFire(EventCode::APP_CODE_KEY_DOWN, ev);
        }
        else if (action == GLFW_RELEASE) {
            Event ev;
            ev.data._i32[0] = key;
            ev.data._i32[1] = scancode;
            eventFire(EventCode::APP_CODE_KEY_UP, ev);
        }
    });
    if (i32 errCode = glfwGetError(&errDesc); errCode != GLFW_NO_ERROR) {
        logFatal("Failed to set key callback; GLFW error: %d, %s", errCode, errDesc);
        return false;
    }

    return true;
}

bool pollOsEvents(PlatformState& state) {
    GlfwPlatformState* glfwState = reinterpret_cast<GlfwPlatformState*>(state.internal);
    glfwWaitEventsTimeout(0.7); // TODO: This timeout might be too short.
    // glfwPollEvents();
    bool shouldQuit = !glfwWindowShouldClose(glfwState->glfwWindow);
    return shouldQuit;
}

void destroyPlatform(PlatformState& state) {
    GlfwPlatformState* glfwState = reinterpret_cast<GlfwPlatformState*>(state.internal);
    glfwDestroyWindow(glfwState->glfwWindow);
    glfwTerminate();
    memFree(glfwState);
}

} // namespace stlv
