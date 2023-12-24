#include <application/logger.h>
#include <platform/platform.h>
#include <renderer/backends/vulkan/vulkan_backend.h>
#include <renderer/backends/vulkan/vulkan_platform.h>

namespace stlv {

struct PlatformState;
struct RendererBackend;

namespace {

u32 g_cachedFrameBufferWidth = 0;
u32 g_cachedFrameBufferHeight = 0;

// Instance

bool createVulkanInstance(RendererBackend& backend);
void destroyVulkanInstance(RendererBackend& backend);
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

// Surface

bool createVulkanSurface(RendererBackend& backend, PlatformState& pltState);
void destroyVulkanSurface(RendererBackend& backend);

// Device

bool createVulkanDevice(RendererBackend& backend);
void destroyVulkanDevice(RendererBackend& backend);

// Swapchain

bool createVulkanSwapchain(RendererBackend& backend);
void destroyVulkanSwapchain(RendererBackend& backend);

// Render Pass
bool createVulkanRenderPass(RendererBackend& backend);
void destroyVulkanRenderPass(RendererBackend& backend);

// Command Buffers
bool createCommandBuffers(RendererBackend& backend);
void destroyCommandBuffers(RendererBackend& backend);

// Frame Buffers

void regenerateFramebuffers(RendererBackend& backend, VulkanSwapchain& swapchain, VulkanRenderPass& renderPass);
void destroyFramebuffers(RendererBackend& backend, VulkanSwapchain& swapchain);

// Sync Objects

bool createSyncObjects(RendererBackend& backend);
void destroySyncObjects(RendererBackend& backend, VulkanSwapchain& swapchain, bool waitDeviceIdle);

} // namespace

bool initRendererBackend(RendererBackend& backend,
                         PlatformState& pltState,
                         u32 frameBufferWidth,
                         u32 frameBufferHeight) {
    backend = {};
    backend.frameBufferWidth = frameBufferWidth != 0 ? frameBufferWidth : 800;
    backend.frameBufferHeight = frameBufferHeight != 0 ? frameBufferHeight : 600;
    g_cachedFrameBufferHeight = 0;
    g_cachedFrameBufferWidth = 0;

    if (!createVulkanInstance(backend))          return false;
    if (!createVulkanSurface(backend, pltState)) return false;
    if (!createVulkanDevice(backend))            return false;
    if (!createVulkanSwapchain(backend))         return false;
    if (!createVulkanRenderPass(backend))        return false;

    backend.swapchain.frameBuffers = reinterpret_cast<VulkanFrameBuffer*>(
        RendererBackendAllocator::alloc(sizeof(VulkanFrameBuffer) * backend.swapchain.imageCount));
    regenerateFramebuffers(backend, backend.swapchain, backend.mainRenderPass);

    if (!createCommandBuffers(backend)) return false;
    if (!createSyncObjects(backend))    return false;

    return true;
}

void shutdownRendererBackend(RendererBackend& backend) {
    vkDeviceWaitIdle(backend.device.logicalDevice);

    destroySyncObjects(backend, backend.swapchain, false);
    destroyCommandBuffers(backend);
    destroyFramebuffers(backend, backend.swapchain);
    destroyVulkanRenderPass(backend);
    destroyVulkanSwapchain(backend);
    destroyVulkanDevice(backend);
    destroyVulkanSurface(backend);
#if STLV_DEBUG
    destroyDebugMessenger(backend);
#endif
    destroyVulkanInstance(backend);
}

void rendererOnResizeBackend(RendererBackend& backend, u32 width, u32 height) {
    g_cachedFrameBufferHeight = height;
    g_cachedFrameBufferWidth = width;
    backend.frameBufferSizeGen++;

    logTraceTagged(LogTag::T_RENDERER,
                   "Vulkan renderer Resized: w/h/gen: %d/%d/%llu",
                    width, height, backend.frameBufferSizeGen);
}

bool beginFrameRendererBackend(RendererBackend&, f64) {
    return true;
}

bool endFrameRendererBackend(RendererBackend&, f64) {
    return true;
}

i32 RendererBackend::findMemoryTypeIndex(u32 typeFilter, u32 propertyFlags) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(this->device.physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            return i32(i);
        }
    }

    logWarnTagged(LogTag::T_RENDERER, "Failed to find suitable memory type.");
    return -1;
}

