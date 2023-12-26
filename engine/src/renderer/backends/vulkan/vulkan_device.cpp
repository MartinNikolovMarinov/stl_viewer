#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

namespace {

struct VulkanPhysicalDeviceRequirements;
struct QueueIndices;

// Format candidates in order of preference.
VkFormat requiredDepthFormats[] = {
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT
};
constexpr addr_size requiredDepthFormatsLen = sizeof(requiredDepthFormats) / sizeof(VkFormat);

constexpr const char* depthFormatToCptr(VkFormat format) {
    if (format == VK_FORMAT_D32_SFLOAT)         return "VK_FORMAT_D32_SFLOAT";
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT) return "VK_FORMAT_D32_SFLOAT_S8_UINT";
    if (format == VK_FORMAT_D24_UNORM_S8_UINT)  return "VK_FORMAT_D24_UNORM_S8_UINT";

    return "UNKNOWN";
}

bool selectPhysicalDevice(RendererBackend& backend, const char** requiredDeviceExts, addr_size requiredDeviceExtsLen);
bool createLogicalDevice(RendererBackend& backend, const char** requiredDeviceExts, addr_size requiredDeviceExtsLen);
bool vulkanDeviceDetectDepthFormat(VkPhysicalDevice pdevice, VkFormat& outFormat);
bool verifySwapchainSupportForExtensions(VkPhysicalDevice pdevice,
                                         const char** requiredDeviceExts,
                                         addr_size requiredDeviceExtsLen);
void giveCompatibilityScoreForDevice(VkPhysicalDevice pdevice,
                                     VkSurfaceKHR surface,
                                     const VulkanPhysicalDeviceRequirements& requirements,
                                     const VkPhysicalDeviceProperties& properties,
                                     const VkPhysicalDeviceFeatures& features,
                                     const VkPhysicalDeviceMemoryProperties& memoryProperties,
                                     u32& outScore,
                                     QueueIndices& outIndices,
                                     VkFormat& outDepthFormat);

} // namespace

bool vulkanDeviceCreate(RendererBackend& backend) {
    const char* requiredDeviceExts[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    constexpr addr_size requiredDeviceExtsLen = sizeof(requiredDeviceExts) / sizeof(const char*);

    logSectionTitleInfoTagged(LogTag::T_RENDERER, "Selecting Vulkan physical device...");
    if (!selectPhysicalDevice(backend, requiredDeviceExts, requiredDeviceExtsLen)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to select physical device.");
        return false;
    }
    logSectionTitleInfoTagged(LogTag::T_RENDERER, "Physical device selected.");

    logInfoTagged(LogTag::T_RENDERER, "Creating logical device...");
    if (!createLogicalDevice(backend, requiredDeviceExts, requiredDeviceExtsLen)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create logical device.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Logical device created.");

    logInfoTagged(LogTag::T_RENDERER, "Creating graphics command pool...");
    VkCommandPoolCreateInfo createPoolInfo{};
    createPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // TODO: Consider resseting the entire pool at some far point in the future.
    createPoolInfo.queueFamilyIndex = u32(backend.device.graphicsQueue.familyIndex);
    VK_EXPECT_OR_RETURN(
        vkCreateCommandPool(
            backend.device.logicalDevice,
            &createPoolInfo,
            backend.allocator,
            &backend.device.graphicsCommandPool),
        "Failed to create graphics command pool."
    )
    logInfoTagged(LogTag::T_RENDERER, "Graphics command pool created.");

    return true;
}

void vulkanDeviceDestroy(RendererBackend& backend) {
    VulkanDevice& device = backend.device;

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan graphics command pool.");
    if (device.graphicsCommandPool) {
        vkDestroyCommandPool(device.logicalDevice, device.graphicsCommandPool, backend.allocator);
    }

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan logical device.");
    if (device.logicalDevice) {
        vkDestroyDevice(device.logicalDevice, backend.allocator);
    }

    device = {};
}

void vulkanDeviceQuerySwapchainSupport(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo& info) {
    VK_EXPECT(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice, surface, &info.capabilities),
        "Failed to get surface capabilities."
    );

    u32 formatCount = 0;
    VK_EXPECT(
        vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface, &formatCount, nullptr),
        "Failed to get surface formats."
    );
    if (formatCount != 0) {
        if (info.formats.len() < formatCount) {
            info.formats = VkSurfaceFormatKHRList (formatCount);
        }
        VK_EXPECT(
            vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface, &formatCount, info.formats.data()),
            "Failed to get surface formats."
        );
    }

    u32 presentModeCount = 0;
    VK_EXPECT(
        vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice, surface, &presentModeCount, nullptr),
        "Failed to get surface present modes."
    );
    if (presentModeCount != 0) {
        if (info.presentModes.len() < presentModeCount) {
            info.presentModes = VkPresentModeKHRList (presentModeCount);
        }
        VK_EXPECT(
            vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice, surface, &presentModeCount, info.presentModes.data()),
            "Failed to get surface present modes."
        );
    }
}

