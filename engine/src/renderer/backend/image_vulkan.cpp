#include <application/logger.h>
#include <renderer/backend/renderer_vulkan.h>

namespace stlv {

void vulkanImageCreate(RendererBackend& backend, u32 width, u32 height, VkFormat format,
                       VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags,
                       bool createView, VkImageAspectFlags viewAspectFlags,
                       VulkanImage& outImage) {
    outImage.width = width;
    outImage.height = height;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1; // TODO: Support mipmaps.
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: Support multisampling.
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // TODO: Support concurrent sharing.

    VK_CHECK(
        vkCreateImage(backend.device.logicalDevice, &imageInfo, backend.allocator, &outImage.handle),
        "Failed to create image."
    );

    // Get the memory requirements for the image.
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(backend.device.logicalDevice, outImage.handle, &memRequirements);

    // Find the memory type index that satisfies the memory requirements.
    i32 memoryTypeIdx = backend.findMemoryTypeIndex(memRequirements.memoryTypeBits, memoryFlags);
    if (memoryTypeIdx == -1) {
        logErrTagged(LogTag::T_RENDERER, "Required memory type not found. Image is not valid");
        Panic(memoryTypeIdx != -1, "Required memory type not found. Image is not valid");
    }

    // Allocate memory for the image.
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.memoryTypeIndex = u32(memoryTypeIdx);
    VK_CHECK(
        vkAllocateMemory(backend.device.logicalDevice, &allocInfo, backend.allocator, &outImage.memory),
        "Failed to allocate image memory."
    );

    // Bind the memory
    VkDeviceSize memoryOffset = 0; // TODO: Will need this for image pooling.
    VK_CHECK(
        vkBindImageMemory(backend.device.logicalDevice, outImage.handle, outImage.memory, memoryOffset),
        "Failed to bind image memory."
    );

    // Crate the image view.
    if (createView) {
        if (outImage.view != nullptr) {
            logWarnTagged(LogTag::T_RENDERER, "Image view already exists. Overwriting.");
            outImage.view = nullptr;
        }

        vulkanImageViewCreate(backend, format, outImage, viewAspectFlags);
    }
}

void vulkanImageViewCreate(RendererBackend& backend, VkFormat format,
                           VulkanImage& image, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    // TODO: Make all of these parameters configurable:
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(
        vkCreateImageView(backend.device.logicalDevice, &viewCreateInfo, backend.allocator, &image.view),
        "Failed to create image view."
    );
}

void vulkanImageDestroy(RendererBackend& backend, VulkanImage& image) {
    logInfoTagged(LogTag::T_RENDERER, "Vulkan Destroying image.");
    if (image.view != nullptr) {
        vkDestroyImageView(backend.device.logicalDevice, image.view, backend.allocator);
        image.view = nullptr;
    }
    if (image.memory != nullptr) {
        vkFreeMemory(backend.device.logicalDevice, image.memory, backend.allocator);
        image.memory = nullptr;
    }
    if (image.handle != nullptr) {
        vkDestroyImage(backend.device.logicalDevice, image.handle, backend.allocator);
        image.handle = nullptr;
    }
}

} // namespace stlv
