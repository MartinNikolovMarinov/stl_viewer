#include <application/logger.h>
#include <renderer/renderer_backend.h>
#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

namespace {

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

// Render Pass

bool renderPassesCreate(RendererBackend& backend);
void renderPassesDestroy(RendererBackend& backend);

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

    if (!createInstance(backend)) return false;
    if (!createSurface(backend, pltState)) return false;
    if (!deviceCreate(backend)) return false;
    if (!swapchainCreate(backend)) return false;
    if (!renderPassesCreate(backend)) return false;

    logInfoTagged(LogTag::T_RENDERER, "Vulkan Backend Initialized SUCCESSFULLY.");
    return true;
}

void shutdownRendererBackend(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Renderer Vulkan Backend Shutting Down...");

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
    backend.frameBufferWidth = width;
    backend.frameBufferHeight = height;

    logTraceTagged(LogTag::T_RENDERER, "Renderer Vulkan Backend Resizing to %ux%u.", width, height);
}

bool beginFrameRendererBackend(RendererBackend&, f64) {
    return true;
}

bool endFrameRendererBackend(RendererBackend&, f64) {
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

// Render Pass

bool renderPassesCreate(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Creating Vulkan main render pass.");

    /**
     * NOTE: On load and store operations.
     *
     * VkAttachmentLoadOp - Specifies how contents of an attachment are initialized at the beginning of a subpass.
     * VkAttachmentStoreOp - Specifies how contents of an attachment are treated at the end of a subpass.
     *
     * For load operations, there are 3 options (excluding extensions):
     *  VK_ATTACHMENT_LOAD_OP_LOAD - specifies that the previous contents of the image within the render area will be
     *      preserved as the initial values.
     *  VK_ATTACHMENT_LOAD_OP_CLEAR - specifies that the contents of the image will be cleared to a uniform value.
     *  VK_ATTACHMENT_LOAD_OP_DONT_CARE - specifies that the previous contents within the area need not be preserved.
     *
     * For store operations, there are 2 options (excluding extensions):
     *  VK_ATTACHMENT_STORE_OP_STORE - specifies that the contents generated during the render pass and within the
     *      render area are written to memory.
     *  VK_ATTACHMENT_STORE_OP_DONT_CARE - specifies that the contents within the render area are not needed after
     *      rendering, and may be discarded.
    */

    /**
     * NOTE: On layout transitions.
     *
     * initialLayout - Layout to automatically transition from at the start of the render pass.
     * finalLayout - Layout to automatically transition to at the end of the render pass.
     *
     * There are a lot of image layouts to choose from, but the most importnat ones are:
     *  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - used as a color or resolve attachment in a VkFramebuffer.
     *  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR - used for presenting a swapchain image for display.
     *  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL - used as a depth/stencil attachment in a VkFramebuffer.
    */

    // Attachments

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = backend.swapchain.imageFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = backend.device.depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
    constexpr addr_size attachmentsCount = sizeof(attachments) / sizeof(VkAttachmentDescription);

    // Subpasses

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr; // TODO: add input attachments.
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr; // TODO: add preserve attachments.
    subpass.pResolveAttachments = nullptr; // TODO: add resolve attachments.

    // Subpass Dependencies

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // The src of the first subpass is an implicit subpass that refers to all commands executed before the render pass.
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Render Pass

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = u32(attachmentsCount);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_EXPECT_OR_RETURN(
        vkCreateRenderPass(backend.device.logicalDevice, &renderPassInfo, backend.allocator, &backend.mainRenderPass),
        "Failed to create Vulkan render pass."
    );

    return true;
}

void renderPassesDestroy(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan render passes.");
    if (backend.mainRenderPass) {
        vkDestroyRenderPass(backend.device.logicalDevice, backend.mainRenderPass, backend.allocator);
    }
}

} // namespace

} // namespace stlv

