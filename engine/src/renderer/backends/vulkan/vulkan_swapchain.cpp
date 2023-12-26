#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

namespace {

bool create(RendererBackend& backend, VulkanSwapchain& swapchain, const VulkanSwapchainCreationInfo& createInfo);
bool setSwapchainImages(RendererBackend& backend, VulkanSwapchain& swapchain);
void destroy(RendererBackend& backend, VulkanSwapchain& swapchain);

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(VkSurfaceFormatKHRList& availableFormats);
VkPresentModeKHR chooseSwapchainPresentMode(VkPresentModeKHRList& availablePresentModes);

} // namespace

bool vulkanSwapchainCreate(RendererBackend& backend, VulkanSwapchain& swapchain, const VulkanSwapchainCreationInfo& createInfo) {
    if (!create(backend, swapchain, createInfo)) return false;
    if (!setSwapchainImages(backend, swapchain)) return false;
    return true;
}

void vulkanSwapchainDestroy(RendererBackend& backend, VulkanSwapchain& swapchain) {
    destroy(backend, swapchain);
}

bool vulkanSwapchainRecreate(RendererBackend& backend, VulkanSwapchain& swapchain, const VulkanSwapchainCreationInfo& createInfo) {
    destroy(backend, swapchain);
    if (!create(backend, swapchain, createInfo)) return false;
    if (!setSwapchainImages(backend, swapchain)) return false;
    return true;
}

bool vulkanSwapchainAcquireNextImage(
    RendererBackend& backend,
    VulkanSwapchain& swapchain,
    u64 timeoutNs,
    VkSemaphore imageAvailableSemaphore,
    VkFence fence,
    u32& outImageIdx
) {
    VkResult result = vkAcquireNextImageKHR(
        backend.device.logicalDevice,
        swapchain.handle,
        timeoutNs,
        imageAvailableSemaphore,
        fence,
        &outImageIdx);

    if (result != VK_SUCCESS) {
        return true;
    }
    if (result != VK_SUBOPTIMAL_KHR) {
        // TODO: Maybe this should be a re-create as well ?
        return true;
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        VulkanSwapchainCreationInfo createInfo;
        createInfo.width = backend.frameBufferWidth;
        createInfo.height = backend.frameBufferHeight;
        createInfo.maxFramesInFlight = swapchain.maxFramesInFlight;
        if (!vulkanSwapchainRecreate(backend, swapchain, createInfo)) {
            logErrTagged(LogTag::T_RENDERER, "Failed to recreate swapchain");
        }
        return false;
    }

    // Might be that a device was lost, or somthing worse.
    logErrTagged(LogTag::T_RENDERER, "Failed to acquire next image from swapchain");
    return false;
}

bool vulkanSwapchainPresent(
    RendererBackend& backend,
    VulkanSwapchain& swapchain,
    VkQueue,
    VkQueue presentQueue,
    VkSemaphore renderCompleteSemaphore,
    u32 presentImageIdx
) {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.handle;
    presentInfo.pImageIndices = &presentImageIdx;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    bool ret = false;

    if (result == VK_SUCCESS) {
        ret = true;
    }
    else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        VulkanSwapchainCreationInfo createInfo;
        createInfo.width = backend.frameBufferWidth;
        createInfo.height = backend.frameBufferHeight;
        createInfo.maxFramesInFlight = swapchain.maxFramesInFlight;
        if (!vulkanSwapchainRecreate(backend, swapchain, createInfo)) {
            logErrTagged(LogTag::T_RENDERER, "Failed to recreate swapchain");
            ret = false;
        }
        else {
            ret = true;
        }
    }

    backend.currentFrame = (backend.currentFrame + 1) % swapchain.maxFramesInFlight;
    return ret;
}

