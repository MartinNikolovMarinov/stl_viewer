#include <app_logger.h>
#include <platform.h>
#include <renderer.h>
#include <vulkan_device_picker.h>
#include <vulkan_include.h>
#include <vulkan_swapchain.h>

using RendererError::FAILED_TO_CREATE_VULKAN_INSTANCE;
using RendererError::FAILED_TO_GET_VULKAN_VERSION;
using RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES;
using RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES;
using RendererError::FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT;
using RendererError::FAILED_TO_CREATE_VULKAN_DEBUG_MESSENGER;
using RendererError::FAILED_TO_FIND_GPUS_WITH_VULKAN_SUPPORT;
using RendererError::FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES;
using RendererError::FAILED_TO_CREATE_LOGICAL_DEVICE;

#define FAILED_TO_GET_VULKAN_VERSION_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_GET_VULKAN_VERSION));
#define FAILED_TO_CREATE_VULKAN_INSTANCE_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_CREATE_VULKAN_INSTANCE));
#define FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES));
#define FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES));
#define FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT));
#define FAILED_TO_CREATE_VULKAN_DEBUG_MESSENGER_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_CREATE_VULKAN_DEBUG_MESSENGER));
#define FAILED_TO_FIND_GPUS_WITH_VULKAN_SUPPORT_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_FIND_GPUS_WITH_VULKAN_SUPPORT));
#define FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES));
#define FAILED_TO_CREATE_LOGICAL_DEVICE_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_CREATE_LOGICAL_DEVICE));

namespace {

using ExtPropsList = core::ArrList<VkExtensionProperties>;
using LayerPropsList = core::ArrList<VkLayerProperties>;
using GPUDeviceList = core::ArrList<GPUDevice>;

struct VulkanQueue {
    VkQueue queue = VK_NULL_HANDLE;
    i32 idx = -1;
};

#if STLV_DEBUG
constexpr bool VALIDATION_LAYERS_ENABLED = true;
#else
constexpr bool VALIDATION_LAYERS_ENABLED = false;
#endif

const char* requiredDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
constexpr addr_size requiredDeviceExtensionsLen = sizeof(requiredDeviceExtensions) / sizeof(requiredDeviceExtensions[0]);

ExtPropsList g_allSupportedInstExts;
LayerPropsList g_allSupportedInstLayers;
GPUDeviceList g_allSupportedGPUs;

VkInstance g_instance = VK_NULL_HANDLE;
VkSurfaceKHR g_surface = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT g_debugMessenger = VK_NULL_HANDLE;
const GPUDevice* g_selectedGPU = nullptr;
VkDevice g_device = VK_NULL_HANDLE;
VulkanQueue g_graphicsQueue = {};
VulkanQueue g_presentQueue = {};
Swapchain g_swapchain = {};

constexpr addr_size VERSION_BUFFER_SIZE = 255;
core::expected<AppError> getVulkanVersion(char out[VERSION_BUFFER_SIZE]);
core::expected<AppError> logVulkanVersion();

core::expected<ExtPropsList*, AppError> getAllSupportedInstExtensions(bool useCache = true);
void                                    logInstExtPropsList(const ExtPropsList& list);
bool                                    checkSupportForInstExtension(const char* extensionName);

core::expected<LayerPropsList*, AppError> getAllSupportedInstLayers(bool useCache = true);
void                                      logInstLayersList(const LayerPropsList& list);
bool                                      checkSupportForInstLayer(const char* name);

core::expected<GPUDeviceList*, AppError> getAllSupportedPhysicalDevices(VkInstance instance, bool useCache = true);
void                                     logPhysicalDevicesList(const GPUDeviceList& list);

VkResult wrap_vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger);
void wrap_vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks* pAllocator);

core::expected<VkInstance, AppError> vulkanCreateInstance(const char* appName);
VkDebugUtilsMessengerCreateInfoEXT defaultDebugMessengerInfo();
core::expected<VkDevice, AppError> vulkanCreateLogicalDevice(const PickedGPUDevice& picked, core::Memory<const char*> deviceExts);

core::expected<VkDebugUtilsMessengerEXT, AppError> vulkanCreateDebugMessenger(VkInstance instance);

} // namespace

