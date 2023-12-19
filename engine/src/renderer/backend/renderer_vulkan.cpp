#include <fwd_internal.h>

#include <renderer/renderer_backend.h>
#include <renderer/backend/renderer_vulkan.h>
#include <renderer/backend/platform_vulkan.h>
#include <platform/platform.h>

namespace stlv {

VkBool32 debugCallback (
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData
) {
    char buff[core::KILOBYTE*2] = {};
    char* pbuff = buff;
    pbuff = core::cptrCopyUnsafe(pbuff, "[VULKAN MESSAGE] \ntype: ");

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

bool initRendererBE(RendererBackend& backend, PlatformState& pltState) {
    backend = {};

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
    defer { RendererBackendAllocator::decreaseUsedMem(requiredExtensions.byteCap()); };

    requiredExtensions.append(VK_KHR_SURFACE_EXTENSION_NAME);
    pltGetRequiredExtensionNames_vulkan(requiredExtensions);
#if STLV_DEBUG
    requiredExtensions.append(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    logInfoTagged(LogTag::T_RENDERER, "Required Vulkan extensions:");
    for (addr_size i = 0; i < requiredExtensions.len(); ++i) {
        logInfoTagged(LogTag::T_RENDERER, "\t%s", requiredExtensions[i]);
    }

    instanceInfo.enabledExtensionCount = u32(requiredExtensions.len());
    instanceInfo.ppEnabledExtensionNames = requiredExtensions.data();

#if STLV_DEBUG
    logInfoTagged(LogTag::T_RENDERER, "Vulkan validation layers ENABLED.");

    const char* requiredValidationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    constexpr addr_size requiredValidationLayersLen = sizeof(requiredValidationLayers) / sizeof(const char*);

    u32 availableLayersCount = 0;
    VK_EXPECT(
        vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr),
        "Failed to enumerate Vulkan instance layers."
    );
    VkLayerProperties availableLayers[availableLayersCount];
    VK_EXPECT(
        vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers),
        "Failed to enumerate Vulkan instance layers."
    );

    logInfoTagged(LogTag::T_RENDERER, "Verifying all required layers are available:");
    for (addr_size i = 0; i < requiredValidationLayersLen; ++i) {
        const char* curr = requiredValidationLayers[i];
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
        logInfoTagged(LogTag::T_RENDERER, "\t%s FOUND", curr);
    }

    instanceInfo.enabledLayerCount = u32(requiredValidationLayersLen);
    instanceInfo.ppEnabledLayerNames = requiredValidationLayers;
#else
    logInfoTagged(LogTag::T_RENDERER, "Vulkan validation layers DISABLED.");
#endif

    VK_EXPECT(
        vkCreateInstance(&instanceInfo, backend.allocator, &backend.instance),
        "Failed to create Vulkan instance."
    );
    logInfoTagged(LogTag::T_RENDERER, "Vulkan Instance created.");

#if STLV_DEBUG
    u32 logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo {};
    debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerInfo.messageSeverity = logSeverity;
    debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerInfo.pfnUserCallback = &debugCallback;
    debugMessengerInfo.pUserData = nullptr;
    VK_EXPECT(
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

    if (!createVulkanDevice(backend)) {
        logErrTagged(LogTag::T_RENDERER, "Failed to create Vulkan device.");
        return false;
    }
    logInfoTagged(LogTag::T_RENDERER, "Vulkan device created.");

    return true;
}

void shutdownRendererBE(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Shutting down renderer backend.");

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan device.");
    destroyVulkanDevice(backend);

#if STLV_DEBUG
    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan debug messenger.");
    call_vkDestroyDebugUtilsMessengerEXT(backend.instance, backend.debugMessenger, backend.allocator);
#endif

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan surface.");
    vkDestroySurfaceKHR(backend.instance, backend.surface, backend.allocator);

    logInfoTagged(LogTag::T_RENDERER, "Destroying Vulkan instance.");
    vkDestroyInstance(backend.instance, backend.allocator);
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

} // namespace stlv
