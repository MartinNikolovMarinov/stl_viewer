#include <application/logger.h>
#include <renderer/backend/renderer_vulkan.h>

namespace stlv {

namespace {

void createSwapchain(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& swapchain) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan swapchain...");

    swapchain.maxFramesInFlight = 2; // Setting up tripple buffering.

    // Chose a preffered format for the swapchain.

    bool found = false;
    for (u32 i = 0; i < backend.device.swapchainSupportInfo.formats.len(); ++i) {
        VkSurfaceFormatKHR& f = backend.device.swapchainSupportInfo.formats[i];
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain.imageFormat = f;
            found = true;
            break;
        }
    }

    if (!found) {
        // Preferred format was not found.
        // Use the first available format and hope for the best.
        // TODO: The selected format should be normalized, or some behaviour might be undefined. I should, probably,
        //       just panic here.
        logWarnTagged(LogTag::T_RENDERER, "Could not find preffered swapchain format, using first available.");
        swapchain.imageFormat = backend.device.swapchainSupportInfo.formats[0];
    }

    // Choose a preffered preset mode for the swapchain.

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed to exist.
    for (u32 i = 0; i < backend.device.swapchainSupportInfo.presentModes.len(); ++i) {
        VkPresentModeKHR& mode = backend.device.swapchainSupportInfo.presentModes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }
    }

    if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
        logWarnTagged(LogTag::T_RENDERER, "Could not find preffered swapchain present mode, using FIFO.");
    }

    // Re-query for swapchain support.

    bool ok = vulkanDeviceQuerySwapchainSupport(backend.device.physicalDevice,
                                    backend.surface,
                                    backend.device.swapchainSupportInfo);
    if (!ok) {
        // This is not good, but don't panic the program. Lets assume that the same supported info is still valid.
        logErrTagged(LogTag::T_RENDERER, "Failed to re-query swapchain support during swapchain creation.");
    }

    // Prepare the swapchain creation parameters.

    VkExtent2D swapchainExtent = { width, height };
    VkExtent2D& currentExtent = backend.device.swapchainSupportInfo.capabilities.currentExtent;
    if (currentExtent.width != core::MAX_U32) {
        logInfoTagged(LogTag::T_RENDERER, "Swapchain extent is not set to max u32, using current extent.");
        swapchainExtent = currentExtent;
    }

    VkExtent2D& minExtent = backend.device.swapchainSupportInfo.capabilities.minImageExtent;
    VkExtent2D& maxExtent = backend.device.swapchainSupportInfo.capabilities.maxImageExtent;
    swapchainExtent.width = core::clamp(swapchainExtent.width, minExtent.width, maxExtent.width);
    swapchainExtent.height = core::clamp(swapchainExtent.height, minExtent.height, maxExtent.height);

    u32 imageCount = core::clamp(backend.device.swapchainSupportInfo.capabilities.minImageCount + 1,
                                 backend.device.swapchainSupportInfo.capabilities.minImageCount,
                                 backend.device.swapchainSupportInfo.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = backend.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapchain.imageFormat.format;
    createInfo.imageColorSpace = swapchain.imageFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT specifies that the image can be used to create a VkImageView suitable for use
    // as a color or resolve attachment in a VkFramebuffer.
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (backend.device.graphicsQueueFamilyIdx != backend.device.presentQueueFamilyIdx) {
        u32 queueFamilyIndices[] = { backend.device.graphicsQueueFamilyIdx, backend.device.presentQueueFamilyIdx };
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]);
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = backend.device.swapchainSupportInfo.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE; // TODO: Try using the old swapchain.

    // Create the swapchain:
    VK_EXPECT(
        vkCreateSwapchainKHR(backend.device.logicalDevice, &createInfo, backend.allocator, &swapchain.handle),
        "Failed to create swapchain"
    )

    // Set the current frame index to 0.
    backend.currentFrame = 0;

    // Get the number of images in the swapchain:
    swapchain.imageCount = 0;
    VK_EXPECT(
        vkGetSwapchainImagesKHR(backend.device.logicalDevice, swapchain.handle, &swapchain.imageCount, nullptr),
        "Failed to get swapchain image count"
    )

    // Allocate memory for the swapchain images and image views if none was allocated yet.
    if (!swapchain.images) {
        swapchain.images = reinterpret_cast<VkImage*>(
            RendererBackendAllocator::alloc(sizeof(VkImage) * swapchain.imageCount));
    }
    if (!swapchain.imageViews) {
        swapchain.imageViews = reinterpret_cast<VkImageView*>(
            RendererBackendAllocator::alloc(sizeof(VkImageView) * swapchain.imageCount));
    }

    // Get the swapchain images.
    VK_EXPECT(
        vkGetSwapchainImagesKHR(backend.device.logicalDevice, swapchain.handle, &swapchain.imageCount, swapchain.images),
        "Failed to get swapchain images"
    )

    // Create the image views.
    for (u32 i = 0; i < swapchain.imageCount; i++) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = swapchain.images[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = swapchain.imageFormat.format;

        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        VK_EXPECT(
            vkCreateImageView(backend.device.logicalDevice, &viewCreateInfo, backend.allocator, &swapchain.imageViews[i]),
            "Failed to create swapchain image view"
        )
    }

    // Create the depth buffer image.
    if (!vulkanDeviceDetectDepthFormat(backend.device)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to detect depth buffer format. Using VK_FORMAT_UNDEFINED.");
        backend.device.depthFormat = VK_FORMAT_UNDEFINED;
    }

    logInfoTagged(LogTag::T_RENDERER, "Creating depth buffer image...");
    vulkanImageCreate(backend,
                      swapchainExtent.width,
                      swapchainExtent.height,
                      backend.device.depthFormat,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      VK_IMAGE_ASPECT_DEPTH_BIT,
                      swapchain.depthAttachment);

    logInfoTagged(LogTag::T_RENDERER, "Swapchain created.");
}

