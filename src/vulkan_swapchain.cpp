#include <vulkan_swapchain.h>

#include <vulkan_device_picker.h>

core::expected<Swapchain, AppError> Swapchain::create(const PickedGPUDevice& pickedDevice,
                                                      VkDevice logicalDevice,
                                                      VkSurfaceKHR surface) {
    u32 imageCount = pickedDevice.imageCount;
    auto& surfaceFormat = pickedDevice.surfaceFormat;
    auto& imageExtent = pickedDevice.extent;
    u32 graphicsQueueIdx = pickedDevice.graphicsQueueIdx;
    u32 presentQueueIdx = pickedDevice.presentQueueIdx;
    auto& currentTransform = pickedDevice.currentTransform;
    auto& presentMode = pickedDevice.presentMode;

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;

    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = imageExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (graphicsQueueIdx != presentQueueIdx) {
        u32 queueFamilyIndices[] = { graphicsQueueIdx, presentQueueIdx };
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainCreateInfo.preTransform = currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // TODO: Handle window resize by setting oldSwapchain.

    Swapchain ret = {};
    if (
        VkResult vres = vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &ret.swapchain);
        vres != VK_SUCCESS
    ) {
        ret.swapchain = VK_NULL_HANDLE;
        return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_SWAPCHAIN));
    }

    return ret;
}
