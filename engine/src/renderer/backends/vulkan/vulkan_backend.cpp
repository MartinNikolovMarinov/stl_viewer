#include <application/logger.h>
#include <renderer/renderer_backend.h>
#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

namespace {

u32 g_cachedframeBufferWidth;
u32 g_cachedframeBufferHeight;

// Instance

bool createInstance(RendererBackend& backend);
void destroyInstance(RendererBackend& backend);
bool verifyExtensionsAreSupported(const ExtensionNames& extensions);
bool verifyExtensionsAreSupported(const ExtensionNames& extNames);
bool verifyLayersAreSupported(const char** layers, addr_size layersCount);

// Debug Messenger

#if STLV_DEBUG

VkBool32 debugCallback (
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData
);
VkDebugUtilsMessengerCreateInfoEXT createDebugMessengerInfo();
void destroyDebugMessenger(RendererBackend& backend);
VkResult call_vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pMessenger);
void call_vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT messenger,
                                          const VkAllocationCallbacks* pAllocator);

#endif

// Create Surface

bool createSurface(RendererBackend& backend, PlatformState& pltState);
void destroySurface(RendererBackend& backend);

// Device

bool deviceCreate(RendererBackend& device);
void deviceDestroy(RendererBackend& backend);

// Swapchain

bool swapchainCreate(RendererBackend& backend);
void swapchainDestroy(RendererBackend& backend);
bool recreateSwapchain(RendererBackend& backend);

// Render Pass

bool renderPassesCreate(RendererBackend& backend);
void renderPassesDestroy(RendererBackend& backend);

// Command Buffers

bool createCommandBuffers(RendererBackend& backend);
void destroyCommandBuffers(RendererBackend& backend);

// Frame Buffers

bool createFrameBuffers(RendererBackend& backend);
bool regenerateFrameBuffers(RendererBackend& backend, VulkanSwapchain& swapchain, VulkanRenderPass& renderPass);
void destroyFrameBuffers(RendererBackend& backend);

// Sync Objects

bool createSyncObjects(RendererBackend& backend);
void destroySyncObjects(RendererBackend& backend);

} // namespace

i32 RendererBackend::findMemoryIndex(u32 typeFilter, u32 propertyFlags) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);

    for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            return i32(i);
        }
    }

    return -1;
}

bool initRendererBackend(RendererBackend& backend, PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight) {
    logInfoTagged(LogTag::T_RENDERER, "Initializing Vulkan Backend...");

    backend.frameBufferWidth = frameBufferWidth;
    backend.frameBufferHeight = frameBufferHeight;
    backend.frameBufferSizeGen = 0;
    backend.frameBufferSizeLastGen = 0;
    g_cachedframeBufferHeight = 0;
    g_cachedframeBufferWidth = 0;
    backend.recreatingSwapchain = false;

    if (!createInstance(backend)) return false;
    if (!createSurface(backend, pltState)) return false;
    if (!deviceCreate(backend)) return false;
    if (!swapchainCreate(backend)) return false;
    if (!renderPassesCreate(backend)) return false;
    if (!createFrameBuffers(backend)) return false;
    if (!createCommandBuffers(backend)) return false;
    if (!createSyncObjects(backend)) return false;

    logInfoTagged(LogTag::T_RENDERER, "Vulkan Backend Initialized SUCCESSFULLY.");
    return true;
}

void shutdownRendererBackend(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Renderer Vulkan Backend Shutting Down...");

    VK_EXPECT(vkDeviceWaitIdle(backend.device.logicalDevice), "Failed to wait for Vulkan device to become idle.");

    destroySyncObjects(backend);
    destroyCommandBuffers(backend);
    destroyFrameBuffers(backend);
    renderPassesDestroy(backend);
    swapchainDestroy(backend);
    deviceDestroy(backend);
    destroySurface(backend);
#if STLV_DEBUG
    destroyDebugMessenger(backend);
#endif
    destroyInstance(backend);

    backend = {};
}

