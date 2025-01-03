#include <app_logger.h>
#include <vulkan_device_picker.h>
#include <vulkan_swapchain.h>

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
        return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_SWAPCHAIN));
    }

    u32 finalImageCount = 0;
    if (
        VkResult vres = vkGetSwapchainImagesKHR(logicalDevice, ret.swapchain, &finalImageCount, nullptr);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_SWAPCHAIN_IMAGES));
    }

    ret.images = core::ArrList<VkImage>(finalImageCount, VkImage{});

    if (
        VkResult vres = vkGetSwapchainImagesKHR(logicalDevice, ret.swapchain, &finalImageCount, ret.images.data());
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_SWAPCHAIN_IMAGES));
    }

    ret.imageViews = core::ArrList<VkImageView>(finalImageCount, VkImageView{});

    for (size_t i = 0; i < ret.images.len(); i++) {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = ret.images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = surfaceFormat.format;

        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if (
            VkResult vres = vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &ret.imageViews[i]);
            vres != VK_SUCCESS
        ) {
            return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_SWAPCHAIN_IMAGE_VIEW));
        }
    }

    ret.extent = imageExtent;
    ret.imageFormat = surfaceFormat.format;

    return ret;
}

void Swapchain::destroy(VkDevice logicalDevice, Swapchain& swapchain) {
    defer { swapchain = {}; };

    if (logicalDevice == VK_NULL_HANDLE) {
        // This is probably a bug.
        logErrTagged(RENDERER_TAG, "Trying to destroy a swapchain with logical device equal to null.");
        return;
    }

    if (VkResult vres = vkDeviceWaitIdle(logicalDevice); vres != VK_SUCCESS) {
        logWarnTagged(RENDERER_TAG, "Failed to wait idle the logical device");
        return;
    }

    if (!swapchain.imageViews.empty()) {
        logInfoTagged(RENDERER_TAG, "Destroying swapchain image views");
        for (addr_size i = 0; i < swapchain.imageViews.len(); i++) {
            vkDestroyImageView(logicalDevice, swapchain.imageViews[i], nullptr);
        }
    }

    if (swapchain.swapchain != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying swapchain");
        vkDestroySwapchainKHR(logicalDevice, swapchain.swapchain, nullptr);
    }
}