namespace {

struct VulkanPhysicalDeviceRequirements {
    struct Requirement {
        bool isRequired;
        u32 scoreWeight;
    };

    Requirement descreteGPU;
    Requirement anisotropy;
    Requirement depthFormat;

    Requirement graphicsQueue;
    Requirement computeQueue;
    Requirement transferQueue;
    Requirement transferIsOnSeparateQueue;
    Requirement presentQueue;

    const char** requiredDeviceExts;
    addr_size requiredDeviceExtsLen;
};

struct QueueIndices {
    i32 graphicsFamilyIdx;
    i32 computeFamilyIdx;
    i32 transferFamilyIdx;
    i32 presentFamilyIdx;
};

constexpr QueueIndices UNSET_QUEUE_INDICES = {-1, -1, -1, -1};
constexpr u32 LOW_SCORE = 30;

bool selectPhysicalDevice(RendererBackend& backend, const char** requiredDeviceExts, addr_size requiredDeviceExtsLen) {
    u32 physicalDeviceCount = 0;
    VK_EXPECT_OR_RETURN(
        vkEnumeratePhysicalDevices(backend.instance, &physicalDeviceCount, nullptr),
        "Failed to enumerate physical devices."
    );
    if (physicalDeviceCount == 0) {
        logErrTagged(LogTag::T_RENDERER, "No physical devices on the system support Vulkan.");
        return false;
    }

    core::Arr<VkPhysicalDevice, RendererBackendAllocator> physicalDevices (physicalDeviceCount);
    VK_EXPECT_OR_RETURN(
        vkEnumeratePhysicalDevices(backend.instance, &physicalDeviceCount, physicalDevices.data()),
        "Failed to enumerate physical devices."
    );

    core::Arr<u32, RendererBackendAllocator> scores (physicalDeviceCount);
    core::Arr<QueueIndices, RendererBackendAllocator> queueIndices (physicalDeviceCount);
    u32 highestScore = 0;

    VulkanPhysicalDeviceRequirements requirements = {};
    requirements.descreteGPU = { false, 30 };
    requirements.anisotropy = { true, 1 };
    requirements.depthFormat = { true, 1 };
    requirements.graphicsQueue = { true, 1 };
    requirements.computeQueue = { false, 1 };
    requirements.transferQueue = { true, 1 };
    requirements.transferIsOnSeparateQueue = { false, 5 };
    requirements.presentQueue = { true, 1 };
    requirements.requiredDeviceExts = requiredDeviceExts;
    requirements.requiredDeviceExtsLen = requiredDeviceExtsLen;

    for (u32 i = 0; i < physicalDeviceCount; ++i) {
        auto& pdevice = physicalDevices[i];

        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceMemoryProperties memoryProperties;

        vkGetPhysicalDeviceProperties(pdevice, &properties);
        vkGetPhysicalDeviceFeatures(pdevice, &features);
        vkGetPhysicalDeviceMemoryProperties(pdevice, &memoryProperties);

        logInfoTagged(LogTag::T_RENDERER, "Checking compatibility for device: %s", properties.deviceName);

        VulkanSwapchainSupportInfo swapChainInfo;

        vulkanDeviceQuerySwapchainSupport(pdevice, backend.surface, swapChainInfo);
        if (swapChainInfo.formats.len() < 1 || swapChainInfo.presentModes.len() < 1) {
            logErrTagged(LogTag::T_RENDERER, "Device's swapchain does not support any formats or present modes.");
            continue;
        }

        if (!verifySwapchainSupportForExtensions(pdevice,
                                                 requirements.requiredDeviceExts,
                                                 requirements.requiredDeviceExtsLen)) {
            logErrTagged(LogTag::T_RENDERER, "Device's swapchain does not support required extensions.");
            continue;
        }

        VkFormat depthFormat = VK_FORMAT_UNDEFINED;
        logInfoTagged(LogTag::T_RENDERER, "Giving device compatibility score...");
        giveCompatibilityScoreForDevice(pdevice, backend.surface, requirements,
                                        properties, features, memoryProperties,
                                        scores[i],
                                        queueIndices[i],
                                        depthFormat);
        if (scores[i] > highestScore) {
            highestScore = scores[i];

            backend.device.graphicsQueue.familyIndex = u32(queueIndices[i].graphicsFamilyIdx);
            backend.device.computeQueue.familyIndex  = u32(queueIndices[i].computeFamilyIdx);
            backend.device.transferQueue.familyIndex = u32(queueIndices[i].transferFamilyIdx);
            backend.device.presentQueue.familyIndex  = u32(queueIndices[i].presentFamilyIdx);

            backend.device.physicalDevice = pdevice;
            backend.device.properties = core::move(properties);
            backend.device.memoryProperties = core::move(memoryProperties);
            backend.device.features = core::move(features);
            backend.device.swapchainSupportInfo = core::move(swapChainInfo);
            backend.device.depthFormat = depthFormat;
        }

        logInfoTagged(LogTag::T_RENDERER, "COMPATIBILITY Table for device \"%s\" :", properties.deviceName);
        logInfoTagged(LogTag::T_RENDERER, "\t| Score | Graphics | Compute | Transfer | Present |");
        logInfoTagged(LogTag::T_RENDERER, "\t|   %.2u  |    %.2d    |    %.2d   |    %.2d    |   %.2d    |",
                        scores[i],
                        queueIndices[i].graphicsFamilyIdx,
                        queueIndices[i].computeFamilyIdx,
                        queueIndices[i].transferFamilyIdx,
                        queueIndices[i].presentFamilyIdx);
    }

    if (highestScore == 0) {
        logErrTagged(LogTag::T_RENDERER, "No suitable physical device found.");
        return false;
    }
    if (highestScore < LOW_SCORE) {
        logWarnTagged(LogTag::T_RENDERER, "Selected device has a low compatibility score (%u).", highestScore);
    }

    auto& selectedDevice = backend.device;

    logInfoTagged(LogTag::T_RENDERER, "Selected physical device: %s", selectedDevice.properties.deviceName);
    logInfoTagged(LogTag::T_RENDERER, "GPU Driver version: %d.%d.%d",
                  VK_VERSION_MAJOR(selectedDevice.properties.driverVersion),
                  VK_VERSION_MINOR(selectedDevice.properties.driverVersion),
                  VK_VERSION_PATCH(selectedDevice.properties.driverVersion));
    logInfoTagged(LogTag::T_RENDERER, "Vulkan API version: %d.%d.%d",
                  VK_VERSION_MAJOR(selectedDevice.properties.apiVersion),
                  VK_VERSION_MINOR(selectedDevice.properties.apiVersion),
                  VK_VERSION_PATCH(selectedDevice.properties.apiVersion));

    logInfoTagged(LogTag::T_RENDERER, "Device memory properties:");
    for (addr_size i = 0; i < selectedDevice.memoryProperties.memoryHeapCount; ++i) {
        const VkMemoryHeap& heap = selectedDevice.memoryProperties.memoryHeaps[i];
        f32 memorySizeGB = f32(heap.size) / f32(core::GIGABYTE);
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            logInfoTagged(LogTag::T_RENDERER, "Device Local GPU memory: %.2f GB", f64(memorySizeGB));
        }
        else {
            logInfoTagged(LogTag::T_RENDERER, "Host Shared System memory: %.2f GB", f64(memorySizeGB));
        }
    }

