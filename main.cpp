#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

int main() {
    #ifdef __APPLE__
        setenv("VK_ICD_FILENAMES", "/Users/good-mood14/repos/stl_viewer/lib/MoltenVK/sdk-1.3.296.0/MoltenVK_icd.json", 1);
    #endif

    uint32_t version = 0;
    VkResult result = vkEnumerateInstanceVersion(&version);
    if (result == VK_SUCCESS) {
        std::cout << "Vulkan version: "
                  << VK_VERSION_MAJOR(version) << "."
                  << VK_VERSION_MINOR(version) << "."
                  << VK_VERSION_PATCH(version) << "\n";
    }
    else {
        std::cerr << "Failed to enumerate Vulkan version.\n";
        return -1;
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return -1;
    }

    std::cout << "GLFW initialized successfully and Vulkan is supported.\n";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (!glfwVulkanSupported()) {
        std::cerr << "GLFW: Vulkan not supported.\n";
        glfwTerminate();
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }
    glfwSetErrorCallback([](int error, const char* description) {
        std::cout << "GLFW error:" << error << " - " << description << std::endl;
    });

    std::cout << "GLFW window created successfully.\n";

    VkInstance instance;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Minimal Vulkan Example";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions != nullptr && glfwExtensionCount > 0) {
        std::cout << "GLFW required extensions:\n";
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            std::cout << "\t" << glfwExtensions[i] << "\n";
        }
    } else {
        std::cerr << "GLFW: Failed to get required Vulkan instance extensions.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::vector<const char*> extensions (glfwExtensions, glfwExtensions + glfwExtensionCount);

    VkInstanceCreateInfo instCreateInfo{};
    #ifdef __APPLE__
    // // Add the portability enumeration extension for MoltenVK
    // extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    // // Set the enumerate portability flag
    // instCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
    instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.pApplicationInfo = &appInfo;
    instCreateInfo.enabledExtensionCount = extensions.size();
    instCreateInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&instCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "Vulkan instance successfully created.\n";

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan surface.\n";
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "Vulkan surface successfully created.\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
