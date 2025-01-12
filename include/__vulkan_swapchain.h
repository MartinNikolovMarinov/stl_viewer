#pragma once

#include <app_error.h>
#include <basic.h>
#include <vulkan_include.h>

struct PickedGPUDevice;

struct Swapchain {
    struct CreateInfo {
        u32 imageCount;
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D extent;
        u32 graphicsQueueIdx;
        u32 presentQueueIdx;
        VkSurfaceTransformFlagBitsKHR currentTransform;
        VkPresentModeKHR presentMode;
        VkDevice logicalDevice;
        VkSurfaceKHR surface;
    };

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    core::ArrList<VkImage> images;
    core::ArrList<VkImageView> imageViews;
    VkFormat imageFormat;
    VkExtent2D extent;

    static core::expected<Swapchain, AppError> create(const CreateInfo& info);
    static void destroy(VkDevice logicalDevice, Swapchain& swapchain);
};
