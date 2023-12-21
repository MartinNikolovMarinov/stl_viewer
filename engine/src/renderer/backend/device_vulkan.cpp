#include <fwd_internal.h>

#include <renderer/renderer_backend.h>
#include <renderer/backend/renderer_vulkan.h>

namespace stlv {

namespace {

struct VulkanPhysicalDeviceRequirements {
    bool hasDescreteGPU;
    bool hasAnisotropy;

    bool hasGraphicsQueue;
    bool hasComputeQueue;
    bool hasTransferQueue;
    bool hasPresentQueue;
    const char** deviceExtensions;
    addr_size deviceExtensionsLen;
};

struct DeviceScore {
    u32 score;
    i32 graphicsFamilyIdx;
    i32 computeFamilyIdx;
    i32 transferFamilyIdx;
    i32 presentFamilyIdx;
};

constexpr DeviceScore ZERO_SCORE = {0, -1, -1, -1, -1};

DeviceScore giveCompatibilityScoreForDevice(VkPhysicalDevice pdevice,
                                            VkSurfaceKHR surface,
                                            const VulkanPhysicalDeviceRequirements& requirements,
                                            const VkPhysicalDeviceProperties& properties,
                                            const VkPhysicalDeviceFeatures& features,
                                            const VkPhysicalDeviceMemoryProperties&) {
    DeviceScore ret = ZERO_SCORE;

    if (requirements.hasDescreteGPU) {
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            logInfoTagged(LogTag::T_RENDERER, "\tDevice has discrete GPU.");
            ret.score++;
        }
        else {
            logInfoTagged(LogTag::T_RENDERER, "\tDevice does NOT have a discrete GPU.");
            return ZERO_SCORE;
        }
    }

