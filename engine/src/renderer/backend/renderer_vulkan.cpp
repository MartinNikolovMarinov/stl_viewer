#include <fwd_internal.h>

#include <renderer/renderer_backend.h>
#include <renderer/backend/renderer_vulkan.h>
#include <renderer/backend/platform_vulkan.h>
#include <platform/platform.h>

namespace stlv {

namespace {

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

    for (addr_size i = 0; i < extNames.len(); ++i) {
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

    for (addr_size i = 0; i < layersCount; ++i) {
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

void createCommandBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan graphics command buffers...");

    if (backend.graphicsCmdBuffers.empty()) {
        backend.graphicsCmdBuffers.fill(VulkanCommandBuffer{}, 0, backend.swapchain.imageCount);
    }

    for (addr_size i = 0; i < backend.graphicsCmdBuffers.len(); ++i) {
        VulkanCommandBuffer& cmdBuffer = backend.graphicsCmdBuffers[i];
        if (cmdBuffer.handle) {
            vkFreeCommandBuffers(backend.device.logicalDevice,
                                backend.device.graphicsCmdPool, 1,
                                &cmdBuffer.handle);
        }
        cmdBuffer = {};
        vulkanCommandBufferAllocate(backend,
                                    backend.device.graphicsCmdPool,
                                    true,
                                    cmdBuffer);
    }
}

void destroyCommandBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan graphics command buffers...");

    for (addr_size i = 0; i < backend.graphicsCmdBuffers.len(); ++i) {
        VulkanCommandBuffer& cmdBuffer = backend.graphicsCmdBuffers[i];
        vulkanCommandBufferFree(backend, backend.device.graphicsCmdPool, cmdBuffer);
    }
}

void regenerateFrameBuffers(RendererBackend& backend, VulkanSwapchain& swapchain, VulkanRenderPass* renderpass) {
    logInfoTagged(LogTag::T_RENDERER, "Regenerating Vulkan frame buffers...");

    for (u32 i = 0; i < swapchain.imageCount; i++) {
        u32 attachmentCount = 2;
        VkImageView attachments[attachmentCount] = {
            swapchain.imageViews[i],       // color attachment
            swapchain.depthAttachment.view // depth attachment
        };

        VulkanFrameBuffer& frameBuffer = swapchain.frameBuffers[i];
        vulkanFrameBufferCreate(backend, *renderpass,
                                backend.framebufferWidth, backend.framebufferHeight,
                                attachments, attachmentCount,
                                frameBuffer);
    }
}

void destroyFrameBuffers(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan frame buffers...");

    for (u32 i = 0; i < backend.swapchain.imageCount; i++) {
        VulkanFrameBuffer& frameBuffer = backend.swapchain.frameBuffers[i];
        vulkanFrameBufferDestroy(backend, frameBuffer);
    }
}

} // namespace

extern core::tuple<u32, u32> getAppFrameBufferSize();

bool initRendererBE(RendererBackend& backend, PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight) {
    logInfoTagged(LogTag::T_RENDERER, "Initializing renderer backend.");

    backend = {};
    backend.framebufferWidth = frameBufferWidth;
    backend.framebufferHeight = frameBufferHeight;

    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.pApplicationName = "STLV";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "STLV Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);

    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

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

    instanceInfo.enabledExtensionCount = u32(requiredExtensions.len());
    instanceInfo.ppEnabledExtensionNames = requiredExtensions.data();

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

    instanceInfo.enabledLayerCount = u32(requiredValidationLayersLen);
    instanceInfo.ppEnabledLayerNames = requiredValidationLayers;

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
    instanceInfo.pNext = &validationFeatures;
    validationFeatures.pNext = &debugMessengerInfo;
#else
    logInfoTagged(LogTag::T_RENDERER, "Vulkan validation layers DISABLED.");
#endif

    VK_EXPECT_OR_RETURN(
        vkCreateInstance(&instanceInfo, backend.allocator, &backend.instance),
        "Failed to create Vulkan instance."
    );
    logInfoTagged(LogTag::T_RENDERER, "Vulkan Instance created.");

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

    if (!pltCreateVulkanSurface_vulkan(pltState, backend)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create window surface.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Vulkan surface created.");

    if (!vulkanDeviceCreate(backend)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create Vulkan device.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Vulkan device created.");

    logSectionTitleInfoTagged(LogTag::T_RENDERER, "Vulkan swapchain initial creation...");
    vulkanSwapchainCreate(backend, backend.framebufferWidth, backend.framebufferHeight, backend.swapchain);

    constexpr core::vec4f clearColor = core::v(0.0f, 0.0f, 0.4f, 1.0f);
    vulkanRenderpassCreate(backend, backend.mainRenderPass,
                           0, 0, f32(backend.framebufferWidth), f32(backend.framebufferHeight),
                           clearColor, 1.0f, 0);
    logInfoTagged(LogTag::T_RENDERER, "Vulkan renderpass created.");

    backend.swapchain.frameBuffers = reinterpret_cast<VulkanFrameBuffer*>(
        RendererBackendAllocator::alloc(sizeof(VulkanFrameBuffer) * backend.swapchain.imageCount));
    regenerateFrameBuffers(backend, backend.swapchain, &backend.mainRenderPass);
    logInfoTagged(LogTag::T_RENDERER, "Vulkan initial frame buffers created.");

    createCommandBuffers(backend);
    logInfoTagged(LogTag::T_RENDERER, "Vulkan command buffers created.");

    return true;
}

void shutdownRendererBE(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Shutting down renderer backend.");

    destroyFrameBuffers(backend);

    destroyCommandBuffers(backend);

    vulkanRenderpassDestroy(backend, backend.mainRenderPass);

    vulkanSwapchainDestroy(backend, backend.swapchain);

    vulcanDeviceDestroy(backend);

#if STLV_DEBUG
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan debug messenger.");
    if (backend.debugMessenger) {
        call_vkDestroyDebugUtilsMessengerEXT(backend.instance, backend.debugMessenger, backend.allocator);
    }
#endif

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan surface.");
    if (backend.surface) {
        vkDestroySurfaceKHR(backend.instance, backend.surface, backend.allocator);
    }

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan instance.");
    if (backend.instance) {
        vkDestroyInstance(backend.instance, backend.allocator);
    }
}

bool beginFrameRendererBE(RendererBackend&, f64) {
    return true;
}

bool endFrameRendererBE(RendererBackend&, f64) {
    return true;
}

#if STLV_DEBUG

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

i32 RendererBackend::findMemoryTypeIndex(u32 memoryTypeBits, VkMemoryPropertyFlags memoryFlags) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & memoryFlags) == memoryFlags) {
            return i32(i);
        }
    }

    return -1;
}

} // namespace stlv
