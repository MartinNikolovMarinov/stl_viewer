#include <app_logger.h>
#include <vulkan_device_picker.h>

namespace {

struct QueueFamilyIndices {
    i32 graphicsIndex = -1;
    i32 presentIndex  = -1;

    constexpr bool hasMinimumSupport() {
        return graphicsIndex >= 0 && presentIndex >= 0;
    }

    static core::expected<QueueFamilyIndices, AppError> create(
        VkPhysicalDevice device,
        VkSurfaceKHR surface,
        const core::ArrList<VkQueueFamilyProperties>& vkQueueFamilyProps
    );
};

core::ArrList<VkQueueFamilyProperties>                         getVkQueueFamilyPropsForDevice(VkPhysicalDevice device);
core::expected<QueueFamilyIndices, AppError>                   findQueueIndices(VkPhysicalDevice device, const PickDeviceInfo& info);
core::expected<core::ArrList<VkExtensionProperties>, AppError> getAllSupportedExtensionsForDevice(VkPhysicalDevice device);
bool                                                           checkRequiredExtSupport(const PickDeviceInfo& info,
                                                                                       const core::ArrList<VkExtensionProperties>& supportedDeviceExts);

/**
 * @brief Gives a score for the physical device based on the supported feature set. Higher is better. A score of 0 means
 *        that the device does not meet the application's minimal requirements.
 *
 * @param gpu The device to score.
 * @param info External information to aid the scoring.
 * @param outIndices The output indices for the required queues.
 *
 * @return The score.
*/
u32 getDeviceSutabilityScore(const GPUDevice& gpu, const PickDeviceInfo& info, QueueFamilyIndices& outIndices) {
    const auto& device = gpu.device;
    const auto& props = gpu.props;
    const auto& features = gpu.features;
    u32 score = 0;

    // Verify required Queues are supported
    {
        auto res = findQueueIndices(gpu.device, info);
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
    }

    // Get all device extensions
    core::ArrList<VkExtensionProperties> supportedDeviceExts;
    {
        auto res = getAllSupportedExtensionsForDevice(device);
        if (res.hasErr()) {
            logWarnTagged(RENDERER_TAG, "Failed to get all supported extensions for device: %s, reason: %s",
                         gpu.props.deviceName, res.err().toCStr());
            return 0;
        }
        supportedDeviceExts = std::move(res.value());
    }

    // Check Required Device extensions support
    {
        bool supported = checkRequiredExtSupport(info, supportedDeviceExts);
        if (!supported) {
            return 0;
        }

        // Has support for required extensions
        score++;
    }


    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // Discrete GPUs are preffered.
        score += 100;
    }

    return score;
}

} // namespace

core::expected<PickedGPUDevice, AppError> pickDevice(core::Memory<const GPUDevice> gpus, const PickDeviceInfo& info) {
    PickedGPUDevice pickedDevice = {};
    i32 prefferedIdx = -1;
    u32 maxScore = 0;

    for (addr_size i = 0; i < gpus.len(); i++) {
        QueueFamilyIndices queueFamilies{};
        u32 currScore = getDeviceSutabilityScore(gpus[i], info, queueFamilies);
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

namespace  {

core::ArrList<VkQueueFamilyProperties> getVkQueueFamilyPropsForDevice(VkPhysicalDevice device) {
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    core::ArrList<VkQueueFamilyProperties> queueFamilies(queueFamilyCount, VkQueueFamilyProperties{});
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    return queueFamilies;
}

core::expected<QueueFamilyIndices, AppError> QueueFamilyIndices::create(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const core::ArrList<VkQueueFamilyProperties>& vkQueueFamilyProps
) {
    AppError err {};
    QueueFamilyIndices ret{};

    auto findGraphicsQueue = [](const VkQueueFamilyProperties& x, addr_size) {
        VkQueueFlags flags = x.queueFlags;
        bool supportsGraphics = flags & VK_QUEUE_GRAPHICS_BIT;
        return supportsGraphics;
    };

    auto findPresentQueue = [&err, &device, &surface](const VkQueueFamilyProperties&, addr_size i) {
        VkBool32 presentSupport = VK_FALSE;
        if (VkResult vres = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            vres != VK_SUCCESS) {
            err = createRendErr(RendererError::FAILED_TO_QUERY_FOR_PRESENT_QUEUE_SUPPORT);
            return true; // return true for early exit
        }
        return presentSupport == VK_TRUE;
    };

    ret.graphicsIndex = i32(core::find(vkQueueFamilyProps, findGraphicsQueue));
    if (ret.graphicsIndex == -1) return ret;

    ret.presentIndex = i32(core::find(vkQueueFamilyProps, findPresentQueue));
    if (!err.isOk()) return core::unexpected(err);
    if (ret.presentIndex == -1) return ret;

    return ret;
}

core::expected<QueueFamilyIndices, AppError> findQueueIndices(VkPhysicalDevice device, const PickDeviceInfo& info) {
    auto vkQueueFamilyProps = getVkQueueFamilyPropsForDevice(device);
    auto res = QueueFamilyIndices::create(device, info.surface, vkQueueFamilyProps);
    return res;
}

core::expected<core::ArrList<VkExtensionProperties>, AppError> getAllSupportedExtensionsForDevice(VkPhysicalDevice device) {
    u32 extCount = 0;
    if (
        VkResult vres = vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_ENUMERATE_VULKAN_DEVICE_EXTENSION_PROPERTIES));
    }

    core::ArrList<VkExtensionProperties> availableExts(extCount, VkExtensionProperties{});
    if (
        VkResult vres = vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, availableExts.data());
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_ENUMERATE_VULKAN_DEVICE_EXTENSION_PROPERTIES));
    }

    return availableExts;
}

bool checkRequiredExtSupport(
    const PickDeviceInfo& info,
    const core::ArrList<VkExtensionProperties>& supportedDeviceExts
) {
    for (addr_size i = 0; i < info.requiredExtensions.len(); i++) {
        const char* currExtName = info.requiredExtensions[i];
        const addr_size currExtNameLen = core::cstrLen(currExtName);

        addr_off foundIdx = core::find(supportedDeviceExts, [&currExtName, &currExtNameLen](const VkExtensionProperties& v, addr_size) {
            i32 diff = core::memcmp(currExtName, currExtNameLen,
                                    v.extensionName, core::cstrLen(v.extensionName));
            return diff == 0;
        });

        if (foundIdx < 0) {
            logDebugTagged(RENDERER_TAG, "Device does not support extension: %s", currExtName);
            return false;
        }
    }

    return true;
}

} // namespace