    if (requirements.hasAnisotropy) {
        if (features.samplerAnisotropy == VK_TRUE) {
            logInfoTagged(LogTag::T_RENDERER, "\tDevice supports anisotropy.");
            ret.score++;
        }
        else {
            logInfoTagged(LogTag::T_RENDERER, "\tDevice does NOT support anisotropy.");
            return ZERO_SCORE;
        }
    }

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0) {
        logErrTagged(LogTag::T_RENDERER, "\tNo queue families found.");
        return ZERO_SCORE;
    }
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, queueFamilies);

    if (requirements.hasTransferQueue) {
        // NOTE: It's preferable to use a dedicated transfer queue family for transfer operations.
        //       This code gives a higher score to the device, if it has a dedicated transfer queue family.

        // First pass to find optimal transfer queue family.

        constexpr u32 dedicatedTrasnferQueueScoreWeight = 10; // Increase this, if finding a dedicated queue is more important.
        u32 transferScore = 0;
        for (addr_size i = 0; i < queueFamilyCount; i++) {
            VkQueueFamilyProperties queueFamily = queueFamilies[i];

            u32 currTransferScore = dedicatedTrasnferQueueScoreWeight * 3;

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                currTransferScore -= dedicatedTrasnferQueueScoreWeight;
            }

            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                currTransferScore -= dedicatedTrasnferQueueScoreWeight;
            }

            if (requirements.hasPresentQueue) {
                VkBool32 presentSupport = false;
                if (vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, u32(i), surface, &presentSupport) != VK_SUCCESS) {
                    logErrTagged(LogTag::T_RENDERER, "\tFailed to check if queue family supports presentation.");
                    return ZERO_SCORE;
                }
                if (presentSupport) {
                    currTransferScore -= dedicatedTrasnferQueueScoreWeight;
                }
            }

            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                currTransferScore++;
                if (currTransferScore > transferScore) {
                    transferScore = currTransferScore;
                    ret.transferFamilyIdx = i32(i);
                }
            }
        }
        ret.score += transferScore;
    }

    // Second pass to find the reset of the needed queue families.
    // This code will prefer queue families that are not the same as the transfer queue family.

    u32 graphicsScore = 0;
    u32 computeScore = 0;
    u32 presentScore = 0;
    for (addr_size i = 0; i < queueFamilyCount; i++) {
        VkQueueFamilyProperties queueFamily = queueFamilies[i];
        u32 currGraphicsScore = 2;
        u32 currComputeScore = 2;
        u32 currPresentScore = 2;

        if (i == addr_size(ret.transferFamilyIdx)) {
            currGraphicsScore--;
            currComputeScore--;
            currPresentScore--;
        }

        if (requirements.hasGraphicsQueue &&
            graphicsScore < currGraphicsScore &&
            (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        ) {
            graphicsScore = currGraphicsScore;
            ret.graphicsFamilyIdx = i32(i);
        }

        if (requirements.hasComputeQueue &&
            computeScore < currComputeScore &&
            (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        ) {
            computeScore = currComputeScore;
            ret.computeFamilyIdx = i32(i);
        }

        if (requirements.hasPresentQueue && presentScore < currPresentScore) {
            VkBool32 presentSupport = false;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, u32(i), surface, &presentSupport) != VK_SUCCESS) {
                logErrTagged(LogTag::T_RENDERER, "\tFailed to check if queue family supports presentation.");
                return ZERO_SCORE;
            }
            if (presentSupport) {
                presentScore = currPresentScore;
                ret.presentFamilyIdx = i32(i);
            }
        }
    }
    ret.score += graphicsScore + computeScore + presentScore;

    // Check that all needed queue families were found.

    if (requirements.hasGraphicsQueue) {
        if (ret.graphicsFamilyIdx < 0) {
            logInfoTagged(LogTag::T_RENDERER, "\tFailed to find graphics queue family.");
            return ZERO_SCORE;
        }
    }
    if (requirements.hasComputeQueue) {
        if (ret.computeFamilyIdx < 0) {
            logInfoTagged(LogTag::T_RENDERER, "\tFailed to find compute queue family.");
            return ZERO_SCORE;
        }
    }
    if (requirements.hasTransferQueue) {
        if (ret.transferFamilyIdx < 0) {
            logInfoTagged(LogTag::T_RENDERER, "\tFailed to find transfer queue family.");
            return ZERO_SCORE;
        }
    }
    if (requirements.hasPresentQueue) {
        if (ret.presentFamilyIdx < 0) {
            logInfoTagged(LogTag::T_RENDERER, "\tFailed to find present queue family.");
            return ZERO_SCORE;
        }
    }

    return ret;
}

bool isSwapchainSupported(VkPhysicalDevice pdevice,
                         const VulkanPhysicalDeviceRequirements& requirements,
                         const VulkanSwapchainSupportInfo& swapChainInfo) {
    if (swapChainInfo.formats.empty() || swapChainInfo.presentModes.empty()) {
        return false;
    }

    u32 availableExtentionsCount = 0;
    VK_EXPECT(
        vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &availableExtentionsCount, nullptr),
        "Failed to get surface present modes."
    );

    VkExtensionProperties availableDeviceExtensions[availableExtentionsCount];
    VK_EXPECT(
        vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &availableExtentionsCount, availableDeviceExtensions),
        "Failed to get surface present modes."
    );

    for (addr_size i = 0; i < requirements.deviceExtensionsLen; ++i) {
        bool found = false;
        const char* currExt = requirements.deviceExtensions[i];
        addr_size currExtLen = core::cptrLen(currExt);
        for (addr_size j = 0; j < availableExtentionsCount; ++j) {
            if (core::cptrEq(currExt, availableDeviceExtensions[j].extensionName, currExtLen)) {
                found = true;
                break;
            }
        }
        if (!found) {
            logErrTagged(LogTag::T_RENDERER, "\tRequired extension %s not found.", currExt);
            return false;
        }
    }

    return true;
}

