#pragma once

#include <app_error.h>
#include <basic.h>
#include <core_system_checks.h>

#if defined(OS_MAC) && OS_MAC == 1
    #define VK_USE_PLATFORM_METAL_EXT
    #define VK_ENABLE_BETA_EXTENSIONS // need this for VK_KHR_portability_subset
    #include <vulkan/vulkan.h>
#elif defined(OS_WIN) && OS_WIN == 1
    #define VK_USE_PLATFORM_WIN32_KHR
    #define WIN32_LEAN_AND_MEAN // vulkan.h might include windows.h
    #include <vulkan/vulkan.h>
#elif defined(OS_LINUX) && OS_LINUX == 1
    #ifdef USE_X11
        #define VK_USE_PLATFORM_XLIB_KHR
        #include <vulkan/vulkan.h>
    #else
        #define VK_USE_PLATFORM_WAYLAND_KHR
        #include <vulkan/vulkan.h>
    #endif
#endif

#define VK_MUST(expr) Assert((expr) == VK_SUCCESS)

struct Queue {
    VkQueue handle = VK_NULL_HANDLE;
    i32 idx = -1;
};

struct Surface {
    VkSurfaceKHR handle = VK_NULL_HANDLE;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
    VkSurfaceTransformFlagBitsKHR currentTransform;
    u32 imageCount = 0;
};

struct Device {
    struct PhysicalDevice {
        VkPhysicalDevice device;
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures features;
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    core::Memory<const char*> requiredDeviceExtensions;
    VkPhysicalDeviceProperties physicalDeviceProps;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    Surface surface;

    Queue graphicsQueue = {};
    Queue presentQueue = {};

    [[nodiscard]] static core::expected<Device, AppError> create(const struct RendererInitInfo& rendererInitInfo);
    [[nodiscard]] static core::expected<AppError> pickDevice(core::Memory<const PhysicalDevice> gpus, Device& out);

    static void destroy(Device& device);
};

struct Swapchain {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    core::ArrList<VkImage> images;
    core::ArrList<VkImageView> imageViews;
    VkFormat imageFormat;
    VkExtent2D extent;
};

struct Pipeline {
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    core::ArrList<VkFramebuffer> frameBuffers;
};

struct CommandBuffers {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    core::ArrList<VkCommandBuffer> cmdBuffers;
};

struct VulkanContext {
    Device device;
    Surface surface;
    Swapchain swapchain;
    Pipeline pipeline;
    CommandBuffers cmdBuffers;
};