core::expected<AppError> Renderer::init(const RendererInitInfo& info) {
    // TODO2: Verify I do not need this:
    // #ifdef OS_MAC
    //     setenv("VK_ICD_FILENAMES", "./lib/MoltenVK/sdk-1.3.296.0/MoltenVK_icd.json", 1);
    // #endif

    core::addLoggerTag(appLogTagsToCStr(VULKAN_VALIDATION_TAG));

    if (auto res = logVulkanVersion(); res.hasErr()) {
        return res;
    }

    // Query and log all supported Instance extensions
    {
        auto res = getAllSupportedInstExtensions();
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        Assert(res.value() != nullptr, "Failed sanity check");
        logInfoTagged(RENDERER_TAG, "Listing all Supported Instance Extensions");
        logInstExtPropsList(*res.value());
    }

    // Query and log all supported layers
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        auto res = getAllSupportedInstLayers();
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        Assert(res.value() != nullptr, "Failed sanity check");
        logInfoTagged(RENDERER_TAG, "Listing all Supported Instance Layers");
        logInstLayersList(*res.value());
    }

    // Create Instance
    VkInstance instance = VK_NULL_HANDLE;
    {
        auto res = vulkanCreateInstance(info.appName);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        instance = res.value();
        logInfoTagged(RENDERER_TAG, "Vulkan Instance created.");
    }

    // Create Debug Messenger
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        auto res = vulkanCreateDebugMessenger(instance);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        debugMessenger = res.value();
        logInfoTagged(RENDERER_TAG, "Vulkan Debug Messenger created.");
        g_debugMessenger = debugMessenger;
    }

    // Create KHR Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    {
        if (auto err = Platform::createVulkanSurface(instance, surface); !err.isOk()) {
            return core::unexpected(err);
        }
        logInfoTagged(RENDERER_TAG, "Vulkan Surface created");
    }

    // Log all supported Physical Devices
    {
        auto res = getAllSupportedPhysicalDevices(instance);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        Assert(res.value() != nullptr, "Failed sanity check");
        logInfoTagged(RENDERER_TAG, "BEGIN Supported Physical Devices");
        logPhysicalDevicesList(*res.value());
    }

    // Pick a suitable GPU
    PickedGPUDevice pickedDevice;
    {
        PickDeviceInfo info {};
        info.surface = surface;
        info.requiredExtensions = { requiredDeviceExtensions, requiredDeviceExtensionsLen };

        GPUDeviceList* all = core::Unpack(getAllSupportedPhysicalDevices(instance)); // This won't fail since it is cached.

        auto res = pickDevice({all->data(), all->len()}, info);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }

        pickedDevice = std::move(res.value());
        Assert(pickedDevice.gpu != nullptr, "Failed sanity check");
        logInfoTagged(RENDERER_TAG, "Selected GPU: %s", pickedDevice.gpu->props.deviceName);
    }

    // Create logical Device
    VkDevice logicalDevice;
    {
        core::Memory<const char*> deviceExts = { requiredDeviceExtensions, requiredDeviceExtensionsLen };
        auto res = vulkanCreateLogicalDevice(pickedDevice, deviceExts);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        logicalDevice = res.value();
        logInfoTagged(RENDERER_TAG, "Logical Device created");
    }

    // Retrieve the graphics queue from the new logical device
    VulkanQueue graphicsQueue;
    {
        graphicsQueue.idx = pickedDevice.graphicsQueueIdx;
        vkGetDeviceQueue(logicalDevice, u32(graphicsQueue.idx), 0, &graphicsQueue.queue);
        logInfoTagged(RENDERER_TAG, "Graphics Queue set");
    }

    // Retrieve the graphics queue from the new logical device
    VulkanQueue presentQueue;
    {
        presentQueue.idx = pickedDevice.presentQueueIdx;
        vkGetDeviceQueue(logicalDevice, u32(presentQueue.idx), 0, &presentQueue.queue);
        logInfoTagged(RENDERER_TAG, "Present Queue set");
    }

    // Create Swapchain
    Swapchain swapchain;
    {
        auto res = Swapchain::create(pickedDevice, logicalDevice, surface);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        swapchain = res.value();
        logInfoTagged(RENDERER_TAG, "Swapchain created");
    }

    g_instance = instance;
    g_surface = surface;
    g_selectedGPU = pickedDevice.gpu;
    g_device = logicalDevice;
    g_graphicsQueue = graphicsQueue;
    g_presentQueue = presentQueue;
    g_swapchain = swapchain;

    return {};
}

