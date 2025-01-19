
#include <app_error.h>
#include <basic.h>
#include <vulkan_include.h>

#define VK_MUST(expr) Assert((expr) == VK_SUCCESS)

struct VulkanQueue;
struct VulkanSurface;
struct VulkanDevice;
struct VulkanSwapchain;
struct VulkanShaderStage;
struct VulkanShader;
struct VulkanContext;

struct VulkanQueue {
    VkQueue handle = VK_NULL_HANDLE;
    i32 idx = -1;
};

struct VulkanSurface {
    struct Capabilities {
        VkSurfaceCapabilitiesKHR capabilities;
        core::ArrList<VkSurfaceFormatKHR> formats;
        core::ArrList<VkPresentModeKHR> presentModes;
    };

    struct CachedCapabilities {
        VkSurfaceFormatKHR format;
        VkPresentModeKHR presentMode;
        VkExtent2D extent;
        VkSurfaceTransformFlagBitsKHR currentTransform;
        u32 imageCount = 0;
    };

    VkSurfaceKHR handle = VK_NULL_HANDLE;
    CachedCapabilities capabilities;

    [[nodiscard]] static VulkanSurface::Capabilities queryCapabilities(const VulkanSurface& surface,
                                                                       VkPhysicalDevice physicalDevice);
    [[nodiscard]] static core::expected<CachedCapabilities, AppError> pickCapabilities(const Capabilities& capabilities);
};

struct VulkanDevice {
    struct PhysicalDevice {
        VkPhysicalDevice handle;
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
    VulkanSurface surface;

    VulkanQueue graphicsQueue = {};
    VulkanQueue presentQueue = {};

    [[nodiscard]] static core::expected<VulkanDevice, AppError> create(const struct RendererInitInfo& rendererInitInfo);
    [[nodiscard]] static core::expected<AppError> pickDevice(core::Memory<const PhysicalDevice> gpus, VulkanDevice& out);

    static void destroy(VulkanDevice& device);
};

struct VulkanSwapchain {
    VkSwapchainKHR handle = VK_NULL_HANDLE;
    core::ArrList<VkImage> images;
    core::ArrList<VkImageView> imageViews;
    VkFormat imageFormat;
    VkExtent2D extent;

    [[nodiscard]] static core::expected<VulkanSwapchain, AppError> create(const VulkanContext& vkctx,
                                                                          const VulkanSwapchain* old = nullptr);

    static void destroy(VulkanSwapchain& swapchain, const VulkanDevice& device);
};

struct VulkanShaderStage {
    enum Type : u8 {
        UNDEFINED,
        VERTEX,
        FRAGMENT,
    };

    u32 id = 0;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    u8* shaderBytes = nullptr;
    addr_size shaderBytesSize = 0;
    Type stageType = Type::UNDEFINED;

    [[nodiscard]] static core::expected<VulkanShaderStage, AppError> createFromFile(VkDevice logicalDevice,
                                                                                    core::StrView path,
                                                                                    Type stageType);
    static void destroy(VulkanShaderStage& stage, VkDevice logicalDevice);
};

// TODO: [REMAINDER]: These entire structure can be destroyed once a RenderPipeline has been created.
struct VulkanShader {
    struct CreateFromFileInfo {
        core::StrView vertexShaderPath;
        core::StrView fragmetShaderPath;
    };

    static constexpr addr_size MAX_SHADER_STAGES = 5;
    static constexpr const char* SHADERS_ENTRY_FUNCTION = "main";

    core::ArrStatic<VulkanShaderStage, MAX_SHADER_STAGES> stages;

    [[nodiscard]] static VulkanShader createGraphicsShaderFromFile(const CreateFromFileInfo& info,
                                                                   const VulkanContext& vkctx);
    static void destroy(VulkanShader& shader, VkDevice logicalDevice);
};

struct VulkanContext {
    VulkanDevice device;
    VulkanSwapchain swapchain;

    // EXPERIMENTAL SECTION:
    VulkanShader shader;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    core::ArrStatic<VkFramebuffer, 3> frameBuffers;
};