bool selectPhysicalDevice(RendererBackend& backend, const char** deviceExtensions, addr_size deviceExtensionsLen) {
    u32 physicalDeviceCount = 0;
    VK_EXPECT(
        vkEnumeratePhysicalDevices(backend.instance, &physicalDeviceCount, nullptr),
        "Failed to enumerate physical devices."
    );
    if (physicalDeviceCount == 0) {
        logErrTagged(LogTag::T_RENDERER, "No physical devices on the system support Vulkan.");
        return false;
    }

    VkPhysicalDevice physicalDevices[physicalDeviceCount];
    VK_EXPECT(
        vkEnumeratePhysicalDevices(backend.instance, &physicalDeviceCount, physicalDevices),
        "Failed to enumerate physical devices."
    );

    DeviceScore scores[physicalDeviceCount];
    u32 highestScore = 0;

    VulkanPhysicalDeviceRequirements requirements = {};
    requirements.hasDescreteGPU = true;
    requirements.hasGraphicsQueue = true;
    requirements.hasComputeQueue = true;
    requirements.hasTransferQueue = true;
    requirements.hasPresentQueue = true;
    requirements.deviceExtensions = deviceExtensions;
    requirements.deviceExtensionsLen = deviceExtensionsLen;


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
        if (!vulkanDeviceQuerySwapchainSupport(pdevice, backend.surface, swapChainInfo)) {
            logErrTagged(LogTag::T_RENDERER, "\tFailed to query the device for swapchain support.");
            continue;
        }

        if (!isSwapchainSupported(pdevice, requirements, swapChainInfo)) {
            logErrTagged(LogTag::T_RENDERER, "\tSwapchain not sutable. Skipping device.");
            continue;
        }

        scores[i] = giveCompatibilityScoreForDevice(pdevice, backend.surface, requirements,
                                                    properties, features, memoryProperties);
        if (scores[i].score > highestScore) {
            highestScore = scores[i].score;

            backend.device.graphicsQueueFamilyIdx = u32(scores[i].graphicsFamilyIdx);
            backend.device.computeQueueFamilyIdx  = u32(scores[i].computeFamilyIdx);
            backend.device.transferQueueFamilyIdx = u32(scores[i].transferFamilyIdx);
            backend.device.presentQueueFamilyIdx   = u32(scores[i].presentFamilyIdx);

            backend.device.physicalDevice = pdevice;
            backend.device.properties = core::move(properties);
            backend.device.memoryProperties = core::move(memoryProperties);
            backend.device.features = core::move(features);
            backend.device.swapchainSupportInfo = core::move(swapChainInfo);
        }

        logInfoTagged(LogTag::T_RENDERER, "\tDevice COMPATIBILITY score: %u", scores[i].score);
        logInfoTagged(LogTag::T_RENDERER, "\tDevice GRAPHICS QUEUE family index: %d", scores[i].graphicsFamilyIdx);
        logInfoTagged(LogTag::T_RENDERER, "\tDevice COMPUTE QUEUE family index: %d", scores[i].computeFamilyIdx);
        logInfoTagged(LogTag::T_RENDERER, "\tDevice TRANSFER QUEUE family index: %d", scores[i].transferFamilyIdx);
        logInfoTagged(LogTag::T_RENDERER, "\tDevice PRESENT QUEUE family index: %d", scores[i].presentFamilyIdx);
    }

    if (highestScore == 0) {
        logErrTagged(LogTag::T_RENDERER, "No suitable physical device found.");
        return false;
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
            logInfoTagged(LogTag::T_RENDERER, "\tDevice Local GPU memory: %.2f GB", f64(memorySizeGB));
        }
        else {
            logInfoTagged(LogTag::T_RENDERER, "\tHost Shared System memory: %.2f GB", f64(memorySizeGB));
        }
    }

    return true;
}

} // namespace

