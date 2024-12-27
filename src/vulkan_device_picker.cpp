#include <app_logger.h>
#include <vulkan_device_picker.h>

namespace {

struct QueueFamilyIndices {
    i32 graphicsIndex = -1;
    i32 presentIndex  = -1;

    constexpr bool hasMinimumSupport() {
        return graphicsIndex >= 0 && presentIndex >= 0;
    }
};

core::expected<QueueFamilyIndices, AppError> findQueueIndices(VkPhysicalDevice physDevice, VkSurfaceKHR surface) {
    QueueFamilyIndices ret{};

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

    core::ArrList<VkQueueFamilyProperties> queueFamilies(queueFamilyCount, VkQueueFamilyProperties{});
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

    for (addr_size i = 0; i < queueFamilies.len(); i++) {
        // Check for graphics
        const VkQueueFlags flags = queueFamilies[i].queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT) {
            ret.graphicsIndex = i32(i); // Found a graphics queue
        }

        // Check for present support
        VkBool32 presentSupport = false;
        if (VkResult vres = vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &presentSupport);
            vres != VK_SUCCESS) {
            return core::unexpected(createRendErr(RendererError::FAILED_TO_QUERY_FOR_PRESENT_QUEUE_SUPPORT));
        }
        if (presentSupport) {
            if (ret.presentIndex < 0) {
                ret.presentIndex = i32(i); // Found present queue
            }
        }

        if (
            ret.graphicsIndex >= 0 &&
            ret.presentIndex >= 0
        ) {
            break;
        }
    }

    return ret;
}

/**
 * @brief Gives a score for the physical device based on the supported feature set. Higher is better. A score of 0 means
 *        that the device does not meet the application's minimal requirements.
 *
 * @param gpu The device to score.
 *
 * @return The score.
*/
u32 getDeviceSutabilityScore(const GPUDevice& gpu, VkSurfaceKHR surface, QueueFamilyIndices& outIndices) {
    const auto& props = gpu.props;
    const auto& features = gpu.features;
    u32 score = 0;

    auto res = findQueueIndices(gpu.device, surface);
    if (res.hasErr()) {
        logWarnTagged(RENDERER_TAG, "Failed to find queue indices for device: %s, reason: %s",
                      gpu.props.deviceName, res.err().toCStr());
        return 0;
    }

    QueueFamilyIndices& queueFamilies = res.value();
    if (!queueFamilies.hasMinimumSupport()) {
        return 0;
    }
    // Has minimum required support for queues.
    score++;
    outIndices = queueFamilies;

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // Discrete GPUs are preffered.
        score += 100;
    }

    return score;
}

} // namespace

core::expected<PickedGPUDevice, AppError> pickDevice(core::Memory<const GPUDevice> gpus, VkSurfaceKHR surface) {
    PickedGPUDevice pickedDevice = {};
    i32 prefferedIdx = -1;
    u32 maxScore = 0;

    for (addr_size i = 0; i < gpus.len(); i++) {
        QueueFamilyIndices queueFamilies{};
        u32 currScore = getDeviceSutabilityScore(gpus[i], surface, queueFamilies);
        if (currScore > maxScore) {
            prefferedIdx = i32(i);
            maxScore = currScore;

            pickedDevice.gpu = &gpus[addr_size(prefferedIdx)];
            pickedDevice.graphicsQueueIdx = queueFamilies.graphicsIndex;
            pickedDevice.presentQueueIdx = queueFamilies.presentIndex;
        }
    }

    if (prefferedIdx < 0) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES));
    }

    return pickedDevice;
}
