#include <renderer.h>
#include <vulkan_renderer.h>

RendererInitInfo RendererInitInfo::create(const char* appName, bool vSyncOn) {
#if defined(STLV_DEBUG) && STLV_DEBUG == 1
    constexpr bool VALIDATION_LAYERS_ENABLED = true;
#else
    constexpr bool VALIDATION_LAYERS_ENABLED = false;
#endif

    RendererInitInfo info = {};
    info.appName = appName;
    info.backendType = RendererBackendType::VULKAN;
    info.vSyncOn = vSyncOn;

    // Instance Extensions
    {
        static const char* requiredInstanceExtensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
        #if defined(OS_MAC) && OS_MAC == 1
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #endif
        #if defined(STLV_DEBUG) && STLV_DEBUG == 1
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #endif
        };
        info.backend.vk.requiredInstanceExtensions = {
            requiredInstanceExtensions,
            sizeof(requiredInstanceExtensions) / sizeof(requiredInstanceExtensions[0])
        };

        static const char* optionalInstanceExtensions[] = {
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        };
        info.backend.vk.optionalInstanceExtensions = {
            optionalInstanceExtensions,
            sizeof(optionalInstanceExtensions) / sizeof(optionalInstanceExtensions[0])
        };
    }

    // Device Extensions
    {
        static const char* requiredDeviceExts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        #if defined(OS_MAC) && OS_MAC == 1
            VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
        #endif
        };
        info.backend.vk.requiredDeviceExtensions = core::Memory<const char*> {
            requiredDeviceExts,
            sizeof(requiredDeviceExts) / sizeof(requiredDeviceExts[0])
        };
        info.backend.vk.optionalDeviceExtensions = {};
    }

    // Layers
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        static const char* layers[] = {
            "VK_LAYER_KHRONOS_validation",
        };
        info.backend.vk.layers = core::Memory<const char*> {
            layers,
            sizeof(layers) / sizeof(layers[0])
        };
    }

    return info;
}