void destroySwapchain(RendererBackend& backend, VulkanSwapchain& swapchain) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan swapchain.");

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan depth attachment.");
    vulkanImageDestroy(backend, swapchain.depthAttachment);

    // Only destroy the views, not the images, since they are owned by the swapchain and are destroyed automatically
    // when the swapchain is destroyed.
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan swapchain image views.");
    for (u32 i = 0; i < swapchain.imageCount; ++i) {
        vkDestroyImageView(backend.device.logicalDevice, swapchain.imageViews[i], backend.allocator);
    }

    if (swapchain.handle) {
        vkDestroySwapchainKHR(backend.device.logicalDevice, swapchain.handle, backend.allocator);
    }
}

} // namespace

void vulkanSwapchainCreate(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& swapchain) {
    createSwapchain(backend, width, height, swapchain);
}

void vulkanSwapchainRecreate(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& swapchain) {
    destroySwapchain(backend, swapchain);
    createSwapchain(backend, width, height, swapchain);
}

void vulkanSwapchainDestroy(RendererBackend& backend, VulkanSwapchain& swapchain) {
    destroySwapchain(backend, swapchain);
}

bool vulkanSwapchainAcquireNextImageIdx(RendererBackend& backend, VulkanSwapchain& swapchain, u64 timeoutNs,
                                  VkSemaphore imageAvailableSemaphore, VkFence fence, u32& imageIdx) {
    VkResult res = vkAcquireNextImageKHR(backend.device.logicalDevice, swapchain.handle,
                                         timeoutNs, imageAvailableSemaphore, fence, &imageIdx);
    if (res == VK_SUCCESS) return true;

    if (res == VK_SUBOPTIMAL_KHR) {
        // TODO: Might want to recreate swapchain here.
        return true;
    }

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        logInfoTagged(LogTag::T_RENDERER, "Swapchain is out of date, recreating...");
        vulkanSwapchainRecreate(backend, backend.framebufferWidth, backend.framebufferHeight, swapchain);
        return false;
    }

    logErrTagged(LogTag::T_RENDERER, "Failed to acquire next image from swapchain, result error code: %d", i32(res));
    return false;
}

void vulkanSwapchainPresent(RendererBackend& backend, VulkanSwapchain& swapchain,
                      VkQueue, VkQueue presentQueue,
                      VkSemaphore renderCompleteSemaphore, u32 imageIdx) {

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.handle;
    presentInfo.pImageIndices = &imageIdx;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        logWarnTagged(LogTag::T_RENDERER, "Swapchain is out of date or suboptimal, recreating...");
        vulkanSwapchainRecreate(backend, backend.framebufferWidth, backend.framebufferHeight, swapchain);
    }
    else if (result != VK_SUCCESS) {
        logErrTagged(LogTag::T_RENDERER, "Failed to present swapchain image, result error code: %d", i32(result));
        Assert(false, "Failed to present swapchain image");
    }
}

} // namespace stlv