bool vulkanDeviceCreate(RendererBackend& backend) {
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    constexpr addr_size deviceExtensionsLen = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);

    logSectionTitleInfoTagged(LogTag::T_RENDERER, "Selecting Vulkan physical device...");
    if (!selectPhysicalDevice(backend, deviceExtensions, deviceExtensionsLen)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to select physical device.");
        return false;
    }
    logSectionTitleInfoTagged(LogTag::T_RENDERER, "Physical device selected.");

    bool presentSharesGraphicsQueue = backend.device.graphicsQueueFamilyIdx == backend.device.presentQueueFamilyIdx;
    bool transferSharesGraphicsQueue = backend.device.graphicsQueueFamilyIdx == backend.device.transferQueueFamilyIdx;

    u32 queueFamiliesCount = 1;
    if (!presentSharesGraphicsQueue) queueFamiliesCount++;
    if (!transferSharesGraphicsQueue) queueFamiliesCount++;

    u32 idx = 0;
    u32 queueFamilyIndices[queueFamiliesCount];
    queueFamilyIndices[idx++] = backend.device.graphicsQueueFamilyIdx;
    if (!presentSharesGraphicsQueue) {
        queueFamilyIndices[idx++] = backend.device.presentQueueFamilyIdx;
    }
    if (!transferSharesGraphicsQueue) {
        queueFamilyIndices[idx++] = backend.device.transferQueueFamilyIdx;
    }

    VkDeviceQueueCreateInfo queueCreateInfos[queueFamiliesCount] = {};
    for (u32 i = 0; i < queueFamiliesCount; ++i) {
        VkDeviceQueueCreateInfo& curr = queueCreateInfos[i];
        curr.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        curr.queueFamilyIndex = queueFamilyIndices[i];
        curr.queueCount = 1;
        // if (queueFamilyIndices[i] == backend.device.graphicsQueueFamilyIdx) {
        //     curr.queueCount = 2;
        // }
        curr.flags = 0;
        curr.pNext = nullptr;
        f32 queuePriority = 1.0f;
        curr.pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = queueFamiliesCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceCreateInfo.enabledExtensionCount = deviceExtensionsLen;

    VK_EXPECT(
        vkCreateDevice(backend.device.physicalDevice, &deviceCreateInfo, nullptr, &backend.device.logicalDevice),
        "Failed to create logical device."
    );
    logInfoTagged(LogTag::T_RENDERER, "Logical device created.");

    logInfoTagged(LogTag::T_RENDERER, "Obtain graphics queue.");
    vkGetDeviceQueue(backend.device.logicalDevice,
                     backend.device.graphicsQueueFamilyIdx,
                     0,
                     &backend.device.graphicsQueue);

    logInfoTagged(LogTag::T_RENDERER, "Obtain transfer queue.");
    vkGetDeviceQueue(backend.device.logicalDevice,
                     backend.device.transferQueueFamilyIdx,
                     0,
                     &backend.device.transferQueue);

    logInfoTagged(LogTag::T_RENDERER, "Obtain present queue.");
    vkGetDeviceQueue(backend.device.logicalDevice,
                     backend.device.presentQueueFamilyIdx,
                     0,
                     &backend.device.presentQueue);


    return true;
}

void vulcanDeviceDestroy(RendererBackend& backend) {
    VulkanDevice& device = backend.device;

    if (device.logicalDevice) {
        vkDestroyDevice(device.logicalDevice, nullptr);
    }
}

bool vulkanDeviceQuerySwapchainSupport(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo& info) {
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

    return true;
}

bool vulkanDeviceDetectDepthFormat(VulkanDevice& device) {
    // Format candidates in order of preference.
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    constexpr addr_size formatCandidatesLen = sizeof(candidates) / sizeof(candidates[0]);

    constexpr VkFormatFeatureFlagBits flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (addr_size i = 0; i < formatCandidatesLen; ++i) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device.physicalDevice, candidates[i], &props);

        if ((props.linearTilingFeatures & flags) == flags ||
            (props.optimalTilingFeatures & flags) == flags
        ) {
            device.depthFormat = candidates[i];
            return true;
        }
    }

    return false;
}

} // namespace stlv