void rendererOnResizeBackend(RendererBackend& backend, u32 width, u32 height) {
    g_cachedframeBufferWidth = width;
    g_cachedframeBufferHeight = height;
    backend.frameBufferSizeGen++;

    logTraceTagged(
        LogTag::T_RENDERER,
        "Renderer Vulkan Backend Resizing to %u/%u/%llu",
        width, height, backend.frameBufferSizeGen);
}

bool beginFrameRendererBackend(RendererBackend& backend, f64) {
    VulkanDevice& device = backend.device;

    if (backend.recreatingSwapchain) {
        VK_EXPECT_OR_RETURN(
            vkDeviceWaitIdle(device.logicalDevice),
            "Failed to wait for Vulkan device to become idle (1)."
        );
        logTraceTagged(LogTag::T_RENDERER, "Recreating Vulkan swapchain.");
        return false;
    }

    if (backend.frameBufferSizeGen != backend.frameBufferSizeLastGen) {
        VK_EXPECT_OR_RETURN(
            vkDeviceWaitIdle(device.logicalDevice),
            "Failed to wait for Vulkan device to become idle (2)."
        );

        if (!recreateSwapchain(backend)) {
            return false;
        }

        return false;
    }

    // Wait for the execution of the current frame to complete.
    if (!vulkanFenceWait(backend, backend.inFlightFences[backend.currentFrame], core::MAX_U64)) {
        logErrTagged(LogTag::T_RENDERER, "In flight fence failed to wait.");
        return false;
    }

    if (!vulkanSwapchainAcquireNextImageIdx(
            backend,
            backend.swapchain,
            core::MAX_U64,
            backend.imageAvailableSemaphores[backend.currentFrame],
            0, backend.imageIdx)
    ) {
        logErrTagged(LogTag::T_RENDERER, "Failed to acquire Vulkan swapchain image.");
        return false;
    }

    VulkanCommandBuffer& cmdBuffer = backend.graphicsCommandBuffers[backend.imageIdx];
    if (!vulkanCommandBufferReset(cmdBuffer)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to reset Vulkan command buffer.");
        return false;
    }
    if (!vulkanCommandBufferBegin(cmdBuffer, false, false, false)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to begin Vulkan command buffer.");
        return false;
    }

    // Commands
    {
        // TODO: Think about how to setup the viewport to be like OpenGL and not trigger validation errors.
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = f32(backend.frameBufferWidth);
        viewport.height = f32(backend.frameBufferHeight);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor;
        scissor.offset = { 0, 0 };
        scissor.extent = { backend.frameBufferWidth, backend.frameBufferHeight };

        vkCmdSetViewport(cmdBuffer.handle, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuffer.handle, 0, 1, &scissor);
    }

    auto& frameBuffer = backend.swapchain.frameBuffers[backend.imageIdx];

    if (!vulkanRenderPassBegin(cmdBuffer, backend.mainRenderPass, frameBuffer.handle)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to begin Vulkan render pass.");
        return false;
    }

    return true;
}

