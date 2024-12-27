#pragma once

#include <basic.h>
#include <vulkan_include.h>

struct GPUDevice {
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;
};

/**
 * @brief Gives a score for the physical device based on the supported feature set. Higher is better. A score of 0 means
 *        that the device does not meet the application's minimal requirements.
 *
 * @param gpu The device to score.
 *
 * @return The score.
*/
u32 getDeviceSutabilityScore(const GPUDevice& gpu);

const GPUDevice* pickDevice(core::Memory<const GPUDevice> gpus);
