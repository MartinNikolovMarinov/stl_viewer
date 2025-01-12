#include <app_logger.h>
#include <platform.h>
#include <renderer.h>
#include <vulkan_include.h>

namespace {
    using ExtPropsList = core::ArrList<VkExtensionProperties>;
    using LayerPropsList = core::ArrList<VkLayerProperties>;
    using GPUDeviceList = core::ArrList<Device::GPUDevice>;

#if STLV_DEBUG
    constexpr bool VALIDATION_LAYERS_ENABLED = true;
#else
    constexpr bool VALIDATION_LAYERS_ENABLED = false;
#endif

    core::Memory<const char*> getRequiredDeviceExtensions() {
        static const char* requiredExts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    #if defined(OS_MAC) && OS_MAC == 1
            VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
    #endif
        };
        constexpr addr_size len = sizeof(requiredExts) / sizeof(requiredExts[0]);
        return { requiredExts, len };
    }

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

    VkInstance vulkanCreateInstance(const char* appName);

    VkDebugUtilsMessengerEXT vulkanCreateDebugMessenger(VkInstance instance);
    VkDebugUtilsMessengerCreateInfoEXT defaultDebugMessengerInfo();
    VkResult wrap_vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger);
    void wrap_vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                             VkDebugUtilsMessengerEXT debugMessenger,
                                             const VkAllocationCallbacks* pAllocator);

} // namespace

core::expected<Device, AppError> Device::create(CreateInfo&& info) {
    Device device;

    logInfoTagged(RENDERER_TAG, "Creating Device");

    logVulkanVersion();
    logInstExtPropsList(*getAllSupportedInstExtensions());
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        logInstLayersList(*getAllSupportedInstLayers());
    }

    // Create Instance
    device.instance = vulkanCreateInstance(info.appName);
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

    // Log all supported Physical Devices
    logPhysicalDevicesList(*getAllSupportedPhysicalDevices(device.instance));

    // Pick a suitable GPU
    device.requiredDeviceExtensions = getRequiredDeviceExtensions();
    GPUDeviceList* all = getAllSupportedPhysicalDevices(device.instance);
    core::Expect(pickDevice(all->memView(), device), "Failed to pick a physical device");
    logInfoTagged(RENDERER_TAG, "Selected GPU: %s", device.physicalDeviceProps.deviceName);

    return device;
}

void Device::destroy(Device& device) {
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

VkInstance vulkanCreateInstance(const char* appName) {
    // Initialize Vulkan Instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Retrieve required extensions by the platform layer
    i32 requiredPlatformExtCount = 0;
    Platform::requiredVulkanExtsCount(requiredPlatformExtCount);
    core::ArrList<const char*> extensions (requiredPlatformExtCount, nullptr);
    Platform::requiredVulkanExts(extensions.data());

    VkFlags instanceCreateInfoFlags = 0;

    // Setup Instance Extensions
    {
        extensions.push(VK_KHR_SURFACE_EXTENSION_NAME);

        #if defined(OS_MAC) && OS_MAC == 1
            // FIXME: Some of these might be optional ??
            // Enable portability extension for MacOS.
            extensions.push(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            extensions.push(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            instanceCreateInfoFlags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        #endif

        // Check required extensions
        {
            for (addr_size i = 0; i < extensions.len(); i++) {
                if (!checkSupportForInstExtension(extensions[i])) {
                    logFatalTagged(RENDERER_TAG, "Missing required extension: %s", extensions[i]);
                    Assert(false, "Missing required extension");
                }
            }
        }

        // Check optional extensions
        if constexpr (VALIDATION_LAYERS_ENABLED) {
            if (checkSupportForInstExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
                extensions.push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                logInfoTagged(RENDERER_TAG, "Enabling optional extension: %s", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            else {
                logWarnTagged(RENDERER_TAG, "Optional extension %s is not supported", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }
    }

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.flags = instanceCreateInfoFlags;
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = u32(extensions.len());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    // Setup Validation Layers
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        const char* layerNames[1];
        bool supported = checkSupportForInstLayer("VK_LAYER_KHRONOS_validation");
                        //  checkSupportForInstLayer("VK_LAYER_LUNARG_api_dump");
        if (supported) {
            layerNames[0] = "VK_LAYER_KHRONOS_validation";
            // layerNames[1] = "VK_LAYER_LUNARG_api_dump";
            instanceCreateInfo.enabledLayerCount = sizeof(layerNames) / sizeof(layerNames[0]);
            instanceCreateInfo.ppEnabledLayerNames = layerNames;
            logInfoTagged(RENDERER_TAG, "Enabling VK_LAYER_KHRONOS_validation layer");

            // The following code is required to enable the debug utils extension during instance creation
            static VkDebugUtilsMessengerCreateInfoEXT debugMessageInfo = defaultDebugMessengerInfo();
            instanceCreateInfo.pNext = &debugMessageInfo;
        }
        else {
            logWarnTagged(RENDERER_TAG, "VK_LAYER_KHRONOS_validation is not supported");
        }
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

    auto gpus = GPUDeviceList(physDeviceCount, Device::GPUDevice{});
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

void logPhysicalDevicesList(const GPUDeviceList& list) {
    logInfoTagged(RENDERER_TAG, "Physical Devices (%llu)", list.len());
    for (addr_size i = 0; i < list.len(); i++) {
        auto& gpu = list[i];
        auto& props = gpu.props;
        // auto& features = gpu.features; // TODO2: log relevant features

        logInfoTagged(RENDERER_TAG, "");
        logInfoTagged(RENDERER_TAG, "\tDevice Name: %s", props.deviceName);
        logInfoTagged(RENDERER_TAG, "\tAPI Version: %u.%u.%u",
                      VK_VERSION_MAJOR(props.apiVersion),
                      VK_VERSION_MINOR(props.apiVersion),
                      VK_VERSION_PATCH(props.apiVersion));
        logInfoTagged(RENDERER_TAG, "\tDriver Version: %u",
                      props.driverVersion);
        logInfoTagged(RENDERER_TAG, "\tVendor ID: %u, Device ID: %u",
                      props.vendorID, props.deviceID);
        logInfoTagged(RENDERER_TAG, "\tDevice Type: %s",
                      props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "Integrated GPU" :
                      props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "Discrete GPU" :
                      props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ? "Virtual GPU" :
                      props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ? "CPU" : "Other");
    }
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

} // namespace