bool endFrameRendererBackend(RendererBackend& backend, f64) {
    VulkanCommandBuffer& cmdBuffer = backend.graphicsCommandBuffers[backend.imageIdx];

    if (!vulkanRenderPassEnd(cmdBuffer, backend.mainRenderPass)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to end Vulkan render pass.");
        return false;
    }
    if (!vulkanCommandBufferEnd(cmdBuffer)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to end Vulkan command buffer.");
        return false;
    }

    if (backend.imagesInFlight[backend.imageIdx]) {
        if (!vulkanFenceWait(backend, *backend.imagesInFlight[backend.imageIdx], core::MAX_U64)) {
            logErrTagged(LogTag::T_RENDERER, "Failed to wait for Vulkan fence.");
            return false;
        }
    }

    // Mark the image fence as in use by this frame:
    backend.imagesInFlight[backend.imageIdx] = &backend.inFlightFences[backend.currentFrame];

    // Reset fence for use on next frame:
    if (!vulkanFenceReset(backend, backend.inFlightFences[backend.currentFrame])) {
        logErrTagged(LogTag::T_RENDERER, "Failed to reset Vulkan fence.");
        return false;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer.handle;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &backend.queueCompleteSemaphores[backend.currentFrame];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &backend.imageAvailableSemaphores[backend.currentFrame];
    VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = flags;

    VK_EXPECT_OR_RETURN(
        vkQueueSubmit(
            backend.device.graphicsQueue.handle,
            1,
            &submitInfo,
            backend.inFlightFences[backend.currentFrame].handle),
        "Failed to submit Vulkan queue."
    );

    if (!vulkanCommandBufferUpdateSubmitted(cmdBuffer)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to update Vulkan command buffer.");
        return false;
    }

    if(!vulkanSwapchainPresent(
            backend,
            backend.swapchain,
            backend.device.graphicsQueue.handle,
            backend.device.presentQueue.handle,
            backend.queueCompleteSemaphores[backend.currentFrame],
            backend.imageIdx)
    ) {
        logErrTagged(LogTag::T_RENDERER, "Failed to present Vulkan swapchain image.");
        return false;
    }

    return true;
}

namespace {

// Instance

bool createInstance(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan Instance...");

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Application Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Application Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = nullptr;

    ExtensionNames requiredExtensions;
    pltGetRequiredExtensionNames_vulkan(requiredExtensions);
#if OS_MAC
    requiredExtensions.append(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
#if STLV_DEBUG
    requiredExtensions.append(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    logInfoTagged(LogTag::T_RENDERER, "Verifying all required instance extensions are available:");
    if (!verifyExtensionsAreSupported(requiredExtensions)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to verify Vulkan instance extensions.");
        return false;
    }
    instanceCreateInfo.enabledExtensionCount = u32(requiredExtensions.len());
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

#if STLV_DEBUG
    logInfoTagged(LogTag::T_RENDERER, "Vulkan validation layers ENABLED.");

    const char* requiredValidationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    constexpr addr_size requiredValidationLayersLen = sizeof(requiredValidationLayers) / sizeof(const char*);

    logInfoTagged(LogTag::T_RENDERER, "Verifying all required layers are available:");
    if (!verifyLayersAreSupported(requiredValidationLayers, requiredValidationLayersLen)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to verify Vulkan validation layers.");
        return false;
    }

    instanceCreateInfo.enabledLayerCount = u32(requiredValidationLayersLen);
    instanceCreateInfo.ppEnabledLayerNames = requiredValidationLayers;

    VkValidationFeatureEnableEXT enabledFeatures[] = {
        // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };
    constexpr addr_size enabledFeaturesLen = sizeof(enabledFeatures) / sizeof(VkValidationFeatureEnableEXT);

    // VkValidationFeatureDisableEXT disabledFeatures[] = {
    //     // VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT
    //     // VK_VALIDATION_FEATURE_DISABLE_ALL_EXT
    // };
    // constexpr addr_size disabledFeaturesLen = sizeof(disabledFeatures) / sizeof(VkValidationFeatureDisableEXT);

    VkValidationFeaturesEXT validationFeatures = {};
    validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.disabledValidationFeatureCount = 0; // u32(disabledFeaturesLen);
    validationFeatures.enabledValidationFeatureCount = u32(enabledFeaturesLen);
    validationFeatures.pDisabledValidationFeatures = nullptr; // disabledFeatures;
    validationFeatures.pEnabledValidationFeatures = enabledFeatures;

    // VkDebugUtilsMessengerCreateInfoEXT is required to enable the validation layers during instance creation:
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = createDebugMessengerInfo();

    // Add the pNext chain to the instance create info:
    instanceCreateInfo.pNext = &validationFeatures;
    validationFeatures.pNext = &debugMessengerInfo;
#else
    logInfoTagged(LogTag::T_RENDERER, "Vulkan validation layers DISABLED.");
#endif

    VK_EXPECT_OR_RETURN (
        vkCreateInstance(&instanceCreateInfo, backend.allocator, &backend.instance),
        "Failed to create Vulkan Instance."
    );

#if STLV_DEBUG
    VK_EXPECT_OR_RETURN(
        call_vkCreateDebugUtilsMessengerEXT(backend.instance,
                                            &debugMessengerInfo,
                                            backend.allocator,
                                            &backend.debugMessenger),
        "Failed to create Vulkan debug messenger."
    );

    logInfoTagged(LogTag::T_RENDERER, "Vulkan debug messenger created.");
#endif

    logInfoTagged(LogTag::T_RENDERER, "Vulkan Instance Created SUCCESSFULLY.");
    return true;
}

void destroyInstance(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan Instance.");

    if (backend.instance) {
        vkDestroyInstance(backend.instance, backend.allocator);
    }
}

bool verifyExtensionsAreSupported(const ExtensionNames& extNames) {
    u32 allSupportedExtCount = 0;
    VK_EXPECT_OR_RETURN(
        vkEnumerateInstanceExtensionProperties(nullptr, &allSupportedExtCount, nullptr),
        "Failed to enumerate Vulkan instance extensions."
    );
    core::Arr<VkExtensionProperties, RendererBackendAllocator> allSupportedExt (allSupportedExtCount);
    VK_EXPECT_OR_RETURN(
        vkEnumerateInstanceExtensionProperties(nullptr, &allSupportedExtCount, allSupportedExt.data()),
        "Failed to enumerate Vulkan instance extensions."
    );

    for (addr_size i = 0; i < extNames.len(); i++) {
        const char* curr = extNames[i];
        addr_size len = core::cptrLen(curr);
        bool found = false;
        for (addr_size j = 0; j < allSupportedExtCount; ++j) {
            if (core::cptrEq(curr, allSupportedExt[j].extensionName, len)) {
                found = true;
                break;
            }
        }
        if (!found) {
            logErrTagged(LogTag::T_RENDERER, "Required Vulkan instance extension '%s' not available.", curr);
            return false;
        }
        logInfoTagged(LogTag::T_RENDERER, "\t%s SUPPORTED", curr);
    }

    return true;
}

bool verifyLayersAreSupported(const char** layers, addr_size layersCount) {
    u32 availableLayersCount = 0;
    VK_EXPECT_OR_RETURN(
        vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr),
        "Failed to enumerate Vulkan instance layers."
    );
    core::Arr<VkLayerProperties, RendererBackendAllocator> availableLayers (availableLayersCount);
    VK_EXPECT_OR_RETURN(
        vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data()),
        "Failed to enumerate Vulkan instance layers."
    );

    for (addr_size i = 0; i < layersCount; i++) {
        const char* curr = layers[i];
        addr_size len = core::cptrLen(curr);
        bool found = false;
        for (addr_size j = 0; j < availableLayersCount; ++j) {
            if (core::cptrEq(curr, availableLayers[j].layerName, len)) {
                found = true;
                break;
            }
        }
        if (!found) {
            logErrTagged(LogTag::T_RENDERER, "Required Vulkan validation layer '%s' not available.", curr);
            return false;
        }
        logInfoTagged(LogTag::T_RENDERER, "\t%s SUPPORTED", curr);
    }

    return true;
}

#if STLV_DEBUG

VkBool32 debugCallback (
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData
) {
    char buff[core::KILOBYTE*2] = {};
    char* pbuff = buff;
    pbuff = core::cptrCopyUnsafe(pbuff, "\ntype: ");

    switch (messageTypes) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            pbuff = core::cptrCopyUnsafe(pbuff, "GENERAL");
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            pbuff = core::cptrCopyUnsafe(pbuff, "VALIDATION");
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            pbuff = core::cptrCopyUnsafe(pbuff, "PERFORMANCE");
            break;
        default:
            pbuff = core::cptrCopyUnsafe(pbuff, "UNKNOWN");
            break;
    }

    pbuff = core::cptrCopyUnsafe(pbuff, ",\nmessage:");
    pbuff = core::cptrCopyUnsafe(pbuff, "\n\n");
    pbuff = core::cptrCopyUnsafe(pbuff, pCallbackData->pMessage);
    pbuff = core::cptrCopyUnsafe(pbuff, "\n");

    // Add here anything else that might be useful from pCallbackData.

    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            logTraceTagged(LogTag::T_VULKAN, buff);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            logInfoTagged(LogTag::T_VULKAN, buff);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            logWarnTagged(LogTag::T_VULKAN, buff);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            logErrTagged(LogTag::T_VULKAN, buff);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: [[fallthrough]];
        default:
            logErrTagged(LogTag::T_VULKAN, buff);
            break;
    }

    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT createDebugMessengerInfo() {
    u32 logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    VkDebugUtilsMessengerCreateInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = logSeverity;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = &debugCallback;
    info.pUserData = nullptr;
    return info;
}

void destroyDebugMessenger(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan debug messenger.");
    if (backend.debugMessenger) {
        call_vkDestroyDebugUtilsMessengerEXT(backend.instance, backend.debugMessenger, backend.allocator);
    }
}

VkResult call_vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pMessenger) {
    static PFN_vkCreateDebugUtilsMessengerEXT func = nullptr;
    if (func == nullptr) {
        func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func == nullptr) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    VkResult ret = func(instance, pCreateInfo, pAllocator, pMessenger);
    return ret;
}

void call_vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT messenger,
                                          const VkAllocationCallbacks* pAllocator) {
    static PFN_vkDestroyDebugUtilsMessengerEXT func = nullptr;
    if (func == nullptr) {
        func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func == nullptr) {
            return;
        }
    }

    func(instance, messenger, pAllocator);
}

