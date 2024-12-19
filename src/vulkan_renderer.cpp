#include <renderer.h>
#include <vulkan_include.h>
#include <platform.h>

#include <vector> // TODO: [REPLACE_WITH_CORE_IMPL]

using RendererError::FAILED_TO_CREATE_VULKAN_INSTANCE;
using RendererError::FAILED_TO_GET_VULKAN_VERSION;

#define FAILED_TO_GET_VULKAN_VERSION_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_GET_VULKAN_VERSION));
#define FAILED_TO_CREATE_VULKAN_INSTANCE_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_CREATE_VULKAN_INSTANCE));

core::expected<AppError> Renderer::getVersion(char out[VERSION_BUFFER_SIZE]) {
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

core::expected<AppError> Renderer::init() {
    // FIXME: getVersion and log it!

    // TODO2: Verify I do not need this:
    // #ifdef OS_MAC
    //     setenv("VK_ICD_FILENAMES", "./lib/MoltenVK/sdk-1.3.296.0/MoltenVK_icd.json", 1);
    // #endif

    // Initialize Vulkan Instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Metal Example";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Retrieve required extensions
    i32 requiredPlatformExtCount = 0;
    Platform::requiredVulkanExtsCount(requiredPlatformExtCount);
    std::vector<const char*> extensions (requiredPlatformExtCount + 1); // TODO: [REPLACE_WITH_CORE_IMPL]
    Platform::requiredVulkanExts(extensions.data());
    extensions[1] = VK_KHR_SURFACE_EXTENSION_NAME;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        return FAILED_TO_CREATE_VULKAN_INSTANCE_ERREXPR;
    }

    std::cout << "Vulkan instance created.\n"; // TODO: [REPLACE_WITH_CORE_IMPL]

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (auto err = Platform::createVulkanSurface(instance, surface); !err.isOk()) {
        return core::unexpected(err);
    }

    std::cout << "Vulkan surface created.\n"; // TODO: [REPLACE_WITH_CORE_IMPL]

    return {};
}
