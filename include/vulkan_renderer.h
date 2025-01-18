
#include <app_error.h>
#include <basic.h>
#include <vulkan_include.h>

#define VK_MUST(expr) Assert((expr) == VK_SUCCESS)

struct VulkanQueue;
struct Surface;
struct Device;
struct Swapchain;
struct Pipeline;
struct CommandBuffers;
struct VulkanContext;

struct VulkanQueue {
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

    struct Extensions {
        core::Memory<const char*> required;
        core::Memory<const char*> optional;
        core::ArrList<bool> optionalIsActive;

        inline addr_size enabledExtensionsCount() const {
            addr_size computed = required.len();
            for (addr_size i = 0; i < optionalIsActive.len(); i++) {
                if (optionalIsActive[i]) computed++;
            }
            return computed;
        }
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    Extensions deviceExtensions;
    VkPhysicalDeviceProperties physicalDeviceProps;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    Surface surface;

    VulkanQueue graphicsQueue = {};
    VulkanQueue presentQueue = {};

    [[nodiscard]] static core::expected<Device, AppError> create(const struct RendererInitInfo& rendererInitInfo);
    [[nodiscard]] static core::expected<AppError> pickDevice(core::Memory<const PhysicalDevice> gpus, Device& out);

    static void destroy(Device& device);
};

struct Swapchain {
    VkSwapchainKHR handle = VK_NULL_HANDLE;
    core::ArrList<VkImage> images;
    core::ArrList<VkImageView> imageViews;
    VkFormat imageFormat;
    VkExtent2D extent;

    [[nodiscard]] static core::expected<Swapchain, AppError> create(const VulkanContext& vkctx);

    static void destroy(Swapchain& swapchain, const Device& device);
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
    Swapchain swapchain;
    Pipeline pipeline;
    CommandBuffers cmdBuffers;
};
