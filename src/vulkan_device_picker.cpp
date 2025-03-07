#include <app_logger.h>
#include <app_error.h>
#include <platform.h>
#include <vulkan_renderer.h>

namespace {

using PhysicalDevice = VulkanDevice::PhysicalDevice;
using DeviceExtensions = VulkanDevice::Extensions;

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

void                                                           logPhysicalDevice(const PhysicalDevice& device);

u32                                                            getDeviceSutabilityScore(const PhysicalDevice& gpu,
                                                                                        const VulkanDevice& infoDevice,
                                                                                        QueueFamilyIndices& outIndices,
                                                                                        VulkanSurface::CachedCapabilities& outPickedSurfaceCapabilities,
                                                                                        core::ArrList<bool>& outOptionalExtsActiveList);

core::ArrList<VkQueueFamilyProperties>                         getVkQueueFamilyPropsForDevice(VkPhysicalDevice device);
core::expected<QueueFamilyIndices, AppError>                   findQueueIndices(VkPhysicalDevice device, const VulkanDevice& infoDevice);

core::expected<core::ArrList<VkExtensionProperties>, AppError> getAllSupportedExtensionsForDevice(VkPhysicalDevice device);
void                                                           logAllSupportedExtensionsForDevice(core::Memory<const VkExtensionProperties> exts);
void                                                           logAllDeviceEnabledExtensions(const DeviceExtensions& exts);
bool                                                           checkDeviceExtSupport(const char* ext,
                                                                                     const core::ArrList<VkExtensionProperties>& supportedExts);
bool                                                           checkRequiredDeviceExtsSupport(core::Memory<const char*> exts,
                                                                                              const core::ArrList<VkExtensionProperties>& supportedExts);

void                                                           logSurfaceCapabilities(const VulkanSurface& surface);
bool                                                           pickSurfaceFormat(const core::ArrList<VkSurfaceFormatKHR>& formats,
                                                                                 VkSurfaceFormatKHR& out);
VkPresentModeKHR                                               pickSurfacePresentMode(const core::ArrList<VkPresentModeKHR>& presentModes, bool vSyncOn);
VkExtent2D                                                     pickSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities);

} // namespace

core::expected<AppError> VulkanDevice::pickDevice(core::Memory<const PhysicalDevice> gpus, VulkanDevice& out) {
    i32 prefferedIdx = -1;
    u32 maxScore = 0;

    logInfoTagged(RENDERER_TAG, "Physical Devices ({}) to pick from:", gpus.len());

    for (addr_size i = 0; i < gpus.len(); i++) {
        QueueFamilyIndices queueFamilies{};
        VulkanSurface::CachedCapabilities outPickedSurfaceCapabilities;
        core::ArrList<bool> optionalExtsActiveList (out.deviceExtensions.optional.len(), false);

        logPhysicalDevice(gpus[i]);
        u32 currScore = getDeviceSutabilityScore(gpus[i],
                                                 out,
                                                 queueFamilies,
                                                 outPickedSurfaceCapabilities,
                                                 optionalExtsActiveList);

        if (currScore > maxScore) {
            prefferedIdx = i32(i);
            maxScore = currScore;

            out.physicalDevice = gpus[addr_size(prefferedIdx)].handle;
            out.physicalDeviceFeatures = gpus[addr_size(prefferedIdx)].features;
            out.physicalDeviceProps = gpus[addr_size(prefferedIdx)].props;
            out.graphicsQueue.idx = queueFamilies.graphicsIndex;
            out.presentQueue.idx = queueFamilies.presentIndex;
            out.surface.capabilities = std::move(outPickedSurfaceCapabilities);
            out.deviceExtensions.optionalIsActive = std::move(optionalExtsActiveList);
        }
    }

    if (prefferedIdx < 0) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES));
    }

    logInfoTagged(RENDERER_TAG, ANSI_BOLD("Selected GPU: {}"), out.physicalDeviceProps.deviceName);
    logAllDeviceEnabledExtensions(out.deviceExtensions);
    logSurfaceCapabilities(out.surface);

    return {};
}

VulkanSurface::Capabilities VulkanSurface::queryCapabilities(const VulkanSurface& surface,
                                                             VkPhysicalDevice physicalDevice) {
    VulkanSurface::Capabilities details;

    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
    VK_MUST(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice,
                                                      surface.handle,
                                                      &details.capabilities));

    // Surface formats (pixel format, color space)
    u32 formatCount;
    VK_MUST(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
                                                 surface.handle,
                                                 &formatCount,
                                                 nullptr));
    details.formats.replaceWith(VkSurfaceFormatKHR{}, formatCount);
    VK_MUST(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
                                                 surface.handle,
                                                 &formatCount,
                                                 details.formats.data()));

    // Available presentation modes
    uint32_t presentModeCount;
    VK_MUST(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
                                                      surface.handle,
                                                      &presentModeCount,
                                                      nullptr));
    details.presentModes.replaceWith(VkPresentModeKHR{}, presentModeCount);
    VK_MUST(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
                                                      surface.handle,
                                                      &presentModeCount,
                                                      details.presentModes.data()));

    return details;
}

