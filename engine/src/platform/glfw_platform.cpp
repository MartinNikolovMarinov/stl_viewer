#include <platform/platform.h>
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

    // TODO: Set up event callbacks here.

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