namespace {

bool createVulkanInstance(RendererBackend& backend) {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "STLV";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "STLV";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    ExtensionNames requiredExtensions;

    requiredExtensions.append(VK_KHR_SURFACE_EXTENSION_NAME);
    pltGetRequiredExtensionNames_vulkan(requiredExtensions);
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

    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan instance.");
    VK_EXPECT_OR_RETURN(
        vkCreateInstance(&instanceCreateInfo, backend.allocator, &backend.instance),
        "Failed to create Vulkan instance."
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

    logInfoTagged(LogTag::T_RENDERER, "Vulkan instance created.");
    return true;
}

void destroyVulkanInstance(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan instance.");
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
    VkExtensionProperties allSupportedExt[allSupportedExtCount];
    VK_EXPECT_OR_RETURN(
        vkEnumerateInstanceExtensionProperties(nullptr, &allSupportedExtCount, allSupportedExt),
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
    VkLayerProperties availableLayers[availableLayersCount];
    VK_EXPECT_OR_RETURN(
        vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers),
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
    pbuff = core::cptrCopyUnsafe(pbuff, ANSI_BOLD(ANSI_BRIGHT_MAGENTA("[VULKAN MESSAGE]")));
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
            logTraceTagged(LogTag::T_RENDERER, buff);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            logInfoTagged(LogTag::T_RENDERER, buff);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            logWarnTagged(LogTag::T_RENDERER, buff);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            logErrTagged(LogTag::T_RENDERER, buff);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: [[fallthrough]];
        default:
            logErrTagged(LogTag::T_RENDERER, buff);
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

bool createVulkanSurface(RendererBackend& backend, PlatformState& pltState) {
    if (!pltCreateVulkanSurface_vulkan(pltState, backend)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create window surface.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Vulkan surface created.");
    return true;
}

void destroyVulkanSurface(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan surface.");
    if (backend.surface) {
        vkDestroySurfaceKHR(backend.instance, backend.surface, backend.allocator);
    }
}

// Device

bool createVulkanDevice(RendererBackend& backend) {
    if (!vulkanDeviceCreate(backend)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create Vulkan device.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Vulkan device created.");

    return true;
}

void destroyVulkanDevice(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan device.");
    vulkanDeviceDestroy(backend);
}

// Swapchain

bool createVulkanSwapchain(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan swapchain.");
    vulkanSwapchainCreate(backend, g_cachedFrameBufferWidth, g_cachedFrameBufferHeight, backend.swapchain);
    return true;
}

void destroyVulkanSwapchain(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan swapchain.");
    vulkanSwapchainDestroy(backend, backend.swapchain);
}

// Render Pass

bool createVulkanRenderPass(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan main render pass.");
    constexpr core::vec4 clearColor = core::v(0.0f, 0.3f, 0.4f, 1.0f);
    vulkanRenderPassCreate(backend, backend.mainRenderPass,
                           0, 0, f32(backend.frameBufferWidth), f32(backend.frameBufferHeight),
                           clearColor,
                           1.0f, 0);
    return true;
}

void destroyVulkanRenderPass(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan main render pass.");
    vulkanRenderPassDestroy(backend, backend.mainRenderPass);
}

// Command Buffers

bool createCommandBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan command buffers.");

    if (backend.graphicsCmdBuffers.empty()) {
        backend.graphicsCmdBuffers = VulkanCommandBufferList (backend.swapchain.imageCount);
    }

    for (u32 i = 0; i < backend.swapchain.imageCount; i++) {
        if (backend.graphicsCmdBuffers[i].handle) {
            vulkanCommandBufferFree(backend,
                                    backend.device.graphicsCmdPool,
                                    backend.graphicsCmdBuffers[i]);
        }
        vulkanCommandBufferAllocate(backend,
                                    backend.device.graphicsCmdPool,
                                    true,
                                    backend.graphicsCmdBuffers[i]);
    }

    return true;
}

void destroyCommandBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan command buffers.");
     for (u32 i = 0; i < backend.swapchain.imageCount; i++) {
        if (backend.graphicsCmdBuffers[i].handle) {
            vulkanCommandBufferFree(backend, backend.device.graphicsCmdPool, backend.graphicsCmdBuffers[i]);
            backend.graphicsCmdBuffers[i].handle = VK_NULL_HANDLE;
        }
    }
}

// Frame Buffers

void regenerateFramebuffers(RendererBackend& backend, VulkanSwapchain& swapchain, VulkanRenderPass& renderPass) {
    logInfoTagged(LogTag::T_RENDERER, "Regenerating Vulkan framebuffers...");

    for (u32 i = 0; i < swapchain.imageCount; i++) {
        constexpr u32 attachmentCount = 2; // TODO: Should be configurable.
        VkImageView attachments[attachmentCount] = {
            swapchain.imageViews[i],
            swapchain.depthAttachment.view
        };

        vulkanFrameBufferCreate(backend,
                                renderPass,
                                backend.frameBufferWidth,
                                backend.frameBufferHeight,
                                attachmentCount,
                                attachments,
                                swapchain.frameBuffers[i]);
    }
}

void destroyFramebuffers(RendererBackend& backend, VulkanSwapchain& swapchain) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan framebuffers...");
    for (u32 i = 0; i < swapchain.imageCount; i++) {
        vulkanFrameBufferDestroy(backend, swapchain.frameBuffers[i]);
    }
}

// Sync Objects

bool createSyncObjects(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan sync objects...");

    backend.imageAvailableSemaphores = VkSemaphoreList(backend.swapchain.maxFramesInFlight);
    backend.renderCompleteSemaphores = VkSemaphoreList(backend.swapchain.maxFramesInFlight);
    backend.inFlightFences = reinterpret_cast<VulkanFence*>(
        RendererBackendAllocator::alloc(sizeof(VulkanFence) * backend.swapchain.maxFramesInFlight));

    for (u8 i = 0; i < backend.swapchain.maxFramesInFlight; ++i) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(backend.device.logicalDevice, &semaphoreCreateInfo, backend.allocator, &backend.imageAvailableSemaphores[i]);
        vkCreateSemaphore(backend.device.logicalDevice, &semaphoreCreateInfo, backend.allocator, &backend.renderCompleteSemaphores[i]);

        // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
        // This will prevent the application from waiting indefinitely for the first frame to render since it
        // cannot be rendered until a frame is "rendered" before it.
        vulkanFenceCreate(backend, true, backend.inFlightFences[i]);
    }

    // In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
    // because the initial state should be 0, and will be 0 when not in use. Actual fences are not owned
    // by this list.
    backend.imagesInFlight = reinterpret_cast<VulkanFence**>(
        RendererBackendAllocator::alloc(sizeof(VulkanFence*) * backend.swapchain.imageCount));
    for (u32 i = 0; i < backend.swapchain.imageCount; ++i) {
        backend.imagesInFlight[i] = 0;
    }

    logInfoTagged(LogTag::T_RENDERER, "Vulkan sync objects created.");
    return true;
}

void destroySyncObjects(RendererBackend& backend, VulkanSwapchain& swapchain, bool waitDeviceIdle) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan sync objects...");

    if (waitDeviceIdle) {
        vkDeviceWaitIdle(backend.device.logicalDevice);
    }

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan image available semaphores...");
    for (u32 i = 0; i < swapchain.maxFramesInFlight; i++) {
        if (!backend.imageAvailableSemaphores.empty()) {
            vkDestroySemaphore(backend.device.logicalDevice, backend.imageAvailableSemaphores[i], backend.allocator);
            backend.imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan render complete semaphores...");
    for (u32 i = 0; i < swapchain.maxFramesInFlight; i++) {
        if (!backend.renderCompleteSemaphores.empty()) {
            vkDestroySemaphore(backend.device.logicalDevice, backend.renderCompleteSemaphores[i], backend.allocator);
            backend.renderCompleteSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    if (backend.inFlightFences) {
        logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan in flight fences...");
        for (u32 i = 0; i < swapchain.maxFramesInFlight; i++) {
            vulkanFenceDestroy(backend, backend.inFlightFences[i]);
        }

        RendererBackendAllocator::free(backend.inFlightFences);
        backend.inFlightFences = nullptr;

        RendererBackendAllocator::free(backend.imagesInFlight);
        backend.imagesInFlight = nullptr;
    }
}

} // namespace

} // namespace stlv