#endif

// Surface

bool createSurface(RendererBackend& backend, PlatformState& pltState) {
    if (!pltCreateVulkanSurface_vulkan(pltState, backend)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create window surface.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Vulkan surface created.");
    return true;
}

void destroySurface(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan surface.");
    if (backend.surface) {
        vkDestroySurfaceKHR(backend.instance, backend.surface, backend.allocator);
    }
}

// Device

bool deviceCreate(RendererBackend& backend) {
    if (!vulkanDeviceCreate(backend)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create Vulkan device.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Vulkan device created.");

    return true;
}

void deviceDestroy(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan device.");
    vulkanDeviceDestroy(backend);
}

// Swapchain

bool swapchainCreate(RendererBackend& backend) {
    VulkanSwapchainCreationInfo createInfo = {};
    createInfo.width = backend.frameBufferWidth;
    createInfo.height = backend.frameBufferHeight;
    createInfo.maxFramesInFlight = 2; // FIXME: move this to AppCreateInfo
    if (!vulkanSwapchainCreate(backend, backend.swapchain, createInfo)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create Vulkan swapchain.");
        return false;
    }

    logInfoTagged(LogTag::T_RENDERER, "Vulkan swapchain created.");
    return true;
}

void swapchainDestroy(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan swapchain.");
    vulkanSwapchainDestroy(backend, backend.swapchain);
}

bool recreateSwapchain(RendererBackend& backend) {
    if (backend.recreatingSwapchain) {
        logDebugTagged(LogTag::T_RENDERER, "Already recreating Vulkan swapchain.");
        return false;
    }

    if (backend.frameBufferWidth == 0 || backend.frameBufferHeight == 0) {
        logDebugTagged(LogTag::T_RENDERER, "Vulkan swapchain cannot be created with zero width or height.");
        return false;
    }

    backend.recreatingSwapchain = true;

    VK_EXPECT_OR_RETURN(
        vkDeviceWaitIdle(backend.device.logicalDevice),
        "Failed to wait for Vulkan device to become idle (3)."
    );

    backend.imagesInFlight.fill(nullptr, 0, backend.swapchain.imageCount);

    vulkanDeviceQuerySwapchainSupport(backend.device.physicalDevice, backend.surface, backend.device.swapchainSupportInfo);
    if (!vulkanDeviceDetectDepthFormat(backend.device.physicalDevice, backend.device.depthFormat)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to detect Vulkan depth format.");
        return false;
    }

    VulkanSwapchainCreationInfo recreateInfo;
    recreateInfo.width = g_cachedframeBufferWidth;
    recreateInfo.height = g_cachedframeBufferHeight;
    recreateInfo.maxFramesInFlight = 2; // FIXME: move this to AppCreateInfo
    if (!vulkanSwapchainRecreate(backend, backend.swapchain, recreateInfo)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to recreate Vulkan swapchain.");
        return false;
    }

    backend.frameBufferWidth = g_cachedframeBufferWidth;
    backend.frameBufferHeight = g_cachedframeBufferHeight;
    backend.mainRenderPass.w = f32(backend.frameBufferWidth);
    backend.mainRenderPass.h = f32(backend.frameBufferHeight);
    backend.frameBufferSizeLastGen = backend.frameBufferSizeGen;
    g_cachedframeBufferHeight = 0;
    g_cachedframeBufferWidth = 0;

    backend.frameBufferSizeGen = backend.frameBufferSizeLastGen;

    for (u32 i = 0; i < backend.swapchain.imageCount; ++i) {
        vulkanFrameBufferDestroy(backend, backend.swapchain.frameBuffers[i]);
    }

    backend.mainRenderPass.x = 0.0f;
    backend.mainRenderPass.y = 0.0f;
    backend.mainRenderPass.w = f32(backend.frameBufferWidth);
    backend.mainRenderPass.h = f32(backend.frameBufferHeight);

    if (!regenerateFrameBuffers(backend, backend.swapchain, backend.mainRenderPass)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to regenerate Vulkan frame buffers.");
        return false;
    }

    if (!createCommandBuffers(backend)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to recreate Vulkan command buffers.");
        return false;
    }

    logInfoTagged(LogTag::T_RENDERER, "Vulkan swapchain recreated.");
    return true;
}

// Render Pass

bool renderPassesCreate(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan main render pass.");
    constexpr core::vec4f clearColor = core::v(0.0f, 0.0f, 0.4f, 1.0f);
    if (!vulkanCreateRenderPass(backend, backend.mainRenderPass,
            0, 0, f32(backend.frameBufferWidth), f32(backend.frameBufferHeight),
            1, 0, clearColor)
    ) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create Vulkan main render pass.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Vulkan main render pass created.");

    return true;
}

void renderPassesDestroy(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan render passes.");
    if (backend.mainRenderPass.handle) {
        vulkanDestroyRenderPass(backend, backend.mainRenderPass);
    }
}

// Command Buffers

bool createCommandBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan command buffers.");

    backend.graphicsCommandBuffers.fill({}, 0, backend.swapchain.imageCount);

    for (u32 i = 0; i < backend.swapchain.imageCount; ++i) {
        VulkanCommandBuffer& cmdBuffer = backend.graphicsCommandBuffers[i];
        if (cmdBuffer.handle) {
            vulkanCommandBufferFree(backend, backend.device.graphicsCommandPool, cmdBuffer);
        }
        if (!vulkanCommandBufferAllocate(backend, backend.device.graphicsCommandPool, true, cmdBuffer)) {
            logErrTagged(LogTag::T_RENDERER, "Failed to allocate Vulkan command buffer.");
            return false;
        }
    }

    logInfoTagged(LogTag::T_RENDERER, "Vulkan command buffers created.");
    return true;
}

void destroyCommandBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan command buffers.");
    for (u32 i = 0; i < backend.swapchain.imageCount; ++i) {
        VulkanCommandBuffer& cmdBuffer = backend.graphicsCommandBuffers[i];
        if (cmdBuffer.handle) {
            vulkanCommandBufferFree(backend, backend.device.graphicsCommandPool, cmdBuffer);
        }
    }
}

