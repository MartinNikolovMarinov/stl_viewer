#include <fwd_internal.h>

#include <renderer/renderer_backend.h>
#include <renderer/backend/renderer_vulkan.h>

namespace stlv {

namespace {

bool selectPhysicalDevice(RendererBackend& backend) {
    u32 physicalDeviceCount = 0;
    VK_EXPECT(
        vkEnumeratePhysicalDevices(backend.instance, &physicalDeviceCount, nullptr),
        "Failed to enumerate physical devices."
    );
    if (physicalDeviceCount == 0) {
        logErrTagged(LogTag::T_RENDERER, "No physical devices on the system support Vulkan.");
        return false;
    }

    VkPhysicalDevice* physicalDevices = reinterpret_cast<VkPhysicalDevice*>(
        RendererBackendAllocator::alloc(sizeof(VkPhysicalDevice) * physicalDeviceCount));
    VK_EXPECT(
        vkEnumeratePhysicalDevices(backend.instance, &physicalDeviceCount, physicalDevices),
        "Failed to enumerate physical devices."
    );

    for (u32 i = 0; i < physicalDeviceCount; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
        logInfoTagged(LogTag::T_RENDERER, "Found physical device: %s", properties.deviceName);
    }

    return true;
}

} // namespace


bool createVulkanDevice(RendererBackend& backend) {
    // VulkanDevice& device = backend.device;
    if (!selectPhysicalDevice(backend)) {
        return false;
    }
    return true;
}

void destroyVulkanDevice(RendererBackend&) {
    // VulkanDevice& device = backend.device;

}

} // namespace stlv
