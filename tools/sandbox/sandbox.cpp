#include "./sandbox.h"

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

int runSandbox() {
    // Query the number of available layers
    uint32_t layerCount = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    if (result != VK_SUCCESS || layerCount == 0) {
        std::cerr << "Failed to enumerate Vulkan layers or no layers available" << std::endl;
        return -1;
    }

    // Retrieve the layer properties
    std::vector<VkLayerProperties> layers(layerCount);
    result = vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    if (result != VK_SUCCESS) {
        std::cerr << "Failed to retrieve Vulkan layer properties" << std::endl;
        return -1;
    }

    // Print the layers
    std::cout << "Available Vulkan Layers:" << std::endl;
    for (const auto& layer : layers) {
        std::cout << "-----------------------------------" << std::endl;
        std::cout << "Layer Name: " << layer.layerName << std::endl;
        std::cout << "Description: " << layer.description << std::endl;
        std::cout << "Version: " << VK_VERSION_MAJOR(layer.specVersion) << ""
                  << VK_VERSION_MINOR(layer.specVersion) << ""
                  << VK_VERSION_PATCH(layer.specVersion) << std::endl;
        std::cout << "Implementation Version: " << layer.implementationVersion << std::endl;
    }

    return 0;
}
