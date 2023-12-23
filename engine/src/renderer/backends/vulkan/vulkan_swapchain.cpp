#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

namespace {

constexpr u32 MAX_FRAMES_IN_FLIGHT = 2; // triple buffering

void create(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& outSwapchain);
void destroy(RendererBackend& backend, VulkanSwapchain& swapchain);

} // namespace

void vulkanSwapchainCreate(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& outSwapchain) {
    create(backend, width, height, outSwapchain);
}

void vulkanSwapchainRecreate(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& outSwapchain) {
    destroy(backend, outSwapchain);
    create(backend, width, height, outSwapchain);
}

void vulkanSwapchainDestroy(RendererBackend& backend, VulkanSwapchain& swapchain) {
    destroy(backend, swapchain);
}

bool vulkanSwapchainAcquireNextImage(RendererBackend& backend,
                                     VulkanSwapchain& swapchain,
                                     u64 timeoutNs,
                                     VkSemaphore imageAvailableSemaphore,
                                     VkFence fence,
                                     u32& outImageIdx) {

    VkResult result = vkAcquireNextImageKHR(backend.device.logicalDevice,
                                            swapchain.handle,
                                            timeoutNs,
                                            imageAvailableSemaphore,
                                            fence,
                                            &outImageIdx);
    if (result == VK_SUCCESS) return true;

    if (result == VK_SUBOPTIMAL_KHR) {
        // TODO: Might want to recreate swapchain here.
        return true;
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        logTraceTagged(LogTag::T_RENDERER, "Swapchain is out of date, recreating...");
        vulkanSwapchainRecreate(backend, backend.frameBufferWidth, backend.frameBufferHeight, swapchain);
        return false;
    }

    // Somthing more critical has happend here. Maybe a device lost, or somthing like that.
    logErrTagged(LogTag::T_RENDERER,
                "Failed to acquire next image from swapchain, reason: %s",
                vulkanResultToCptr(result));
    return false;
}

void vulkanSwapchainPresent(RendererBackend& backend,
                            VulkanSwapchain& swapchain,
                            VulkanQueue&,
                            VulkanQueue& presentQueue,
                            VkSemaphore renderCompleteSemaphore,
                            u32 presentImageIdx) {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.handle;
    presentInfo.pImageIndices = &presentImageIdx;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(presentQueue.handle, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        logTraceTagged(LogTag::T_RENDERER, "Swapchain is out of date, recreating...");
        vulkanSwapchainRecreate(backend, backend.frameBufferWidth, backend.frameBufferHeight, swapchain);
    }
    else if (result != VK_SUCCESS) {
        logFatalTagged(LogTag::T_RENDERER,
                      "Failed to present swapchain image, reason: %s",
                       vulkanResultToCptr(result));
        Panic(false, "Failed to present swapchain image");
    }

    // Updateing the current frame index.
    backend.currentFrame = (backend.currentFrame + 1) % backend.swapchain.maxFramesInFlight; // TODO: Use a function for this ?
}

