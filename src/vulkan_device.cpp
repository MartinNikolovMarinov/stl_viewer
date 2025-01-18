#include <app_logger.h>
#include <platform.h>
#include <renderer.h>
#include <vulkan_renderer.h>

namespace {

using ExtPropsList = core::ArrList<VkExtensionProperties>;
using LayerPropsList = core::ArrList<VkLayerProperties>;
using GPUDeviceList = core::ArrList<Device::PhysicalDevice>;

#if STLV_DEBUG
constexpr bool VALIDATION_LAYERS_ENABLED = true;
#else
constexpr bool VALIDATION_LAYERS_ENABLED = false;
#endif

ExtPropsList g_allSupportedInstExts;
LayerPropsList g_allSupportedInstLayers;
GPUDeviceList g_allSupportedGPUs;

constexpr addr_size VERSION_BUFFER_SIZE = 255;
void getVulkanVersion(char out[VERSION_BUFFER_SIZE]);
void logVulkanVersion();

ExtPropsList* getAllSupportedInstExtensions(bool useCache = true);
void          logInstExtPropsList(const ExtPropsList& list);
bool          checkSupportForInstExtension(const char* extensionName);

LayerPropsList* getAllSupportedInstLayers(bool useCache = true);
void            logInstLayersList(const LayerPropsList& list);
bool            checkSupportForInstLayer(const char* name);

GPUDeviceList* getAllSupportedPhysicalDevices(VkInstance instance, bool useCache = true);
void           logPhysicalDevicesList(const GPUDeviceList& list);

VkInstance vulkanCreateInstance(const RendererInitInfo& rendererInitInfo);

VkDebugUtilsMessengerEXT vulkanCreateDebugMessenger(VkInstance instance);
VkDebugUtilsMessengerCreateInfoEXT defaultDebugMessengerInfo();
VkResult wrap_vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                            const VkAllocationCallbacks* pAllocator,
                                            VkDebugUtilsMessengerEXT* pDebugMessenger);
void wrap_vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                            VkDebugUtilsMessengerEXT debugMessenger,
                                            const VkAllocationCallbacks* pAllocator);

VkDevice vulkanCreateLogicalDevice(const RendererInitInfo& rendererInitInfo, Device& device);

} // namespace

core::expected<Device, AppError> Device::create(const RendererInitInfo& rendererInitInfo) {
    Device device;

    logInfoTagged(RENDERER_TAG, "Creating Device");

    logVulkanVersion();
    logInstExtPropsList(*getAllSupportedInstExtensions());
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        logInstLayersList(*getAllSupportedInstLayers());
    }

    // Create Instance
    device.instance = vulkanCreateInstance(rendererInitInfo);
    logInfoTagged(RENDERER_TAG, "VkInstance created");

    // Create Debug Messenger
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        VkDebugUtilsMessengerEXT debugMessenger = vulkanCreateDebugMessenger(device.instance);
        logInfoTagged(RENDERER_TAG, "VkDebugUtilsMessengerEXT created");
        device.debugMessenger = debugMessenger;
    }

    // Create a Surface
    Surface surface;
    Assert(Platform::createVulkanSurface(device.instance, surface.handle).isOk());
    logInfoTagged(RENDERER_TAG, "WSI Surface created");
    device.surface = surface;

    // Pick a suitable GPU
    device.deviceExtensions.required = rendererInitInfo.backend.vk.requiredDeviceExtensions;
    device.deviceExtensions.optional = rendererInitInfo.backend.vk.optionalDeviceExtensions;
    GPUDeviceList* all = getAllSupportedPhysicalDevices(device.instance);
    core::Expect(pickDevice(all->memView(), device), "Failed to pick a physical device");

    // Create a logical device
    device.logicalDevice = vulkanCreateLogicalDevice(rendererInitInfo, device);
    logInfoTagged(RENDERER_TAG, "Logical Device created");

    // Retrive queues
    {
        // Retrieve the Graphics Queue from the new logical device
        vkGetDeviceQueue(device.logicalDevice, u32(device.graphicsQueue.idx), 0, &device.graphicsQueue.handle);
        logInfoTagged(RENDERER_TAG, "Graphics Queue set");

        // Retrieve the Present Queue from the new logical device
        vkGetDeviceQueue(device.logicalDevice, u32(device.presentQueue.idx), 0, &device.presentQueue.handle);
        logInfoTagged(RENDERER_TAG, "Present Queue set");
    }


    return device;
}

