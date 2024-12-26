#include <renderer.h>
#include <vulkan_include.h>
#include <platform.h>

using RendererError::FAILED_TO_CREATE_VULKAN_INSTANCE;
using RendererError::FAILED_TO_GET_VULKAN_VERSION;
using RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES;
using RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES;
using RendererError::FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT;

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

namespace {

using ExtPropsList = core::ArrList<VkExtensionProperties>;
using LayerPropsList = core::ArrList<VkLayerProperties>;

#if STLV_DEBUG
constexpr bool VALIDATION_LAYERS_ENABLED = true;
#else
constexpr bool VALIDATION_LAYERS_ENABLED = false;
#endif

ExtPropsList g_allSupportedInstExts;
LayerPropsList g_allSupportedInstLayers;

VkInstance g_instance = VK_NULL_HANDLE;
VkSurfaceKHR g_surface = VK_NULL_HANDLE;

constexpr addr_size VERSION_BUFFER_SIZE = 255;
core::expected<AppError> getVulkanVersion(char out[VERSION_BUFFER_SIZE]);
core::expected<AppError> logVulkanVersion();

core::expected<ExtPropsList*, AppError> getAllSupportedExtensions(bool useCache = true);
void                                    logExtPropsList(const ExtPropsList& list);
bool                                    checkSupportForExtension(const char* extensionName);

core::expected<LayerPropsList*, AppError> getAllSupportedInstanceLayers(bool useCache = true);
void                                      logInstLayersList(const LayerPropsList& list);
bool                                      checkSupportForLayer(const char* name);

core::expected<VkInstance, AppError> vulkanCreateInstance(const char* appName);

} // namespace

core::expected<AppError> Renderer::init(const RendererInitInfo& info) {
    // TODO2: Verify I do not need this:
    // #ifdef OS_MAC
    //     setenv("VK_ICD_FILENAMES", "./lib/MoltenVK/sdk-1.3.296.0/MoltenVK_icd.json", 1);
    // #endif

    if (auto res = logVulkanVersion(); res.hasErr()) {
        return res;
    }

    // Query and log all supported extensions
    {
        auto res = getAllSupportedExtensions();
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        Assert(res.value() != nullptr, "Failed sanity check");
        logInfo("Listing all Supported Instance Extensions");
        logExtPropsList(*res.value());
    }

    // Query and log all supported layers
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        auto res = getAllSupportedInstanceLayers();
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        Assert(res.value() != nullptr, "Failed sanity check");
        logInfo("Listing all Supported Instance Layers");
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
        logInfo("Vulkan instance created.");
    }

    // Create KHR Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    {
        if (auto err = Platform::createVulkanSurface(instance, surface); !err.isOk()) {
            return core::unexpected(err);
        }
        logInfo("Vulkan surface created.");
    }

    g_instance = instance;
    g_surface = surface;

    return {};
}

void Renderer::shutdown() {
    if (g_surface) {
        logInfo("Destroying Vulkan KHR surface");
        vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
        g_surface = VK_NULL_HANDLE;
    }

    if (g_instance) {
        logInfo("Destroying Vulkan instance");
        vkDestroyInstance(g_instance, nullptr);
        g_instance = VK_NULL_HANDLE;
    }

    g_allSupportedInstExts.free();
    g_allSupportedInstLayers.free();
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
                if (!checkSupportForExtension(extensions[i])) {
                    logFatal("Missing required extension: %s", extensions[i]);
                    return FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT_ERREXPR;
                }
            }
        }

        // Check optional extensions
        if constexpr (VALIDATION_LAYERS_ENABLED) {
            if (checkSupportForExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
                extensions.push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                logInfo("Enabling optional extension: %s", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            else {
                logWarn("Optional extension %s is not supported", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
        bool supported = checkSupportForLayer("VK_LAYER_KHRONOS_validation");
        if (supported) {
            layerNames[0] = "VK_LAYER_KHRONOS_validation";
            instanceCreateInfo.enabledLayerCount = sizeof(layerNames) / sizeof(layerNames[0]);
            instanceCreateInfo.ppEnabledLayerNames = layerNames;
            logInfo("Enabling VK_LAYER_KHRONOS_validation layer.");
        }
        else {
            logWarn("VK_LAYER_KHRONOS_validation is not supported");
        }
    }

    VkInstance ret = VK_NULL_HANDLE;
    if (VkResult vres = vkCreateInstance(&instanceCreateInfo, nullptr, &ret); vres != VK_SUCCESS) {
        return FAILED_TO_CREATE_VULKAN_INSTANCE_ERREXPR;
    }

    return ret;
}

core::expected<ExtPropsList*, AppError> getAllSupportedExtensions(bool useCache) {
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

void logExtPropsList(const ExtPropsList& list) {
    logInfo("Extensions (%llu)", list.len());
    for (addr_size i = 0; i < list.len(); i++) {
        logInfo("\t%s v%u", list[i].extensionName, list[i].specVersion);
    }
}

bool checkSupportForExtension(const char* extensionName) {
    auto res = getAllSupportedExtensions();
    if (res.hasErr()) {
        logWarn("Query for all supported extensions failed, reason: %s", res.err().toCStr());
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

core::expected<LayerPropsList*, AppError> getAllSupportedInstanceLayers(bool useCache) {
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
    logInfo("Layers (%llu)", list.len());
    for (addr_size i = 0; i < list.len(); i++) {
        logInfo("\tname: %s, description: %s, spec version: %u, impl version: %u",
                list[i].layerName, list[i].description, list[i].specVersion, list[i].implementationVersion);
    }
}

bool checkSupportForLayer(const char* name) {
    auto res = getAllSupportedInstanceLayers();
    if (res.hasErr()) {
        logWarn("Query for all supported layers failed, reason: %s", res.err().toCStr());
        return false;
    }

    const LayerPropsList& layers = *res.value();
    for (addr_size i = 0; layers.len(); i++) {
        VkLayerProperties p = layers[i];
        if (core::memcmp(p.layerName, name, core::cstrLen(name)) == 0) {
            return true;
        }
    }

    return false;
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

} // namespace
