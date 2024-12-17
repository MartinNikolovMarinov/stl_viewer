#include <renderer.h>
#include <vulkan_include.h>
#include <platform.h>

#include <vector> // TODO: [REPLACE_WITH_CORE_IMPL]

using RendererErr = Renderer::Error;

void logVulkanVersion() {
    // Vulkan Initialization
    u32 version = 0;
    if (VkResult result = vkEnumerateInstanceVersion(&version); result == VK_SUCCESS) {
        std::cout << "Vulkan version: " << VK_VERSION_MAJOR(version) << "."
                  << VK_VERSION_MINOR(version) << "."
                  << VK_VERSION_PATCH(version) << "\n";
    }
    else {
        std::cerr << "Failed to enumerate Vulkan version.\n";
        Assert(false, "call to vkEnumerateInstanceVersion was not successfull!");
    }
}

core::expected<RendererErr> Renderer::init() {
    logVulkanVersion();

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
    std::vector<const char*> extensions (requiredPlatformExtCount + 1);
    Platform::requiredVulkanExts(extensions.data());
    extensions[1] = VK_KHR_SURFACE_EXTENSION_NAME;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    VkInstance instance;
    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance.\n";
        return core::unexpected(RendererErr::INIT_FAILED);
    }
    std::cout << "Vulkan instance created.\n";

    VkSurfaceKHR surface;
    if (auto err = Platform::createVulkanSurface(instance, surface); !Platform::isOk(err)) {
        const char* errMsg = Platform::errToCStr(err);
        std::cout << "Error: " << errMsg << std::endl;
        return core::unexpected(RendererErr::INIT_FAILED);
    }
    std::cout << "Vulkan surface created.\n";

    return {};
}