void Device::destroy(Device& device) {
    if (device.logicalDevice != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Vulkan logical device");
        vkDestroyDevice(device.logicalDevice, nullptr);
        device.logicalDevice = VK_NULL_HANDLE;
    }

    if (device.surface.handle != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Vulkan KHR surface");
        vkDestroySurfaceKHR(device.instance, device.surface.handle, nullptr);
        device.surface.handle = VK_NULL_HANDLE;
    }

    if (device.debugMessenger != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Debug messenger");
        wrap_vkDestroyDebugUtilsMessengerEXT(device.instance, device.debugMessenger, nullptr);
        device.debugMessenger = VK_NULL_HANDLE;
    }

    if (device.instance != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Vulkan instance");
        vkDestroyInstance(device.instance, nullptr);
        device.instance = VK_NULL_HANDLE;
    }

    g_allSupportedInstExts.free();
    g_allSupportedInstLayers.free();
    g_allSupportedGPUs.free();
}

namespace {

VkInstance vulkanCreateInstance(const RendererInitInfo& rendererInitInfo) {
    // Initialize Vulkan Instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = rendererInitInfo.appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Retrieve required extensions by the platform layer
    i32 requiredPlatformExtCount = 0;
    Platform::requiredVulkanExtsCount(requiredPlatformExtCount);
    const addr_size requiredExtCount = requiredPlatformExtCount + rendererInitInfo.backend.vk.requiredInstanceExtensions.len();
    core::ArrList<const char*> extensions (requiredExtCount, nullptr);
    Platform::requiredVulkanExts(extensions.data());

    // Setup Instance Extensions
    {
        // Add Required extensions
        for (addr_size i = 0; i < rendererInitInfo.backend.vk.requiredInstanceExtensions.len(); i++) {
            const char* ex = rendererInitInfo.backend.vk.requiredInstanceExtensions[i];
            if (checkSupportForInstExtension(ex)) {
                extensions[requiredPlatformExtCount + i] = rendererInitInfo.backend.vk.requiredInstanceExtensions[i];
            }
            else {
                logFatalTagged(RENDERER_TAG, "Missing required extension: %s", ex);
                Panic(false, "Missing required extension");
            }
        }

        // Add Optional extensions
        for (addr_size i = 0; i < rendererInitInfo.backend.vk.optionalInstanceExtensions.len(); i++) {
            const char* ex = rendererInitInfo.backend.vk.optionalInstanceExtensions[i];
            if (checkSupportForInstExtension(ex)) {
                extensions.push(ex);
            }
            else {
                logWarnTagged(RENDERER_TAG, "Missing optional extension: %s", ex);
            }
        }

        logInfoTagged(RENDERER_TAG, "Enabled extensions:");
        for (addr_size i = 0; i < extensions.len(); i++) {
            const char* ex = extensions[i];
            logInfoTagged(RENDERER_TAG, "\t%s", ex);
        }
    }

    VkFlags instanceCreateInfoFlags = 0;
    #if defined(OS_MAC) && OS_MAC == 1
        // Enable portability extension for MacOS.
        instanceCreateInfoFlags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.flags = instanceCreateInfoFlags;
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = u32(extensions.len());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    // Setup Validation Layers
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        logInfoTagged(RENDERER_TAG, "Enabled layers:");
        auto& layers = rendererInitInfo.backend.vk.layers;
        for (addr_size i = 0; i < layers.len(); i++) {
            const char* layer = layers[i];
            if (checkSupportForInstLayer(layer)) {
                logInfoTagged(RENDERER_TAG, "\t%s", layer);
            }
            else {
                logErrTagged(RENDERER_TAG, "%s layer is not supported", layer);
                Panic(false, "Missing Vulkan Instance Layer"); // this should not happen
            }
        }

        instanceCreateInfo.enabledLayerCount = u32(layers.len());
        instanceCreateInfo.ppEnabledLayerNames = layers.data();

        // The following code is required to enable the debug utils extension during instance creation:
        static VkDebugUtilsMessengerCreateInfoEXT debugMessageInfo = defaultDebugMessengerInfo();
        instanceCreateInfo.pNext = &debugMessageInfo;
    }

    VkInstance instane = VK_NULL_HANDLE;
    VK_MUST(vkCreateInstance(&instanceCreateInfo, nullptr, &instane));

    return instane;
}

