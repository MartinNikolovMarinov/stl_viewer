#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

bool vulkanImageCreate(
    RendererBackend& backend,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties,
    bool createView,
    VkImageAspectFlags aspectFlags,
    VulkanImage& outImage
) {
    logTraceTagged(LogTag::T_RENDERER, "Creating Vulkan image.");

    outImage.width = width;
    outImage.height = height;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_EXPECT_OR_RETURN(
        vkCreateImage(backend.device.logicalDevice, &imageInfo, backend.allocator, &outImage.handle),
        "Failed to create image"
    );

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(backend.device.logicalDevice, outImage.handle, &memRequirements);

    i32 memoryType = backend.findMemoryIndex(memRequirements.memoryTypeBits, memoryProperties);
    if (memoryType == -1) {
        logErrTagged(LogTag::T_RENDERER, "Failed to find suitable memory type for image.");
        return false;
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = u32(memoryType);

    VK_EXPECT_OR_RETURN(
        vkAllocateMemory(backend.device.logicalDevice, &allocInfo, backend.allocator, &outImage.memory),
        "Failed to allocate image memory"
    );

    VK_EXPECT_OR_RETURN(
        vkBindImageMemory(backend.device.logicalDevice, outImage.handle, outImage.memory, 0),
        "Failed to bind image memory"
    );

    if (createView) {
        if (outImage.view != VK_NULL_HANDLE) {
            // This is likely a bug, but lets hard crash.
            logWarnTagged(LogTag::T_RENDERER, "Image view already exists. Destroying old view...");
            vkDestroyImageView(backend.device.logicalDevice, outImage.view, backend.allocator);
        }

        outImage.view = VK_NULL_HANDLE;
        if (!vulkanImageViewCreate(backend, format, aspectFlags, outImage)) {
            return false;
        }
    }

    logTraceTagged(LogTag::T_RENDERER, "Vulkan Image created.");
    return true;
}

bool vulkanImageViewCreate(
    RendererBackend& backend,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VulkanImage& outImage
) {
    logTraceTagged(LogTag::T_RENDERER, "Creating Vulkan image view.");

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = outImage.handle;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_EXPECT_OR_RETURN(
        vkCreateImageView(backend.device.logicalDevice, &viewInfo, backend.allocator, &outImage.view),
        "Failed to create image view"
    );

    logTraceTagged(LogTag::T_RENDERER, "Vulkan image view created successfully.");
    return true;
}

void vulkanDestroyImage(RendererBackend& backend, VulkanImage& image) {
    if (image.view != VK_NULL_HANDLE) {
        vkDestroyImageView(backend.device.logicalDevice, image.view, backend.allocator);
        image.view = VK_NULL_HANDLE;
    }

    if (image.handle != VK_NULL_HANDLE) {
        vkDestroyImage(backend.device.logicalDevice, image.handle, backend.allocator);
        image.handle = VK_NULL_HANDLE;
    }

    if (image.memory != VK_NULL_HANDLE) {
        vkFreeMemory(backend.device.logicalDevice, image.memory, backend.allocator);
        image.memory = VK_NULL_HANDLE;
    }
}

} // namespace stlv