namespace {

bool create(RendererBackend& backend, VulkanSwapchain& swapchain, const VulkanSwapchainCreationInfo& createInfo) {
    logTraceTagged(LogTag::T_RENDERER, "Creating swapchain.");

    auto& swapchainSupportInfo = backend.device.swapchainSupportInfo;

    VkSurfaceFormatKHR imageFormat = chooseSwapchainSurfaceFormat(swapchainSupportInfo.formats);
    VkPresentModeKHR presentMode = chooseSwapchainPresentMode(swapchainSupportInfo.presentModes);
    VkExtent2D extent = { createInfo.width, createInfo.height };

    if (swapchainSupportInfo.capabilities.currentExtent.width != core::MAX_U32) {
        // Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent.
        extent = swapchainSupportInfo.capabilities.currentExtent;
    }

    // Clamp the extent to the min and max supported values.
    VkExtent2D min = swapchainSupportInfo.capabilities.minImageExtent;
    VkExtent2D max = swapchainSupportInfo.capabilities.maxImageExtent;
    extent.width = core::clamp(extent.width, min.width, max.width);
    extent.height = core::clamp(extent.height, min.height, max.height);

    u32 imageCount = swapchainSupportInfo.capabilities.minImageCount + 1;
    if (swapchainSupportInfo.capabilities.maxImageCount > 0 &&
        imageCount > swapchainSupportInfo.capabilities.maxImageCount) {
        // Don't exceed the max image count.
        imageCount = swapchainSupportInfo.capabilities.maxImageCount;
    }

    // Set up the swapchain.
    swapchain.imageFormat = imageFormat;
    swapchain.maxFramesInFlight = createInfo.maxFramesInFlight;
    swapchain.imageCount = imageCount;

    VkSwapchainCreateInfoKHR createInfoKHR = {};
    createInfoKHR.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfoKHR.pNext = nullptr;
    createInfoKHR.surface = backend.surface;
    createInfoKHR.minImageCount = imageCount;
    createInfoKHR.imageFormat = imageFormat.format;
    createInfoKHR.imageColorSpace = imageFormat.colorSpace;
    createInfoKHR.imageExtent = extent;
    createInfoKHR.imageArrayLayers = 1;
    createInfoKHR.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (backend.device.graphicsQueue.familyIndex != backend.device.presentQueue.familyIndex) {
        u32 queueFamilyIndices[] = {
            backend.device.graphicsQueue.familyIndex,
            backend.device.presentQueue.familyIndex,
        };
        createInfoKHR.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfoKHR.queueFamilyIndexCount = 2;
        createInfoKHR.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfoKHR.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfoKHR.queueFamilyIndexCount = 0;
        createInfoKHR.pQueueFamilyIndices = nullptr;
    }

    createInfoKHR.preTransform = swapchainSupportInfo.capabilities.currentTransform;
    createInfoKHR.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfoKHR.presentMode = presentMode;
    createInfoKHR.clipped = VK_TRUE;
    createInfoKHR.oldSwapchain = VK_NULL_HANDLE;

    VK_EXPECT_OR_RETURN(
        vkCreateSwapchainKHR(backend.device.logicalDevice, &createInfoKHR, backend.allocator, &swapchain.handle),
        "Failed to create swapchain"
    );

    logTraceTagged(LogTag::T_RENDERER, "Swapchain created.");
    return true;
}

bool setSwapchainImages(RendererBackend& backend, VulkanSwapchain& swapchain) {
    logTraceTagged(LogTag::T_RENDERER, "Retreaving swapchain images.");

    auto& device = backend.device;

    // NOTE: The imageCount can change here, because the swapchain can choose to use more than the minimum number of
    // images, depending on the presentation mode that was chosen.
    VK_EXPECT_OR_RETURN(
        vkGetSwapchainImagesKHR(device.logicalDevice, swapchain.handle, &swapchain.imageCount,  nullptr),
        "Failed to get the number of images from the swapchain"
    );

    swapchain.images.clear();
    swapchain.imageViews.clear();
    swapchain.images.fill(VK_NULL_HANDLE, 0, swapchain.imageCount);
    swapchain.imageViews.fill(VK_NULL_HANDLE, 0, swapchain.imageCount);

    VK_EXPECT_OR_RETURN(
        vkGetSwapchainImagesKHR(device.logicalDevice, swapchain.handle, &swapchain.imageCount, swapchain.images.data()),
        "Failed to get images from the swapchain"
    );

    logTraceTagged(LogTag::T_RENDERER, "Creating swapchain image views.");

    for (u32 i = 0; i < swapchain.imageCount; i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.image = swapchain.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchain.imageFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_EXPECT_OR_RETURN(
            vkCreateImageView(device.logicalDevice, &createInfo, backend.allocator, &swapchain.imageViews[i]),
            "Failed to create image view"
        );
    }

    logTraceTagged(LogTag::T_RENDERER, "Create depth buffer in swapchian.");
    bool ok = vulkanImageCreate(
        backend,
        backend.frameBufferWidth,
        backend.frameBufferHeight,
        backend.device.depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        swapchain.depthImage
    );
    if (!ok) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create depth buffer in swapchain");
        return false;
    }


    logTraceTagged(LogTag::T_RENDERER, "Swapchain images retreaved.");
    return true;
}

VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(VkSurfaceFormatKHRList& availableFormats) {
    for (addr_size i = 0; i < availableFormats.len(); i++) {
        VkSurfaceFormatKHR& surfaceFormat = availableFormats[i];
        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return surfaceFormat;
        }
    }

    VkSurfaceFormatKHR ret = availableFormats[0];
    logWarnTagged(LogTag::T_RENDERER, "Failed to find preferred surface format, using %d", ret);
    return ret;
}

VkPresentModeKHR chooseSwapchainPresentMode(VkPresentModeKHRList& availablePresentModes) {
    for (addr_size i = 0; i < availablePresentModes.len(); i++) {
        VkPresentModeKHR& presentMode = availablePresentModes[i];
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    logWarnTagged(LogTag::T_RENDERER, "Failed to find preferred present mode, using FIFO");
    return VK_PRESENT_MODE_FIFO_KHR;
}

void destroy(RendererBackend& backend, VulkanSwapchain& swapchain) {
    if (!swapchain.imageViews.empty()) {
        for (u32 i = 0; i < swapchain.imageViews.len(); i++) {
            vkDestroyImageView(backend.device.logicalDevice, swapchain.imageViews[i], backend.allocator);
        }
        swapchain.imageViews.clear();
    }

    if (swapchain.handle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(backend.device.logicalDevice, swapchain.handle, backend.allocator);
        swapchain.handle = VK_NULL_HANDLE;
    }

    if (swapchain.depthImage.handle != VK_NULL_HANDLE) {
        vulkanDestroyImage(backend, swapchain.depthImage);
    }
}

} // namespace

} // namespace stlv