VkDebugUtilsMessengerCreateInfoEXT defaultDebugMessengerInfo() {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};

    debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                                               // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    debugMessengerCreateInfo.pfnUserCallback = [](
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void* pUserData
    ) -> VkBool32 {
        static constexpr addr_size DEBUG_MESSAGE_BUFFER_SIZE = 1024;
        char buffer[DEBUG_MESSAGE_BUFFER_SIZE];
        char* ptr = buffer;

        // Write message type to the buffer
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
            ptr = core::memcopy(ptr, "[GENERAL] ", core::cstrLen("[GENERAL] "));
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            ptr = core::memcopy(ptr, "[VALIDATION] ", core::cstrLen("[VALIDATION] "));
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
            ptr = core::memcopy(ptr, "[PERFORMANCE] ", core::cstrLen("[PERFORMANCE] "));
        }

        // Write callback message to the buffer
        ptr = core::memcopy(ptr, pCallbackData->pMessage, core::cstrLen(pCallbackData->pMessage));

        // Null-terminate the buffer
        *ptr = '\0';

        // Log the message with the appropriate logger
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            logErrTagged(VULKAN_VALIDATION_TAG, buffer);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            logWarnTagged(VULKAN_VALIDATION_TAG, buffer);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            logInfoTagged(VULKAN_VALIDATION_TAG, buffer);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            logDebugTagged(VULKAN_VALIDATION_TAG, buffer);
        }

        return VK_FALSE;
    };

    return debugMessengerCreateInfo;
}

VkDebugUtilsMessengerEXT vulkanCreateDebugMessenger(VkInstance instance) {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = defaultDebugMessengerInfo();
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkResult vres = wrap_vkCreateDebugUtilsMessengerEXT(instance,
                                                        &debugMessengerCreateInfo,
                                                        nullptr,
                                                        &debugMessenger);
    VK_MUST(vres);
    return debugMessenger;
}

GPUDeviceList* getAllSupportedPhysicalDevices(VkInstance instance, bool useCache) {
    if (useCache && !g_allSupportedGPUs.empty()) {
        return &g_allSupportedGPUs;
    }

    u32 physDeviceCount;
    VK_MUST(vkEnumeratePhysicalDevices(instance, &physDeviceCount, nullptr));

    auto physDeviceList = core::ArrList<VkPhysicalDevice>(physDeviceCount, VkPhysicalDevice{});
    VK_MUST(vkEnumeratePhysicalDevices(instance, &physDeviceCount, physDeviceList.data()));

    auto gpus = GPUDeviceList(physDeviceCount, Device::PhysicalDevice{});
    for (addr_size i = 0; i < physDeviceList.len(); i++) {
        auto pd = physDeviceList[i];

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(pd, &props);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(pd, &features);

        gpus[i].device = pd;
        gpus[i].props = props;
        gpus[i].features = features;
    }

    g_allSupportedGPUs = std::move(gpus);
    return &g_allSupportedGPUs;
}

ExtPropsList* getAllSupportedInstExtensions(bool useCache) {
    if (useCache && !g_allSupportedInstExts.empty()) {
        return &g_allSupportedInstExts;
    }

    u32 extensionCount = 0;
    VK_MUST(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

    auto extList = ExtPropsList(extensionCount, VkExtensionProperties{});
    VK_MUST(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extList.data()));

    g_allSupportedInstExts = std::move(extList);
    return &g_allSupportedInstExts;
}

void logInstExtPropsList(const ExtPropsList& list) {
    logInfoTagged(RENDERER_TAG, "Extensions (%llu)", list.len());
    for (addr_size i = 0; i < list.len(); i++) {
        logInfoTagged(RENDERER_TAG, "\t%s v%u", list[i].extensionName, list[i].specVersion);
    }
}

bool checkSupportForInstExtension(const char* extensionName) {
    const ExtPropsList& extensions = *getAllSupportedInstExtensions();
    for (addr_size i = 0; i < extensions.len(); i++) {
        if (core::memcmp(extensions[i].extensionName, extensionName, core::cstrLen(extensionName)) == 0) {
            return true;
        }
    }

    return false;
}

