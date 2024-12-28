#pragma once

#include <app_error.h>
#include <basic.h>
#include <vulkan_include.h>

struct GPUDevice {
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;
};

struct PickDeviceInfo {
    VkSurfaceKHR surface;
    core::Memory<const char*> requiredExtensions;
};

struct PickedGPUDevice {
    const GPUDevice* gpu;
    i32 graphicsQueueIdx;
    i32 presentQueueIdx;
};

[[nodiscard]]
core::expected<PickedGPUDevice, AppError> pickDevice(core::Memory<const GPUDevice> gpus, const PickDeviceInfo& info);