void Renderer::shutdown() {
    if (g_swapchain.swapchain != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying swapchain");
        vkDestroySwapchainKHR(g_device, g_swapchain.swapchain, nullptr);
        g_swapchain = {};
    }

    if (g_device != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying logical device");
        vkDestroyDevice(g_device, nullptr);
        g_device = VK_NULL_HANDLE;
        g_graphicsQueue = {};
        g_presentQueue = {};
    }

    if (g_surface != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Vulkan KHR surface");
        vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
        g_surface = VK_NULL_HANDLE;
    }

    Platform::shutdown(); // FIXME: Why does this fix the issue?

    if (g_debugMessenger != VK_NULL_HANDLE) {
        wrap_vkDestroyDebugUtilsMessengerEXT(g_instance, g_debugMessenger, nullptr);
        g_debugMessenger = VK_NULL_HANDLE;
    }

    if (g_instance != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Vulkan instance");
        vkDestroyInstance(g_instance, nullptr);
        g_instance = VK_NULL_HANDLE;
    }

    g_allSupportedInstExts.free();
    g_allSupportedInstLayers.free();
    g_allSupportedGPUs.free();
}

namespace {

core::expected<VkInstance, AppError> vulkanCreateInstance(const char* appName) {
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

    // Setup Instance Extensions
    {
        // Check required extensions
        {
            extensions.push(VK_KHR_SURFACE_EXTENSION_NAME);

            for (addr_size i = 0; i < extensions.len(); i++) {
                if (!checkSupportForInstExtension(extensions[i])) {
                    logFatalTagged(RENDERER_TAG, "Missing required extension: %s", extensions[i]);
                    return FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT_ERREXPR;
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

    // TODO2: MacOS should require VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR , but it seems to work fine without
    //        it for now. Verify that this extension is not needed.

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = u32(extensions.len());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    // Setup Validation Layers
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        const char* layerNames[1];
        bool supported = checkSupportForInstLayer("VK_LAYER_KHRONOS_validation");
        if (supported) {
            layerNames[0] = "VK_LAYER_KHRONOS_validation";
            instanceCreateInfo.enabledLayerCount = sizeof(layerNames) / sizeof(layerNames[0]);
            instanceCreateInfo.ppEnabledLayerNames = layerNames;
            logInfoTagged(RENDERER_TAG, "Enabling VK_LAYER_KHRONOS_validation layer.");

            // The following code is required to enable the debug utils extension during instance creation
            VkDebugUtilsMessengerCreateInfoEXT debugMessageInfo = defaultDebugMessengerInfo();
            instanceCreateInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugMessageInfo);
        }
        else {
            logWarnTagged(RENDERER_TAG, "VK_LAYER_KHRONOS_validation is not supported");
        }
    }

    VkInstance ret = VK_NULL_HANDLE;
    if (VkResult vres = vkCreateInstance(&instanceCreateInfo, nullptr, &ret); vres != VK_SUCCESS) {
        return FAILED_TO_CREATE_VULKAN_INSTANCE_ERREXPR;
    }

    return ret;
}

core::expected<VkDevice, AppError> vulkanCreateLogicalDevice(const PickedGPUDevice& picked, core::Memory<const char*> deviceExts) {
    constexpr float queuePriority = 1.0f;
    constexpr addr_size MAX_QUEUES = 5;

    // Extract unique indices
    core::ArrStatic<i32, MAX_QUEUES> uniqueIndices;
    {
        auto uniqueIdxFn = [](i32 v, addr_size, i32 el) { return v == el; };
        core::pushUnique(uniqueIndices, picked.graphicsQueueIdx, uniqueIdxFn);
        core::pushUnique(uniqueIndices, picked.presentQueueIdx, uniqueIdxFn);
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

    // Enable any device features you want here
    VkPhysicalDeviceFeatures deviceFeatures {};
    // For example:
    // deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos       = queueInfos.data();
    deviceCreateInfo.queueCreateInfoCount    = queueInfos.len();
    deviceCreateInfo.pEnabledFeatures        = &deviceFeatures;

    // If you need device-specific extensions (like swapchain):
    deviceCreateInfo.enabledExtensionCount   = deviceExts.len();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExts.data();

    VkDevice logicalDevice = VK_NULL_HANDLE;
    if (
        VkResult vres = vkCreateDevice(picked.gpu->device, &deviceCreateInfo, nullptr, &logicalDevice);
        vres != VK_SUCCESS
    ) {
        return FAILED_TO_CREATE_LOGICAL_DEVICE_ERREXPR;
    }

    return logicalDevice;
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

core::expected<VkDebugUtilsMessengerEXT, AppError> vulkanCreateDebugMessenger(VkInstance instance) {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = defaultDebugMessengerInfo();
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    if (VkResult vres = wrap_vkCreateDebugUtilsMessengerEXT(instance,
                                                            &debugMessengerCreateInfo,
                                                            nullptr,
                                                            &debugMessenger);
        vres != VK_SUCCESS
    ) {
        return FAILED_TO_CREATE_VULKAN_DEBUG_MESSENGER_ERREXPR;
    }

    return debugMessenger;
}

core::expected<ExtPropsList*, AppError> getAllSupportedInstExtensions(bool useCache) {
    if (useCache && !g_allSupportedInstExts.empty()) {
        return &g_allSupportedInstExts;
    }

    u32 extensionCount = 0;
    if (VkResult vres = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        vres != VK_SUCCESS) {
        return FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES_ERREXPR;
    }

    auto extList = ExtPropsList(extensionCount, VkExtensionProperties{});
    if (VkResult vres = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extList.data());
        vres != VK_SUCCESS) {
        return FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES_ERREXPR;
    }

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
    auto res = getAllSupportedInstExtensions();
    if (res.hasErr()) {
        logWarnTagged(RENDERER_TAG, "Query for all supported extensions failed, reason: %s", res.err().toCStr());
        return false;
    }

    const ExtPropsList& extensions = *res.value();
    for (addr_size i = 0; i < extensions.len(); i++) {
        if (core::memcmp(extensions[i].extensionName, extensionName, core::cstrLen(extensionName)) == 0) {
            return true;
        }
    }

    return false;
}

core::expected<LayerPropsList*, AppError> getAllSupportedInstLayers(bool useCache) {
    if (useCache && !g_allSupportedInstLayers.empty()) {
        return &g_allSupportedInstLayers;
    }

    u32 layerCount;
    if (VkResult vres = vkEnumerateInstanceLayerProperties(&layerCount, nullptr); vres != VK_SUCCESS) {
        return FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES_ERREXPR;
    }

    auto layList = LayerPropsList(layerCount, VkLayerProperties{});
    if (VkResult vres = vkEnumerateInstanceLayerProperties(&layerCount, layList.data()); vres != VK_SUCCESS) {
        return FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES_ERREXPR;
    }

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
    auto res = getAllSupportedInstLayers();
    if (res.hasErr()) {
        logWarnTagged(RENDERER_TAG, "Query for all supported layers failed, reason: %s", res.err().toCStr());
        return false;
    }

    const LayerPropsList& layers = *res.value();
    for (addr_size i = 0; i < layers.len(); i++) {
        VkLayerProperties p = layers[i];
        if (core::memcmp(p.layerName, name, core::cstrLen(name)) == 0) {
            return true;
        }
    }

    return false;
}

core::expected<GPUDeviceList*, AppError> getAllSupportedPhysicalDevices(VkInstance instance, bool useCache) {
    if (useCache && !g_allSupportedGPUs.empty()) {
        return &g_allSupportedGPUs;
    }

    u32 physDeviceCount;
    if (VkResult vres = vkEnumeratePhysicalDevices(instance, &physDeviceCount, nullptr); vres != VK_SUCCESS) {
        return FAILED_TO_FIND_GPUS_WITH_VULKAN_SUPPORT_ERREXPR;
    }

    auto physDeviceList = core::ArrList<VkPhysicalDevice>(physDeviceCount, VkPhysicalDevice{});
    if (VkResult vres = vkEnumeratePhysicalDevices(instance, &physDeviceCount, physDeviceList.data()); vres != VK_SUCCESS) {
        return FAILED_TO_FIND_GPUS_WITH_VULKAN_SUPPORT_ERREXPR;
    }

    auto gpus = GPUDeviceList(physDeviceCount, GPUDevice{});
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

core::expected<AppError> getVulkanVersion(char out[VERSION_BUFFER_SIZE]) {
    u32 version = 0;
    if(VkResult vres = vkEnumerateInstanceVersion(&version); vres != VK_SUCCESS) {
        return FAILED_TO_GET_VULKAN_VERSION_ERREXPR;
    }

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

    return {};
}

core::expected<AppError> logVulkanVersion() {
    char buff[VERSION_BUFFER_SIZE];
    if (auto res = getVulkanVersion(buff); res.hasErr()) {
        return res;
    }
    logInfo(buff);
    return {};
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
