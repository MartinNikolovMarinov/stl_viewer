#include <fwd_decl.h>
#include <platform/platform.h>
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

    return true;
}

} // namespace stlv