    return true;
}

bool createLogicalDevice(RendererBackend& backend, const char** requiredDeviceExts, addr_size requiredDeviceExtsLen) {
    constexpr addr_size MAX_QUEUE_FAMILIES = 4;
    core::SArr<u32, MAX_QUEUE_FAMILIES> uniqueQueueFamilies;

    auto isUniqueQueueFamily = [](u32 val, addr_off, u32 curr) { return curr == val; };

    if (backend.device.graphicsQueue.familyIndex != core::MAX_U32) {
        core::appendUnique(uniqueQueueFamilies, backend.device.graphicsQueue.familyIndex, isUniqueQueueFamily);
    }
    if (backend.device.computeQueue.familyIndex != core::MAX_U32) {
        core::appendUnique(uniqueQueueFamilies, backend.device.computeQueue.familyIndex, isUniqueQueueFamily);
    }
    if (backend.device.transferQueue.familyIndex != core::MAX_U32) {
        core::appendUnique(uniqueQueueFamilies, backend.device.transferQueue.familyIndex, isUniqueQueueFamily);
    }
    if (backend.device.presentQueue.familyIndex != core::MAX_U32) {
        core::appendUnique(uniqueQueueFamilies, backend.device.presentQueue.familyIndex, isUniqueQueueFamily);
    }

    u32 queueFamiliesCount = u32(uniqueQueueFamilies.len());

    constexpr f32 queuePriority = 1.0f;
    core::Arr<VkDeviceQueueCreateInfo, RendererBackendAllocator> queueCreateInfos (queueFamiliesCount);
    for (u32 i = 0; i < queueFamiliesCount; ++i) {
        VkDeviceQueueCreateInfo& curr = queueCreateInfos[i];
        curr.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        curr.queueFamilyIndex = uniqueQueueFamilies[addr_size(i)];
        curr.queueCount = 1;
        // if (queueFamilyIndices[i] == backend.device.graphicsQueueFamilyIdx) {
        //     curr.queueCount = 2;
        // }
        curr.flags = 0;
        curr.pNext = nullptr;
        curr.pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = queueFamiliesCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExts;
    deviceCreateInfo.enabledExtensionCount = u32(requiredDeviceExtsLen);

    VK_EXPECT_OR_RETURN(
        vkCreateDevice(backend.device.physicalDevice, &deviceCreateInfo, backend.allocator, &backend.device.logicalDevice),
        "Failed to create logical device."
    );
    logInfoTagged(LogTag::T_RENDERER, "Logical device created.");

    logInfoTagged(LogTag::T_RENDERER, "Obtain graphics queue.");
    vkGetDeviceQueue(backend.device.logicalDevice,
                     backend.device.graphicsQueue.familyIndex,
                     0,
                     &backend.device.graphicsQueue.handle);

    logInfoTagged(LogTag::T_RENDERER, "Obtain transfer queue.");
    vkGetDeviceQueue(backend.device.logicalDevice,
                     backend.device.transferQueue.familyIndex,
                     0,
                     &backend.device.transferQueue.handle);

    logInfoTagged(LogTag::T_RENDERER, "Obtain present queue.");
    vkGetDeviceQueue(backend.device.logicalDevice,
                     backend.device.presentQueue.familyIndex,
                     0,
                     &backend.device.presentQueue.handle);

    logInfoTagged(LogTag::T_RENDERER, "Obtain compute queue.");
    vkGetDeviceQueue(backend.device.logicalDevice,
                     backend.device.computeQueue.familyIndex,
                     0,
                     &backend.device.computeQueue.handle);

    return true;
}

bool verifySwapchainSupportForExtensions(VkPhysicalDevice pdevice,
                                         const char** requiredDeviceExts,
                                         addr_size requiredDeviceExtsLen) {

    logInfoTagged(LogTag::T_RENDERER, "Verifying required extensions for device's swapchain are supported...");

    u32 availableExtentionsCount = 0;
    VK_EXPECT_OR_RETURN(
        vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &availableExtentionsCount, nullptr),
        "Failed to get surface present modes."
    );

    core::Arr<VkExtensionProperties, RendererBackendAllocator> availableDeviceExtensions (availableExtentionsCount);
    VK_EXPECT_OR_RETURN(
        vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &availableExtentionsCount, availableDeviceExtensions.data()),
        "Failed to get surface present modes."
    );

    for (addr_size i = 0; i < requiredDeviceExtsLen; ++i) {
        bool found = false;
        const char* currExt = requiredDeviceExts[i];
        addr_size currExtLen = core::cptrLen(currExt);
        for (addr_size j = 0; j < availableExtentionsCount; ++j) {
            if (core::cptrEq(currExt, availableDeviceExtensions[j].extensionName, currExtLen)) {
                logInfoTagged(LogTag::T_RENDERER, "\tRequired extension %s SUPPORTED", currExt);
                found = true;
                break;
            }
        }
        if (!found) {
            logErrTagged(LogTag::T_RENDERER, "\tRequired extension %s NOT SUPPORTED", currExt);
            return false;
        }
    }

    logInfoTagged(LogTag::T_RENDERER, "Required extensions supported by device's swapchain.");
    return true;
}