core::expected<VulkanSurface::CachedCapabilities, AppError> VulkanSurface::pickCapabilities(
    const VulkanSurface::Capabilities& surfaceCapabilities,
    bool vSyncOn
) {
    const auto& formats = surfaceCapabilities.formats;
    const auto& presentModes = surfaceCapabilities.presentModes;
    const auto& capabilities = surfaceCapabilities.capabilities;

    if (formats.empty()) {
        // No supported formats
        return core::unexpected(createRendErr(RendererError::FAILED_TO_PICK_SUTABLE_SURFACE_FOR_SWAPCHAIN));
    }
    if (presentModes.empty()) {
        // No supported present modes
        return core::unexpected(createRendErr(RendererError::FAILED_TO_PICK_SUTABLE_SURFACE_FOR_SWAPCHAIN));
    }

    VulkanSurface::CachedCapabilities ret;

    ret.presentMode = pickSurfacePresentMode(presentModes, vSyncOn);
    ret.extent = pickSurfaceExtent(capabilities);

    ret.imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && ret.imageCount > capabilities.maxImageCount) {
        ret.imageCount = capabilities.maxImageCount;
    }

    ret.currentTransform = capabilities.currentTransform;

    if (!pickSurfaceFormat(formats, ret.format)) {
        // No suttable surface format
        return core::unexpected(createRendErr(RendererError::FAILED_TO_PICK_SUTABLE_SURFACE_FOR_SWAPCHAIN));
    }

    return ret;
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
 * @param infoDevice External information to aid the scoring.
 * @param out* Output arguments.
 *
 * @return The score.
*/

u32 getDeviceSutabilityScore(
    const PhysicalDevice& gpu,
    const VulkanDevice& infoDevice,
    QueueFamilyIndices& outIndices,
    VulkanSurface::CachedCapabilities& outPickedSurfaceCapabilities,
    core::ArrList<bool>& outOptionalExtsActiveList
) {
    const auto& device = gpu.handle;
    const auto& props = gpu.props;
    u32 score = 0;

    // Verify required Queues are supported
    {
        auto res = findQueueIndices(gpu.handle, infoDevice);
        if (res.hasErr()) {
            logWarnTagged(RENDERER_TAG, "Failed to find queue indices for device: {}, reason: {}",
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
            logWarnTagged(RENDERER_TAG, "Failed to get all supported extensions for device: {}, reason: {}",
                         gpu.props.deviceName, res.err().toCStr());
            return 0;
        }
        supportedDeviceExts = std::move(res.value());
    }

    // Log all supported device extensions
    {
        logAllSupportedExtensionsForDevice(supportedDeviceExts.memView());
    }

    // Check Required Device extensions support
    {
        bool supported = checkRequiredDeviceExtsSupport(infoDevice.deviceExtensions.required, supportedDeviceExts);
        if (!supported) {
            return 0;
        }

        // Has support for required extensions
        score++;
    }

    // Check Optional Device extensions support
    {
        for (addr_size i = 0; i < infoDevice.deviceExtensions.optional.len(); i++) {
            const char* ext = infoDevice.deviceExtensions.optional[i];
            bool supported = checkDeviceExtSupport(ext, supportedDeviceExts);
            if (supported) {
                score++;
                outOptionalExtsActiveList[i] = true;
            }
            else {
                logWarnTagged(RENDERER_TAG, "Device does not support optional extension: {}", ext);
            }
        }
    }

    // Give a score for the supported Surface features.
    {
        VulkanSurface::Capabilities surfaceCapabilities = VulkanSurface::queryCapabilities(infoDevice.surface, gpu.handle);
        auto res = VulkanSurface::pickCapabilities(surfaceCapabilities, infoDevice.vSyncOn);
        if (res.hasErr()) {
            // This should be rare.
            logWarnTagged(RENDERER_TAG, "Device surface does not support the required capabilities.");
            return 0;
        }
        outPickedSurfaceCapabilities = std::move(res.value());
    }

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // Discrete GPUs are preffered.
        score += 100;
    }

    return score;
}