// Frame Buffers

bool createFrameBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan frame buffers.");

    if (!regenerateFrameBuffers(backend, backend.swapchain, backend.mainRenderPass)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to regenerate Vulkan frame buffers.");
        return false;
    }

    logInfoTagged(LogTag::T_RENDERER, "Vulkan frame buffers created.");
    return true;
}

bool regenerateFrameBuffers(RendererBackend& backend, VulkanSwapchain& swapchain, VulkanRenderPass& renderPass) {
    logTraceTagged(LogTag::T_RENDERER, "Regenerating Vulkan frame buffers.");

    backend.swapchain.frameBuffers.ensureCap(backend.swapchain.imageCount);
    for (u32 i = 0; i < swapchain.imageCount; i++) {
        constexpr u32 attachmentCount = 2; // TODO: hardcodeness.
        VkImageView& colorImageView = swapchain.imageViews[i];
        VkImageView& depthImageView = swapchain.depthImage.view;
        VkImageView attachments[attachmentCount] = { colorImageView, depthImageView };

        VulkanFrameBuffer frameBuffer;
        bool ok = vulkanFrameBufferCreate(
            backend,
            renderPass,
            backend.frameBufferWidth, backend.frameBufferHeight,
            attachmentCount, attachments,
            frameBuffer);
        if (!ok) {
            logErrTagged(LogTag::T_RENDERER, "Failed to create Vulkan frame buffer.");
            return false;
        }

        backend.swapchain.frameBuffers.append(core::move(frameBuffer));
    }

    return true;
}

void destroyFrameBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan frame buffers.");
    for (u32 i = 0; i < backend.swapchain.imageCount; ++i) {
        vulkanFrameBufferDestroy(backend, backend.swapchain.frameBuffers[i]);
    }
}

// Sync Objects

bool createSyncObjects(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan sync objects.");

    backend.imageAvailableSemaphores.fill(VK_NULL_HANDLE, 0, backend.swapchain.maxFramesInFlight);
    backend.queueCompleteSemaphores.fill(VK_NULL_HANDLE, 0, backend.swapchain.maxFramesInFlight);
    backend.inFlightFences.fill({}, 0, backend.swapchain.maxFramesInFlight);
    backend.imagesInFlight.fill(nullptr, 0, backend.swapchain.imageCount);

    for (u32 i = 0; i < backend.swapchain.maxFramesInFlight; ++i) {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_EXPECT_OR_RETURN(
            vkCreateSemaphore(
                backend.device.logicalDevice,
                &semaphoreInfo,
                backend.allocator,
                &backend.imageAvailableSemaphores[i]),
            "Failed to create Vulkan semaphore."
        );

        VK_EXPECT_OR_RETURN(
            vkCreateSemaphore(
                backend.device.logicalDevice,
                &semaphoreInfo,
                backend.allocator,
                &backend.queueCompleteSemaphores[i]),
            "Failed to create Vulkan semaphore."
        );

        VulkanFence& fence = backend.inFlightFences[i];
        if (!vulkanFenceCreate(backend, true, fence)) {
            logErrTagged(LogTag::T_RENDERER, "Failed to create Vulkan fence.");
            return false;
        }
    }

    logInfoTagged(LogTag::T_RENDERER, "Vulkan sync objects created.");
    return true;
}

void destroySyncObjects(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan sync objects.");

    logInfoTagged(LogTag::T_RENDERER, "Destroying image available semaphores.");
    for (u32 i = 0; i < backend.swapchain.maxFramesInFlight; ++i) {
        if (backend.imageAvailableSemaphores[i]) {
            vkDestroySemaphore(backend.device.logicalDevice, backend.imageAvailableSemaphores[i], backend.allocator);
        }
    }

    logInfoTagged(LogTag::T_RENDERER, "Destroying queue complete semaphores.");
    for (u32 i = 0; i < backend.swapchain.maxFramesInFlight; ++i) {
        if (backend.queueCompleteSemaphores[i]) {
            vkDestroySemaphore(backend.device.logicalDevice, backend.queueCompleteSemaphores[i], backend.allocator);
        }
    }

    logInfoTagged(LogTag::T_RENDERER, "Destroying in flight fences.");
    for (u32 i = 0; i < backend.swapchain.maxFramesInFlight; ++i) {
        vulkanFenceDestroy(backend, backend.inFlightFences[i]);
    }

    backend.imagesInFlight.clear();
}

} // namespace

} // namespace stlv

