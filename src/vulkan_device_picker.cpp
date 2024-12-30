#include <app_logger.h>
#include <vulkan_device_picker.h>
#include <platform.h>

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

struct SwapchainFeatureDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    core::ArrList<VkSurfaceFormatKHR> formats;
    core::ArrList<VkPresentModeKHR> presentModes;
};

u32                                                            getDeviceSutabilityScore(const GPUDevice& gpu,
                                                                                        const PickDeviceInfo& info,
                                                                                        QueueFamilyIndices& outIndices,
                                                                                        VkSurfaceFormatKHR& outSurfaceFormat,
                                                                                        VkPresentModeKHR& outPresentMode,
                                                                                        VkExtent2D& outExtent,
                                                                                        u32& imageCount,
                                                                                        VkSurfaceTransformFlagBitsKHR& currentTransform);
core::ArrList<VkQueueFamilyProperties>                         getVkQueueFamilyPropsForDevice(VkPhysicalDevice device);
core::expected<QueueFamilyIndices, AppError>                   findQueueIndices(VkPhysicalDevice device, const PickDeviceInfo& info);
core::expected<core::ArrList<VkExtensionProperties>, AppError> getAllSupportedExtensionsForDevice(VkPhysicalDevice device);
bool                                                           checkRequiredExtSupport(const PickDeviceInfo& info,
                                                                                       const core::ArrList<VkExtensionProperties>& supportedDeviceExts);
core::expected<SwapchainFeatureDetails, AppError>              getSwapchainFeatures(VkPhysicalDevice device, const PickDeviceInfo& info);
bool                                                           pickSurfaceFormat(const core::ArrList<VkSurfaceFormatKHR>& formats,
                                                                                 VkSurfaceFormatKHR& out);
VkPresentModeKHR                                               pickSwapPresentMode(const core::ArrList<VkPresentModeKHR>& presentModes,
                                                                                   u32& score);
VkExtent2D                                                     pickSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

} // namespace

core::expected<PickedGPUDevice, AppError> pickDevice(core::Memory<const GPUDevice> gpus, const PickDeviceInfo& info) {
    PickedGPUDevice pickedDevice = {};
    i32 prefferedIdx = -1;
    u32 maxScore = 0;

    for (addr_size i = 0; i < gpus.len(); i++) {
        QueueFamilyIndices queueFamilies{};
        VkSurfaceFormatKHR surfaceFormat{};
        VkPresentModeKHR presentMode{};
        VkExtent2D extent{};
        u32 imageCount = 0;
        VkSurfaceTransformFlagBitsKHR currentTransform;

        u32 currScore = getDeviceSutabilityScore(gpus[i],
                                                 info,
                                                 queueFamilies,
                                                 surfaceFormat,
                                                 presentMode,
                                                 extent,
                                                 imageCount,
                                                 currentTransform);

        if (currScore > maxScore) {
            prefferedIdx = i32(i);
            maxScore = currScore;

            pickedDevice.gpu = &gpus[addr_size(prefferedIdx)];
            pickedDevice.graphicsQueueIdx = queueFamilies.graphicsIndex;
            pickedDevice.presentQueueIdx = queueFamilies.presentIndex;
            pickedDevice.surfaceFormat = std::move(surfaceFormat);
            pickedDevice.presentMode = std::move(presentMode);
            pickedDevice.extent = std::move(extent);
            pickedDevice.imageCount = imageCount;
            pickedDevice.currentTransform = currentTransform;
        }
    }

    if (prefferedIdx < 0) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES));
    }

    return pickedDevice;
}

namespace  {

/**
 * @brief Gives a score for the physical device based on the supported feature set. Higher is better. A score of 0 means
 *        that the device does not meet the application's minimal requirements.
 *
 * TODO2: This function does a too much, it should probably be split up into different functions that score different
 *        components.
 *
 * @param gpu The device to score.
 * @param info External information to aid the scoring.
 * @param out* Output arguments.
 *
 * @return The score.
*/

u32 getDeviceSutabilityScore(
    const GPUDevice& gpu,
    const PickDeviceInfo& info,
    QueueFamilyIndices& outIndices,
    VkSurfaceFormatKHR& outSurfaceFormat,
    VkPresentModeKHR& outPresentMode,
    VkExtent2D& outExtent,
    u32& outImageCount,
    VkSurfaceTransformFlagBitsKHR& outCurrentTransform
) {
    const auto& device = gpu.device;
    const auto& props = gpu.props;
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

    // Get features for Swapchain
    SwapchainFeatureDetails swapchainFeatureDetails;
    {
        auto res = getSwapchainFeatures(device, info);
        if (res.hasErr()) {
            logWarnTagged(RENDERER_TAG, "Failed to get Swapchain features for device: %s, reason: %s",
                         gpu.props.deviceName, res.err().toCStr());
            return 0;
        }
        swapchainFeatureDetails = std::move(res.value());
    }

    // Give a score for the supported Swapchain features.
    {
        const auto& formats = swapchainFeatureDetails.formats;
        const auto& presentModes = swapchainFeatureDetails.presentModes;
        const auto& capabilities = swapchainFeatureDetails.capabilities;

        if (formats.empty()) return 0; // No supported formats
        if (presentModes.empty()) return 0; // No supported present modes

        outPresentMode = pickSwapPresentMode(presentModes, score);

        outExtent = pickSwapExtent(capabilities);

        outImageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && outImageCount > capabilities.maxImageCount) {
            outImageCount = capabilities.maxImageCount;
        }

        outCurrentTransform = capabilities.currentTransform;

        if (!pickSurfaceFormat(formats, outSurfaceFormat)) {
            return 0; // No suttable surface format
        }
    }

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // Discrete GPUs are preffered.
        score += 100;
    }

    return score;
}

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