LayerPropsList* getAllSupportedInstLayers(bool useCache) {
    if (useCache && !g_allSupportedInstLayers.empty()) {
        return &g_allSupportedInstLayers;
    }

    u32 layerCount;
    VK_MUST(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

    auto layList = LayerPropsList(layerCount, VkLayerProperties{});
    VK_MUST(vkEnumerateInstanceLayerProperties(&layerCount, layList.data()));

    g_allSupportedInstLayers = std::move(layList);
    return &g_allSupportedInstLayers;
}

void logInstLayersList(const LayerPropsList& list) {
    logInfoTagged(RENDERER_TAG, "Layers (%llu)", list.len());
    for (addr_size i = 0; i < list.len(); i++) {
        logInfoTagged(RENDERER_TAG, "\tname: %s, description: %s, spec version: %u, impl version: %u",
                      list[i].layerName, list[i].description, list[i].specVersion, list[i].implementationVersion);
    }
}

bool checkSupportForInstLayer(const char* name) {
    const LayerPropsList& layers = *getAllSupportedInstLayers();
    for (addr_size i = 0; i < layers.len(); i++) {
        VkLayerProperties p = layers[i];
        if (core::memcmp(p.layerName, name, core::cstrLen(name)) == 0) {
            return true;
        }
    }

    return false;
}

void logVulkanVersion() {
    char buff[VERSION_BUFFER_SIZE];
    getVulkanVersion(buff);
    logInfoTagged(RENDERER_TAG, buff);
}

void getVulkanVersion(char out[VERSION_BUFFER_SIZE]) {
    u32 version = 0;
    VK_MUST(vkEnumerateInstanceVersion(&version));

    u32 n;

    out = core::memcopy(out, "Vulkan v", core::cstrLen("Vulkan v"));
    n = core::Unpack(core::intToCstr(VK_VERSION_MAJOR(version), out, VERSION_BUFFER_SIZE));
    out[n] = '.';
    out += n + 1;

    n = core::Unpack(core::intToCstr(VK_VERSION_MINOR(version), out, VERSION_BUFFER_SIZE));
    out[n] = '.';
    out += n + 1;

    n = core::Unpack(core::intToCstr(VK_VERSION_PATCH(version), out, VERSION_BUFFER_SIZE));
    out[n] = '\0';
    out += n;
}

VkResult wrap_vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger) {
    static PFN_vkCreateDebugUtilsMessengerEXT func = nullptr;

    if (func == nullptr) {
        func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func == nullptr) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void wrap_vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks* pAllocator) {
    static PFN_vkDestroyDebugUtilsMessengerEXT func = nullptr;

    if (func == nullptr) {
        func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func == nullptr) {
            return;
        }
    }

    func(instance, debugMessenger, pAllocator);
}

VkDevice vulkanCreateLogicalDevice(const RendererInitInfo& rendererInitInfo, Device& device) {
    constexpr float queuePriority = 1.0f;
    constexpr addr_size MAX_QUEUES = 5;

    // Extract unique indices
    core::ArrStatic<i32, MAX_QUEUES> uniqueIndices;
    {
        constexpr auto uniqueIdxFn = [](i32 v, addr_size, i32 el) { return v == el; };
        core::pushUnique(uniqueIndices, device.graphicsQueue.idx, uniqueIdxFn);
        core::pushUnique(uniqueIndices, device.graphicsQueue.idx, uniqueIdxFn);
    }

    core::ArrStatic<VkDeviceQueueCreateInfo, MAX_QUEUES> queueInfos;
    for (addr_size i = 0; i < uniqueIndices.len(); i++) {
        i32 idx = uniqueIndices[i];
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = u32(idx);
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueInfos.push(queueCreateInfo);
    }

    core::ArrList<const char*> enabledExtensions (device.deviceExtensions.required.len() +
                                                  device.deviceExtensions.optional.len());
    enabledExtensions.push(device.deviceExtensions.required);
    for (addr_size i = 0; i < device.deviceExtensions.optional.len(); i++) {
        const char* ext = device.deviceExtensions.optional[i];
        bool isActivated = device.deviceExtensions.optionalIsActive[i];
        if (isActivated) {
            enabledExtensions.push(ext);
        }
    }

    // Enable any device features you want here
    VkPhysicalDeviceFeatures deviceFeatures {};
    // For example:
    // deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos       = queueInfos.data();
    deviceCreateInfo.queueCreateInfoCount    = u32(queueInfos.len());
    deviceCreateInfo.pEnabledFeatures        = &deviceFeatures;

    // If you need device-specific extensions (like swapchain):
    deviceCreateInfo.enabledExtensionCount   = u32(enabledExtensions.len());
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

    VkDevice logicalDevice = VK_NULL_HANDLE;
    VK_MUST(vkCreateDevice(device.physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice));

    return logicalDevice;
}

} // namespace