namespace {

void create(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& outSwapchain) {
    logTraceTagged(LogTag::T_RENDERER, "Creating Vulkan swapchain...");

    outSwapchain.maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;

    // Chose a preffered format for the swapchain images:
    bool found = false;
    for (u32 i = 0; i < backend.device.swapchainSupportInfo.formats.len(); ++i) {
        VkSurfaceFormatKHR& f = backend.device.swapchainSupportInfo.formats[i];
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            outSwapchain.imageFormat = f;
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
        outSwapchain.imageFormat = backend.device.swapchainSupportInfo.formats[0];
    }

    // Choose a preffered preset mode for the swapchain:
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

    // Re-query for swapchain support. This is necessary because the swapchain support info might have changed afeter a
    // display change, or a resize, or somthing like that:
    vulkanDeviceQuerySwapchainSupport(backend.device.physicalDevice,
                                      backend.surface,
                                      backend.device.swapchainSupportInfo);

    // Check swapchain extent:
    VkExtent2D swapchainExtent = { width, height };
    if (backend.device.swapchainSupportInfo.capabilities.currentExtent.width != MAX_U32) {
        swapchainExtent = backend.device.swapchainSupportInfo.capabilities.currentExtent;
    }

    // Clamp extent to values allowed by the GPU:
    VkExtent2D min = backend.device.swapchainSupportInfo.capabilities.minImageExtent;
    VkExtent2D max = backend.device.swapchainSupportInfo.capabilities.maxImageExtent;
    swapchainExtent.width = core::clamp(swapchainExtent.width, min.width, max.width);
    swapchainExtent.height = core::clamp(swapchainExtent.height, min.height, max.height);

    u32 imageCount = backend.device.swapchainSupportInfo.capabilities.minImageCount + 1;
    if (backend.device.swapchainSupportInfo.capabilities.maxImageCount > 0 &&
        imageCount > backend.device.swapchainSupportInfo.capabilities.maxImageCount) {
        imageCount = backend.device.swapchainSupportInfo.capabilities.maxImageCount;
    }

    // Swapchain create info.

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = backend.surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = outSwapchain.imageFormat.format;
    swapchainCreateInfo.imageColorSpace = outSwapchain.imageFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT specifies that the image can be used to create a VkImageView suitable for use
    // as a color or resolve attachment in a VkFramebuffer.
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (backend.device.graphicsQueue.familyIdx != backend.device.presentQueue.familyIdx) {
        u32 queueFamilyIndices[] = {
            backend.device.graphicsQueue.familyIdx,
            backend.device.presentQueue.familyIdx
        };
        constexpr addr_size queueFamilyIndicesLen = sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]);
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndicesLen;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainCreateInfo.preTransform = backend.device.swapchainSupportInfo.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // TODO: Use old swapchain here!

    // Create the swapchain:
    VK_EXPECT(
        vkCreateSwapchainKHR(backend.device.logicalDevice,
                             &swapchainCreateInfo,
                             backend.allocator,
                             &outSwapchain.handle),
        "Failed to create swapchain"
    )

    backend.currentFrame = 0; // TODO: use a function for this ?
    outSwapchain.imageCount = 0;

    // Get the image count:
    VK_EXPECT(
        vkGetSwapchainImagesKHR(backend.device.logicalDevice,
                                outSwapchain.handle,
                                &outSwapchain.imageCount,
                                nullptr),
        "Failed to get swapchain images"
    )

    // Allocate memory for the swapchain images and image views if none was allocated yet:
    if (!outSwapchain.images) {
        outSwapchain.images = reinterpret_cast<VkImage*>(
            RendererBackendAllocator::alloc(sizeof(VkImage) * outSwapchain.imageCount));
    }
    if (!outSwapchain.imageViews) {
        outSwapchain.imageViews = reinterpret_cast<VkImageView*>(
            RendererBackendAllocator::alloc(sizeof(VkImageView) * outSwapchain.imageCount));
    }

    // Get the swapchain images:
    VK_EXPECT(
        vkGetSwapchainImagesKHR(backend.device.logicalDevice,
                                outSwapchain.handle,
                                &outSwapchain.imageCount,
                                outSwapchain.images),
        "Failed to get swapchain images"
    )

    // Create image views for the swapchain images:
    for (u32 i = 0; i < outSwapchain.imageCount; ++i) {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = outSwapchain.images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = outSwapchain.imageFormat.format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        // imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        // imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        // imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        // imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VK_EXPECT(
            vkCreateImageView(backend.device.logicalDevice,
                              &imageViewCreateInfo,
                              backend.allocator,
                              &outSwapchain.imageViews[i]),
            "Failed to create swapchain image view"
        )
    }

    // Create depth buffer image attachment:

    if (!vulkanDeviceDetectDepthFormat(backend.device)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to detect depth buffer format. Using VK_FORMAT_UNDEFINED.");
        backend.device.depthFormat = VK_FORMAT_UNDEFINED;
    }

    logTraceTagged(LogTag::T_RENDERER, "Creating depth buffer image...");
    vulkanImageCreate(backend,
                      swapchainExtent.width,
                      swapchainExtent.height,
                      backend.device.depthFormat,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      VK_IMAGE_ASPECT_DEPTH_BIT,
                      outSwapchain.depthAttachment);

    logTraceTagged(LogTag::T_RENDERER, "Vulkan swapchain created.");
}

void destroy(RendererBackend& backend, VulkanSwapchain& swapchain) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan swapchain.");

    vkDeviceWaitIdle(backend.device.logicalDevice);

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

} // namespace stlv