void logPhysicalDevice(const VulkanDevice::PhysicalDevice& device) {
    auto& gpu = device;
    auto& props = gpu.props;

    logInfoTagged(RENDERER_TAG, "");
    logInfoTagged(RENDERER_TAG, "Device Name: {}", props.deviceName);
    logInfoTagged(RENDERER_TAG, "API Version: {}.{}.{}",
                    VK_VERSION_MAJOR(props.apiVersion),
                    VK_VERSION_MINOR(props.apiVersion),
                    VK_VERSION_PATCH(props.apiVersion));
    logInfoTagged(RENDERER_TAG, "Driver Version: {}",
                    props.driverVersion);
    logInfoTagged(RENDERER_TAG, "Vendor ID: {}, Device ID: {}",
                    props.vendorID, props.deviceID);
    logInfoTagged(RENDERER_TAG, "Device Type: {}",
                    props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "Integrated GPU" :
                    props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "Discrete GPU" :
                    props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ? "Virtual GPU" :
                    props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ? "CPU" : "Other");
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
        if (VkResult vres = vkGetPhysicalDeviceSurfaceSupportKHR(device, u32(i), surface, &presentSupport);
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

core::expected<QueueFamilyIndices, AppError> findQueueIndices(VkPhysicalDevice device, const VulkanDevice& infoDevice) {
    auto vkQueueFamilyProps = getVkQueueFamilyPropsForDevice(device);
    auto res = QueueFamilyIndices::create(device, infoDevice.surface.handle, vkQueueFamilyProps);
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

void logAllSupportedExtensionsForDevice(core::Memory<const VkExtensionProperties> exts) {
    logDebugTagged(RENDERER_TAG, "Device Supported Extensions ({}):", exts.len());
    for (addr_size i = 0; i < exts.len(); i++) {
        logDebugTagged(RENDERER_TAG, "\t{} (v{})", exts[i].extensionName, exts[i].specVersion);
    }
}

void logAllDeviceEnabledExtensions(const DeviceExtensions& exts) {
    logInfoTagged(RENDERER_TAG, "Device Enabled Extensions ({}):", exts.enabledExtensionsCount());
    for (addr_size i = 0; i < exts.required.len(); i++) {
        const char* ext = exts.required[i];
        logInfoTagged(RENDERER_TAG, "\t{} (required)", ext);
    }
    for (addr_size i = 0; i < exts.optional.len(); i++) {
        if (exts.optionalIsActive[i]) {
            const char* ext = exts.optional[i];
            logInfoTagged(RENDERER_TAG, "\t{} (optional)", ext);
        }
    }
}

bool checkDeviceExtSupport(const char* ext, const core::ArrList<VkExtensionProperties>& supportedExts) {
    const addr_size currExtNameLen = core::cstrLen(ext);

    addr_off foundIdx = core::find(supportedExts, [&ext, &currExtNameLen](const VkExtensionProperties& v, addr_size) {
        i32 diff = core::memcmp(ext, currExtNameLen, v.extensionName, core::cstrLen(v.extensionName));
        return diff == 0;
    });

    if (foundIdx < 0) {
        return false;
    }

    return true;
}


bool checkRequiredDeviceExtsSupport(core::Memory<const char*> exts, const core::ArrList<VkExtensionProperties>& supportedExts) {
    for (addr_size i = 0; i < exts.len(); i++) {
        const char* currExtName = exts[i];
        if (!checkDeviceExtSupport(currExtName, supportedExts)) {
            logInfoTagged(RENDERER_TAG, "Device does not support required extension: {}", currExtName);
            return false;
        }
    }

    return true;
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

VkPresentModeKHR pickSurfacePresentMode(const core::ArrList<VkPresentModeKHR>& presentModes, bool vSyncOn) {
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

    if (!vSyncOn) {
        for (addr_size i = 0; i < presentModes.len(); i++) {
           if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }

        logWarnTagged(RENDERER_TAG, "Can't turn off VSync - VK_PRESENT_MODE_IMMEDIATE_KHR is not supported.");
    }
    else {
        for (addr_size i = 0; i < presentModes.len(); i++) {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                return VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }
    }

    logInfoTagged(RENDERER_TAG, "Defaulting to VK_PRESENT_MODE_FIFO_KHR");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pickSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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

void logSurfaceCapabilities(const VulkanSurface& surface) {
    logInfoTagged(RENDERER_TAG, "Surface Capabilities:");
    logInfoTagged(RENDERER_TAG, "\tformat: {}", surface.capabilities.format.format);
    logInfoTagged(RENDERER_TAG, "\tcolorSpace: {}", surface.capabilities.format.colorSpace);
    logInfoTagged(RENDERER_TAG, "\tpresent_mode: {} (vSync={})",
        surface.capabilities.presentMode,
        surface.capabilities.presentMode != VK_PRESENT_MODE_IMMEDIATE_KHR ? "on": "off");
    logInfoTagged(RENDERER_TAG, "\textent: w={}, h={}",
        surface.capabilities.extent.width, surface.capabilities.extent.height);
    logInfoTagged(RENDERER_TAG, "\tcurrent_transform: {}", surface.capabilities.currentTransform);
    logInfoTagged(RENDERER_TAG, "\timage_count: {}", surface.capabilities.imageCount);
}

} // namespace