core::expected<SwapchainFeatureDetails, AppError> getSwapchainFeatures(VkPhysicalDevice device, const PickDeviceInfo& info) {
    SwapchainFeatureDetails details;

    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
    if (
        VkResult vres = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, info.surface, &details.capabilities);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES));
    }

    // Surface formats (pixel format, color space)
    u32 formatCount;
    if (
        VkResult vres = vkGetPhysicalDeviceSurfaceFormatsKHR(device, info.surface, &formatCount, nullptr);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_FORMATS));
    }

    details.formats = core::ArrList<VkSurfaceFormatKHR>(formatCount, VkSurfaceFormatKHR{});
    if (
        VkResult vres = vkGetPhysicalDeviceSurfaceFormatsKHR(device,
                                                             info.surface,
                                                             &formatCount,
                                                             details.formats.data());
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_FORMATS));
    }

    // Available presentation modes
    uint32_t presentModeCount;
    if (
        VkResult vres = vkGetPhysicalDeviceSurfacePresentModesKHR(device, info.surface, &presentModeCount, nullptr);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_PRESENT_MODES));
    }

    details.presentModes = core::ArrList<VkPresentModeKHR>(presentModeCount, VkPresentModeKHR{});
    if (
        VkResult vres = vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                                                  info.surface,
                                                                  &presentModeCount,
                                                                  details.presentModes.data());
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_PRESENT_MODES));
    }

    return details;
}

bool pickSurfaceFormat(const core::ArrList<VkSurfaceFormatKHR>& formats, VkSurfaceFormatKHR& out) {
    // NOTE:
    //
    // The Vulkan specification mandates that implementations supporting swapchains must support
    // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR. This ensures that any Vulkan-compliant GPU will
    // have this color space available and I can REQUIRE it here!
    //
    // Vulkan implementations that support swapchains are required to support at least one sRGB format, and
    // VK_FORMAT_B8G8R8A8_SRGB is ALMOST ALWAYS included.

    for (addr_size i = 0; i < formats.len(); i++) {
        const auto& f = formats[i];
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            out.format = f.format;
            out.colorSpace = f.colorSpace;
            return true;
        }
    }

    return false;
}

VkPresentModeKHR pickSwapPresentMode(const core::ArrList<VkPresentModeKHR>& presentModes, u32& score) {
    // NOTE: From the Vulkan Tutorial
    //
    // The presentation mode is arguably the most important setting for the swap chain, because it represents the actual
    // conditions for showing images to the screen. There are 4 commonly used modes in Vulkan:
    // * VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away,
    //   which may result in tearing.
    // * VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the
    //   queue when the display is refreshed and the program inserts rendered images at the back of the queue. If the
    //   queue is full then the program has to WAIT. This is most similar to VERTICAL SYNC. The moment that the display
    //   is refreshed is known as "vertical blank".
    // * VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and
    //   the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is
    //   transferred right away when it finally arrives. This may result in visible tearing.
    // * VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application
    //   when the queue is full, the images that are already queued are simply replaced with the newer ones. This mode
    //   can be used to render frames as fast as possible while still avoiding tearing, resulting in fewer latency
    //   issues than standard vertical sync. This is commonly known as "triple buffering", although the existence of
    //   three buffers alone does not necessarily mean that the framerate is unlocked.

    // TODO2: Do I actually need trieple buffering?
    for (addr_size i = 0; i < presentModes.len(); i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    // Decreasing the score because VK_PRESENT_MODE_FIFO_KHR is suboptimal, but it is the only Vulkan standard required
    // present mode.
    score--;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pickSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != core::limitMax<u32>()) {
        return capabilities.currentExtent;
    }

    u32 width, height;
    Platform::getFrameBufferSize(width, height);

    VkExtent2D actualExtent = { width, height };

    actualExtent.width = core::clamp(actualExtent.width,
                                     capabilities.minImageExtent.width,
                                     capabilities.maxImageExtent.width);
    actualExtent.height = core::clamp(actualExtent.height,
                                      capabilities.minImageExtent.height,
                                      capabilities.maxImageExtent.height);

    return actualExtent;
}

} // namespace
