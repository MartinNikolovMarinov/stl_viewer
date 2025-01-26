
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
    [[nodiscard]] static core::expected<CachedCapabilities, AppError> pickCapabilities(const Capabilities& capabilities, bool vSyncOn);
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
    Extensions deviceExtensions = {};
    VkPhysicalDeviceProperties physicalDeviceProps;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VulkanSurface surface = {};
    bool vSyncOn = false;

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

struct Mesh2D {
    template <typename T>
    using container_type = core::ArrList<T>;

    struct MeshBindData {
        core::vec2f positions;
        core::vec4f colors;
        // container_type<core::vec2f> normals;
        // container_type<core::vec2f> uv0;
    };

    container_type<MeshBindData> bindingData;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    inline addr_size vertexCount() {
        return bindingData.len();
    }

    inline addr_size vertexByteSize() {
        return sizeof(MeshBindData) * vertexCount();
    }

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(MeshBindData);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static core::ArrStatic<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        core::ArrStatic<VkVertexInputAttributeDescription, 2> attributeDescriptions (2, {});

        // positions parameter attributes:
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // ain't this stupid
        attributeDescriptions[0].offset = offsetof(MeshBindData, positions);

        // color parameter attributes:
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(MeshBindData, colors);

        return attributeDescriptions;
    }

    static void destroy(VulkanDevice& device, Mesh2D& mesh) {
        if (mesh.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device.logicalDevice, mesh.vertexBuffer, nullptr);
        }
        if (mesh.vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device.logicalDevice, mesh.vertexBufferMemory, nullptr);
        }
    }
};

struct VulkanContext {
    VulkanDevice device;
    VulkanSwapchain swapchain;

    core::ArrList<Mesh2D> meshes;

    // EXPERIMENTAL SECTION:
    VulkanShader shader;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    core::ArrStatic<VkFramebuffer, 5> frameBuffers;
    core::ArrStatic<VkFence, 5> inFlightFences;
    core::ArrStatic<VkSemaphore, 5> imageAvailableSemaphores;
    core::ArrStatic<VkSemaphore, 5> renderFinishedSemaphores;
    core::ArrStatic<VkCommandBuffer, 5> cmdBuffers;
    VkCommandPool cmdBuffersPool;
    u32 currentFrame = 0;
    u32 maxFramesInFlight = 0;
    bool frameBufferResized = false;
};
