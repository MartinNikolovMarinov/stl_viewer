#pragma once

#include <app_error.h>
#include <basic.h>
#include <vulkan_include.h>

struct PickedGPUDevice;

struct Swapchain {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    static core::expected<Swapchain, AppError> create(const PickedGPUDevice& pickedDevice,
                                                      VkDevice logicalDevice,
                                                      VkSurfaceKHR surface);
};