void giveCompatibilityScoreForDevice(VkPhysicalDevice pdevice,
                                     VkSurfaceKHR surface,
                                     const VulkanPhysicalDeviceRequirements& requirements,
                                     const VkPhysicalDeviceProperties& properties,
                                     const VkPhysicalDeviceFeatures& features,
                                     const VkPhysicalDeviceMemoryProperties&,
                                     u32& outScore,
                                     QueueIndices& outIndices,
                                     VkFormat& outDepthFormat) {
    outScore = 0;
    outIndices = UNSET_QUEUE_INDICES;
    outDepthFormat = VK_FORMAT_UNDEFINED;

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        logInfoTagged(LogTag::T_RENDERER, "Device has discrete GPU.");
        outScore += requirements.descreteGPU.scoreWeight;
    }
    else {
        logInfoTagged(LogTag::T_RENDERER, "Device does NOT have a discrete GPU.");
        if (requirements.descreteGPU.isRequired) {
            outScore = 0;
            outIndices = UNSET_QUEUE_INDICES;
            return;
        }
    }

    if (features.samplerAnisotropy == VK_TRUE) {
        logInfoTagged(LogTag::T_RENDERER, "Device supports anisotropy.");
        outScore += requirements.anisotropy.scoreWeight;
    }
    else {
        logInfoTagged(LogTag::T_RENDERER, "Device does NOT support anisotropy.");
        if (requirements.anisotropy.isRequired) {
            outScore = 0;
            outIndices = UNSET_QUEUE_INDICES;
            return;
        }
    }

    if (vulkanDeviceDetectDepthFormat(pdevice, outDepthFormat)) {
        logInfoTagged(LogTag::T_RENDERER, "Device supports depth format: %s", depthFormatToCptr(outDepthFormat));
        outScore += requirements.depthFormat.scoreWeight;
    }
    else {
        logWarnTagged(LogTag::T_RENDERER, "Failed to detect support for the required depth format on device. Using VK_FORMAT_UNDEFINED.");
        if (requirements.depthFormat.isRequired) {
            outScore = 0;
            outIndices = UNSET_QUEUE_INDICES;
            return;
        }
    }

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0) {
        logErrTagged(LogTag::T_RENDERER, "No queue families found.");
        outScore = 0;
        outIndices = UNSET_QUEUE_INDICES;
        return;
    }
    core::Arr<VkQueueFamilyProperties, RendererBackendAllocator> queueFamilies (queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, queueFamilies.data());

    // NOTE: It's preferable to use a dedicated transfer queue family for transfer operations.
    //       This code gives a higher score to the device, if it has a dedicated transfer queue family.
    {
        // First pass to find optimal transfer queue family.

        u32 transferScore = 0;
        u32 dedicatedTransferScoreWeight = requirements.transferIsOnSeparateQueue.scoreWeight;
        for (addr_size i = 0; i < queueFamilyCount; i++) {
            VkQueueFamilyProperties queueFamily = queueFamilies[i];

            // Start the "current transfer score" assuming that the current queue is dedicated to transfer operations.
            // This value will be subtracted by the "dedicated transfer score" for every other supported queue.
            constexpr u32 queuesThatAreNotTransferCount = 3;
            u32 currTransferScore = dedicatedTransferScoreWeight * queuesThatAreNotTransferCount;

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // The queue family has graphics capability, therefore it is not dedicated to transfer operations.
                // Subtracting the dedicated transger score of this queue.
                currTransferScore -= dedicatedTransferScoreWeight;
            }

            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                // The queue family has compute capability, therefore it is not dedicated to transfer operations.
                // Subtracting the dedicated transger score of this queue.
                currTransferScore -= dedicatedTransferScoreWeight;
            }

            VkBool32 presentSupport = false;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, u32(i), surface, &presentSupport) != VK_SUCCESS) {
                logErrTagged(LogTag::T_RENDERER, "Failed to check if queue family supports presentation.");
                outScore = 0;
                outIndices = UNSET_QUEUE_INDICES;
                return;
            }
            if (presentSupport) {
                // The queue family has presentation capability, therefore it is not dedicated to transfer operations.
                // Subtracting the dedicated transger score of this queue.
                currTransferScore -= dedicatedTransferScoreWeight;
            }

            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                // The queue family has transfer capability.
                // Adding the "has transfer queue score" to the current transfer score.
                currTransferScore += requirements.transferQueue.scoreWeight;
                if (currTransferScore > transferScore) {
                    // The current transfer score is higher than the previous max.
                    // Changing the transferFamilyIdx to the current queue family index.
                    transferScore = currTransferScore;
                    outIndices.transferFamilyIdx = i32(i);
                }
            }
        }

        // Add the transfer score to the total score.
        // The transfer score is formed by adding the "dedicated transfer score" and the "has transfer queue score".
        outScore += transferScore;
    }

    // NOTE: This code will prefer queue families that are not the same as the transfer queue family.
    {
        // Second pass to find the reset of the needed queue families.

        u32 graphicsScore = 0;
        u32 computeScore = 0;
        u32 presentScore = 0;
        for (addr_size i = 0; i < queueFamilyCount; i++) {
            VkQueueFamilyProperties queueFamily = queueFamilies[i];
            u32 currGraphicsScore = requirements.graphicsQueue.scoreWeight * 2;
            u32 currComputeScore = requirements.computeQueue.scoreWeight * 2;
            u32 currPresentScore = requirements.presentQueue.scoreWeight * 2;

            if (i == addr_size(outIndices.transferFamilyIdx)) {
                // The current queue family is the same as the transfer queue family.
                // The current queue will not be preferred for graphics, compute or presentation.
                currGraphicsScore -= requirements.graphicsQueue.scoreWeight;
                currComputeScore -= requirements.computeQueue.scoreWeight;
                currPresentScore -= requirements.presentQueue.scoreWeight;
            }

            if (graphicsScore < currGraphicsScore && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                // The current queue family has graphics capability and it has a higher score than the previous max.
                graphicsScore = currGraphicsScore;
                outIndices.graphicsFamilyIdx = i32(i);
            }

            if (computeScore < currComputeScore && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                // The current queue family has compute capability and it has a higher score than the previous max.
                computeScore = currComputeScore;
                outIndices.computeFamilyIdx = i32(i);
            }

            if (presentScore < currPresentScore) {
                VkBool32 presentSupport = false;
                if (vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, u32(i), surface, &presentSupport) != VK_SUCCESS) {
                    logErrTagged(LogTag::T_RENDERER, "Failed to check if queue family supports presentation.");
                    outScore = 0;
                    outIndices = UNSET_QUEUE_INDICES;
                    return;
                }
                if (presentSupport) {
                    // The current queue family has presentation capability and it has a higher score than the previous max.
                    presentScore = currPresentScore;
                    outIndices.presentFamilyIdx = i32(i);
                }
            }
        }

        outScore += graphicsScore + computeScore + presentScore;
    }

    // Check if all required queue families were found. If not, set the score to 0.

    if (requirements.graphicsQueue.isRequired) {
        if (outIndices.graphicsFamilyIdx < 0) {
            logInfoTagged(LogTag::T_RENDERER, "Failed to find graphics queue family.");
            outScore = 0;
            outIndices = UNSET_QUEUE_INDICES;
            return;
        }
    }
    if (requirements.computeQueue.isRequired) {
        if (outIndices.computeFamilyIdx < 0) {
            logInfoTagged(LogTag::T_RENDERER, "Failed to find compute queue family.");
            outScore = 0;
            outIndices = UNSET_QUEUE_INDICES;
            return;
        }
    }
    if (requirements.transferQueue.isRequired) {
        if (outIndices.transferFamilyIdx < 0) {
            logInfoTagged(LogTag::T_RENDERER, "Failed to find transfer queue family.");
            outScore = 0;
            outIndices = UNSET_QUEUE_INDICES;
            return;
        }
    }
    if (requirements.presentQueue.isRequired) {
        if (outIndices.presentFamilyIdx < 0) {
            logInfoTagged(LogTag::T_RENDERER, "Failed to find present queue family.");
            outScore = 0;
            outIndices = UNSET_QUEUE_INDICES;
            return;
        }
    }
}

bool vulkanDeviceDetectDepthFormat(VkPhysicalDevice pdevice, VkFormat& outFormat) {
    constexpr VkFormatFeatureFlagBits flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (addr_size i = 0; i < requiredDepthFormatsLen; ++i) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(pdevice, requiredDepthFormats[i], &props);

        if ((props.linearTilingFeatures & flags) == flags) {
            outFormat = requiredDepthFormats[i];
            return true;
        }
        else if ((props.optimalTilingFeatures & flags) == flags) {
            outFormat = requiredDepthFormats[i];
            return true;
        }
    }

    return false;
}

} // namespace

} // namespace stlv
