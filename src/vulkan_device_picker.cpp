#include <vulkan_device_picker.h>

namespace {

i32 findGraphicsQueueFamilyIndex(VkPhysicalDevice physDevice) {
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

    core::ArrList<VkQueueFamilyProperties> queueFamilies(queueFamilyCount, VkQueueFamilyProperties{});
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

    for (addr_size i = 0; i < queueFamilies.len(); i++) {
        const VkQueueFlags flags = queueFamilies[i].queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT) {
            return i32(i);  // Found a graphics queue
        }
    }

    return -1;
}

/**
 * @brief Gives a score for the physical device based on the supported feature set. Higher is better. A score of 0 means
 *        that the device does not meet the application's minimal requirements.
 *
 * @param gpu The device to score.
 *
 * @return The score.
*/
u32 getDeviceSutabilityScore(const GPUDevice& gpu, i32& outGraphicsQueueIdx) {
    const auto& props = gpu.props;
    const auto& features = gpu.features;
    u32 score = 0;

    i32 graphicsQueueIdx = findGraphicsQueueFamilyIndex(gpu.device);
    if (graphicsQueueIdx < 0) {
        return 0;
    }
    score++; // Has Graphics Queue
    outGraphicsQueueIdx = graphicsQueueIdx;

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // Discrete GPUs are preffered.
        score += 100;
    }

    return score;
}

} // namespace

core::expected<PickedGPUDevice, AppError> pickDevice(core::Memory<const GPUDevice> gpus) {
    PickedGPUDevice pickedDevice = {};
    i32 prefferedIdx = -1;
    u32 maxScore = 0;

    for (addr_size i = 0; i < gpus.len(); i++) {
        i32 graphicsQueueIdx = -1;
        u32 currScore = getDeviceSutabilityScore(gpus[i], graphicsQueueIdx);
        if (currScore > maxScore) {
            prefferedIdx = i32(i);
            maxScore = currScore;

            pickedDevice.gpu = &gpus[addr_size(prefferedIdx)];
            pickedDevice.graphicsQueueIdx = graphicsQueueIdx;
        }
    }

    if (prefferedIdx < 0) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES));
    }

    return pickedDevice;
}
