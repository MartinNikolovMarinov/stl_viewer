#include <app_logger.h>
#include <vulkan_renderer.h>

core::expected<VulkanSwapchain, AppError> VulkanSwapchain::create(const VulkanContext& vkctx, const VulkanSwapchain* old) {
    auto& device = vkctx.device;
    auto& logicalDevice = vkctx.device.logicalDevice;
    auto& surface = vkctx.device.surface;

    u32 imageCount = surface.capabilities.imageCount;
    auto& surfaceFormat = surface.capabilities.format;
    auto& imageExtent = surface.capabilities.extent;
    u32 graphicsQueueIdx = u32(device.graphicsQueue.idx);
    u32 presentQueueIdx = u32(device.presentQueue.idx);
    auto& currentTransform = surface.capabilities.currentTransform;
    auto& presentMode = surface.capabilities.presentMode;

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface.handle;
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

    swapchainCreateInfo.oldSwapchain = old ? old->handle : VK_NULL_HANDLE;

    VulkanSwapchain swapchain = {};
    if (
        VkResult vres = vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &swapchain.handle);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_SWAPCHAIN));
    }

    // Create Swapchain Images:
    u32 finalImageCount = 0;
    {
        if (
            VkResult vres = vkGetSwapchainImagesKHR(logicalDevice, swapchain.handle, &finalImageCount, nullptr);
            vres != VK_SUCCESS
        ) {
            return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_SWAPCHAIN_IMAGES));
        }

        swapchain.images = core::ArrList<VkImage>(finalImageCount, VkImage{});

        if (
            VkResult vres = vkGetSwapchainImagesKHR(logicalDevice, swapchain.handle, &finalImageCount, swapchain.images.data());
            vres != VK_SUCCESS
        ) {
            return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_SWAPCHAIN_IMAGES));
        }
    }

    // Crete Swapchain ImageViews:
    {
        swapchain.imageViews.replaceWith(VkImageView{}, finalImageCount);

        for (size_t i = 0; i < swapchain.images.len(); i++) {
            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = swapchain.images[i];
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
                VkResult vres = vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &swapchain.imageViews[i]);
                vres != VK_SUCCESS
            ) {
                return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_SWAPCHAIN_IMAGE_VIEW));
            }
        }
    }

    logInfoTagged(RENDERER_TAG, "Swapchain created");

    return swapchain;
}

void VulkanSwapchain::destroy(VulkanSwapchain& swapchain, const VulkanDevice& device) {
    defer { swapchain = {}; };

    if (device.logicalDevice == VK_NULL_HANDLE) {
        // This is probably a bug.
        logErrTagged(RENDERER_TAG, "Trying to destroy a swapchain with logical device equal to null.");
        return;
    }

    if (!swapchain.imageViews.empty()) {
        logInfoTagged(RENDERER_TAG, "Destroying swapchain image views");
        for (addr_size i = 0; i < swapchain.imageViews.len(); i++) {
            vkDestroyImageView(device.logicalDevice, swapchain.imageViews[i], nullptr);
        }
    }

    if (swapchain.handle != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying swapchain");
        vkDestroySwapchainKHR(device.logicalDevice, swapchain.handle, nullptr);
    }
}
