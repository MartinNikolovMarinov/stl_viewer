#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

void vulkanImageCreate(RendererBackend& backend,
                       u32 width, u32 height,
                       VkFormat format,
                       VkImageTiling tiling,
                       VkImageUsageFlags usage,
                       VkMemoryPropertyFlags memoryFlags,
                       bool createView,
                       VkImageAspectFlags viewAspectFlags,
                       VulkanImage& outImage) {
    outImage.width = width;
    outImage.height = height;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1; // TODO: Support mipmaps.
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: Support multisampling.
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // TODO: Support concurrent sharing.

    VK_EXPECT(
        vkCreateImage(backend.device.logicalDevice,
                      &imageCreateInfo,
                      backend.allocator,
                      &outImage.handle),
        "Failed to create image."
    );

    // Get the memory requirements for the image.
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(backend.device.logicalDevice,
                                 outImage.handle,
                                 &memRequirements);

    // Find the memory type index that satisfies the memory requirements.
    i32 memoryTypeIdx = backend.findMemoryTypeIndex(memRequirements.memoryTypeBits,
                                                    memoryFlags);
    if (memoryTypeIdx == -1) {
        logErrTagged(LogTag::T_RENDERER, "Required memory type not found. Image is not valid");
        Panic(memoryTypeIdx != -1, "Required memory type not found. Image is not valid");
    }

    // Allocate memory for the image.
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = u32(memoryTypeIdx);
    VK_EXPECT(
        vkAllocateMemory(backend.device.logicalDevice,
                         &allocInfo,
                         backend.allocator,
                         &outImage.memory),
        "Failed to allocate image memory."
    );

    // Bind the memory
    VkDeviceSize memoryOffset = 0; // TODO: Will need this for image pooling.
    VK_EXPECT(
        vkBindImageMemory(backend.device.logicalDevice,
                          outImage.handle,
                          outImage.memory,
                          memoryOffset),
        "Failed to bind image memory."
    );

    // Crate the image view.
    if (createView) {
        if (outImage.view != nullptr) {
            logWarnTagged(LogTag::T_RENDERER, "Image view already exists. Overwriting.");
            outImage.view = nullptr;
        }

        vulkanImageViewCreate(backend, format, viewAspectFlags, outImage);
    }
}

void vulkanImageViewCreate(RendererBackend& backend,
                           VkFormat format,
                           VkImageAspectFlags aspectFlags,
                           VulkanImage& outImage) {
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = outImage.handle;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    // TODO: Make all of these parameters configurable:
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    VK_EXPECT(
        vkCreateImageView(backend.device.logicalDevice,
                          &viewCreateInfo,
                          backend.allocator,
                          &outImage.view),
        "Failed to create image view."
    );
}

void vulkanImageDestroy(RendererBackend& backend, VulkanImage& image) {
    logInfoTagged(LogTag::T_RENDERER, "Vulkan Destroying image.");
    if (image.view) {
        vkDestroyImageView(backend.device.logicalDevice, image.view, backend.allocator);
        image.view = nullptr;
    }
    if (image.handle) {
        vkDestroyImage(backend.device.logicalDevice, image.handle, backend.allocator);
        image.handle = nullptr;
    }
    if (image.memory) {
        vkFreeMemory(backend.device.logicalDevice, image.memory, backend.allocator);
        image.memory = nullptr;
    }
}

} // namespace stlv
