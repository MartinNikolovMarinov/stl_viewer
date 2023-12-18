#include <fwd_internal.h>

#include <application/logger.h>
#include <renderer/renderer_backend.h>
#include <renderer/backend/renderer_vulkan.h>
#include <platform/platform.h>

namespace stlv {

bool initRendererBE(RendererBackend& backend, PlatformState&) {
    backend = {};

    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.pApplicationName = "STLV";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "STLV Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);

    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = 0;
    instanceInfo.ppEnabledExtensionNames = nullptr;
    instanceInfo.enabledLayerCount = 0;
    instanceInfo.ppEnabledLayerNames = nullptr;

    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan instance...");
    if (vkCreateInstance(&instanceInfo, backend.allocator, &backend.instance) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void shutdownRendererBE(RendererBackend& backend) {
    vkDestroyInstance(backend.instance, backend.allocator);
}

bool beginFrameRendererBE(RendererBackend&, f64) {
    return true;
}

bool endFrameRendererBE(RendererBackend&, f64) {
    return true;
}

} // namespace stlv
