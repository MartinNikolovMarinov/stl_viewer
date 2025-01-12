#include <app_logger.h>
#include <platform.h>
#include <renderer.h>
#include <vulkan_device_picker.h>
#include <vulkan_include.h>
#include <vulkan_swapchain.h>
#include <vulkan_renderpipeline.h>

using RendererError::FAILED_TO_CREATE_VULKAN_INSTANCE;
using RendererError::FAILED_TO_GET_VULKAN_VERSION;
using RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES;
using RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES;
using RendererError::FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT;
using RendererError::FAILED_TO_CREATE_VULKAN_DEBUG_MESSENGER;
using RendererError::FAILED_TO_FIND_GPUS_WITH_VULKAN_SUPPORT;
using RendererError::FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES;
using RendererError::FAILED_TO_CREATE_LOGICAL_DEVICE;
using RendererError::FAILED_TO_CREATE_VULKAN_FRAME_BUFFER;
using RendererError::FAILED_TO_CREATE_VULKAN_COMMAND_POOL;
using RendererError::FAILED_TO_ALLOCATE_VULKAN_COMMAND_BUFFER;
using RendererError::FAILED_TO_ALLOCATE_VULKAN_SEMAPHORE;
using RendererError::FAILED_TO_ALLOCATE_VULKAN_FENCE;

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
#define FAILED_TO_CREATE_VULKAN_FRAME_BUFFER_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_CREATE_VULKAN_FRAME_BUFFER));
#define FAILED_TO_CREATE_VULKAN_COMMAND_POOL_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_CREATE_VULKAN_COMMAND_POOL));
#define FAILED_TO_ALLOCATE_VULKAN_COMMAND_BUFFER_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_ALLOCATE_VULKAN_COMMAND_BUFFER));
#define FAILED_TO_ALLOCATE_VULKAN_SEMAPHORE_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_ALLOCATE_VULKAN_SEMAPHORE));
#define FAILED_TO_ALLOCATE_VULKAN_FENCE_ERREXPR \
    core::unexpected(createRendErr(FAILED_TO_ALLOCATE_VULKAN_FENCE));

namespace {

using ExtPropsList = core::ArrList<VkExtensionProperties>;
using LayerPropsList = core::ArrList<VkLayerProperties>;
using GPUDeviceList = core::ArrList<GPUDevice>;
using FrameBufferList = core::ArrList<VkFramebuffer>;

struct VulkanQueue {
    VkQueue queue = VK_NULL_HANDLE;
    i32 idx = -1;
};

struct RecreateSwapchain {
    u32 width = 0;
    u32 height = 0;
    bool recreate = false;
};

#if STLV_DEBUG
constexpr bool VALIDATION_LAYERS_ENABLED = true;
#else
constexpr bool VALIDATION_LAYERS_ENABLED = false;
#endif

const char* requiredDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(OS_MAC) && OS_MAC == 1
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
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
Swapchain::CreateInfo g_swapchainInfo = {};
RecreateSwapchain g_swapchainRecreate = {};
FrameBufferList g_swapchainFrameBuffers;
RenderPipeline g_renderPipeline = {};
VkCommandPool g_commandPool = VK_NULL_HANDLE;
constexpr i32 MAX_FRAMES_IN_FLIGHT = 2;
u32 g_currentFrame = 0;
core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> g_cmdBufs;
core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> g_imageAvailableSemaphores;
core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> g_renderFinishedSemaphores;
core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT> g_inFlightFences;

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
core::expected<FrameBufferList, AppError> createFrameBuffers(VkDevice logicalDevice,
                                                             const Swapchain& swapchain,
                                                             const RenderPipeline& pipeline);
core::expected<VkCommandPool, AppError> createCommandPool(VkDevice logicalDevice, const PickedGPUDevice& picked);
core::expected<core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>, AppError> createCommandBuffers(VkDevice logicalDevice,
                                                                                                      VkCommandPool pool);

core::expected<VkDebugUtilsMessengerEXT, AppError> vulkanCreateDebugMessenger(VkInstance instance);

core::expected<core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT>, AppError> createSemaphores(VkDevice logicalDevice);
core::expected<core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT>, AppError> createFences(VkDevice logicalDevice);

void recordCommandBuffer(VkCommandBuffer cmdBuffer, u32 imageIdx,
                         const RenderPipeline& renderPipeline,
                         const FrameBufferList& frameBuffers,
                         const Swapchain& swapchain);

} // namespace

core::expected<AppError> Renderer::init(const RendererInitInfo& info) {
    core::setLoggerTag(VULKAN_VALIDATION_TAG, appLogTagsToCStr(VULKAN_VALIDATION_TAG));

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
        logInfoTagged(RENDERER_TAG, "Vulkan Instance created");
    }

    // Create Debug Messenger
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        auto res = vulkanCreateDebugMessenger(instance);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        debugMessenger = res.value();
        logInfoTagged(RENDERER_TAG, "Vulkan Debug Messenger created");
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
    Swapchain::CreateInfo swapchainInfo;
    {
        swapchainInfo.imageCount = pickedDevice.imageCount;
        swapchainInfo.surfaceFormat = pickedDevice.surfaceFormat;
        swapchainInfo.extent = pickedDevice.extent;
        swapchainInfo.graphicsQueueIdx = pickedDevice.graphicsQueueIdx;
        swapchainInfo.presentQueueIdx = pickedDevice.presentQueueIdx;
        swapchainInfo.currentTransform = pickedDevice.currentTransform;
        swapchainInfo.presentMode = pickedDevice.presentMode;
        swapchainInfo.logicalDevice = logicalDevice;
        swapchainInfo.surface = surface;
        auto res = Swapchain::create(swapchainInfo);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        swapchain = std::move(res.value());
        logInfoTagged(RENDERER_TAG, "Swapchain created");
    }

    // Create Rendering Pipeline
    RenderPipeline renderPipeline;
    {
        RenderPipeline::CreateInfo pipelineCreateInfo;
        pipelineCreateInfo.logicalDevice = logicalDevice;
        pipelineCreateInfo.fragShaderPath = core::sv(STLV_ASSETS "/shaders/shader.frag.spirv");
        pipelineCreateInfo.vertShaderPath = core::sv(STLV_ASSETS "/shaders/shader.vert.spirv");
        pipelineCreateInfo.swapchainExtent = swapchain.extent;
        pipelineCreateInfo.swapchainImageFormat = swapchain.imageFormat;
        auto res = RenderPipeline::create(std::move(pipelineCreateInfo));
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        renderPipeline = std::move(res.value());
        logInfoTagged(RENDERER_TAG, "Render Pipeline created");
    }

    // Create Frame Buffers
    FrameBufferList frameBuffers;
    {
        auto res = createFrameBuffers(logicalDevice, swapchain, renderPipeline);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        frameBuffers = std::move(res.value());
        logInfoTagged(RENDERER_TAG, "Frame Buffers created");
    }

    // Create Command Pool
    VkCommandPool commandPool = VK_NULL_HANDLE;
    {
        auto res = createCommandPool(logicalDevice, pickedDevice);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        commandPool = res.value();
        logInfoTagged(RENDERER_TAG, "Command Pool created");
    }

    // Create Command Buffer
    core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmdBufs;
    {
        auto res = createCommandBuffers(logicalDevice, commandPool);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        cmdBufs = std::move(res.value());
        logInfoTagged(RENDERER_TAG, "Command Buffer created");
    }

    // Create Synchronization Objects
    core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;
    {
        {
            auto res = createSemaphores(logicalDevice);
            if (res.hasErr()) return core::unexpected(res.err());
            imageAvailableSemaphores = std::move(res.value());
        }
        {
            auto res = createSemaphores(logicalDevice);
            if (res.hasErr()) return core::unexpected(res.err());
            renderFinishedSemaphores = std::move(res.value());
        }
        {
            auto res = createFences(logicalDevice);
            if (res.hasErr()) return core::unexpected(res.err());
            inFlightFences = std::move(res.value());
        }

        logInfoTagged(RENDERER_TAG, "Synchronization Objects created");
    }

    g_instance = instance;
    g_surface = surface;
    g_selectedGPU = pickedDevice.gpu;
    g_device = logicalDevice;
    g_graphicsQueue = graphicsQueue;
    g_presentQueue = presentQueue;
    g_swapchain = std::move(swapchain);
    g_swapchainInfo = std::move(g_swapchainInfo);
    g_swapchainRecreate = {};
    g_renderPipeline = std::move(renderPipeline);
    g_swapchainFrameBuffers = std::move(frameBuffers);
    g_commandPool = commandPool;
    g_cmdBufs = std::move(cmdBufs);
    g_imageAvailableSemaphores = std::move(imageAvailableSemaphores);
    g_renderFinishedSemaphores = std::move(renderFinishedSemaphores);
    g_inFlightFences = std::move(inFlightFences);

    return {};
}

void Renderer::drawFrame() {
    Assert(vkWaitForFences(g_device, 1, &g_inFlightFences[g_currentFrame], VK_TRUE, core::limitMax<u64>()) == VK_SUCCESS);
    vkResetFences(g_device, 1, &g_inFlightFences[g_currentFrame]);

    u32 imageIdx;
    // TODO: Needs different error handling!
    vkAcquireNextImageKHR(g_device,
                          g_swapchain.swapchain,
                          core::limitMax<u64>(),
                          g_imageAvailableSemaphores[g_currentFrame],
                          VK_NULL_HANDLE,
                          &imageIdx);

    Assert(vkResetCommandBuffer(g_cmdBufs[g_currentFrame], 0) == VK_SUCCESS);
    recordCommandBuffer(g_cmdBufs[g_currentFrame], imageIdx, g_renderPipeline, g_swapchainFrameBuffers, g_swapchain);

    // Submit to the command buffer to the Graphics Queue
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &g_cmdBufs[g_currentFrame];

        VkSemaphore waitSemaphores[] = { g_imageAvailableSemaphores[g_currentFrame] };
        constexpr addr_size waitSemaphoresLen = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]);
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = waitSemaphoresLen;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        VkSemaphore signalSemaphores[] = { g_renderFinishedSemaphores[g_currentFrame] };
        constexpr addr_size signalSemaphoresLen = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);
        submitInfo.signalSemaphoreCount = signalSemaphoresLen;
        submitInfo.pSignalSemaphores = signalSemaphores;

        Assert(vkQueueSubmit(g_graphicsQueue.queue, 1, &submitInfo, g_inFlightFences[g_currentFrame]) == VK_SUCCESS);
    }

    // Present
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore signalSemaphores[] = { g_renderFinishedSemaphores[g_currentFrame] };
        constexpr addr_size signalSemaphoresLen = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);

        presentInfo.waitSemaphoreCount = signalSemaphoresLen;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { g_swapchain.swapchain };
        constexpr addr_size swapchainsLen = sizeof(swapchains) / sizeof(swapchains[0]);
        presentInfo.swapchainCount = swapchainsLen;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIdx;
        presentInfo.pResults = nullptr;

        // TODO: Needs different error handling!
        vkQueuePresentKHR(g_presentQueue.queue, &presentInfo);
    }

    g_currentFrame = (g_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::resizeTarget(u32 width, u32 height) {
    g_swapchainRecreate.height = height;
    g_swapchainRecreate.width = width;
    g_swapchainRecreate.recreate = true;
}

void Renderer::shutdown() {
    if (VkResult vres = vkDeviceWaitIdle(g_device); vres != VK_SUCCESS) {
        logErrTagged(RENDERER_TAG, "Failed to wait idle the logical device");
    }

    logInfoTagged(RENDERER_TAG, "Destroying Synchronization objects");
    for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(g_device, g_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(g_device, g_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(g_device, g_inFlightFences[i], nullptr);
    }

    if (g_commandPool != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Command Pool");
        vkDestroyCommandPool(g_device, g_commandPool, nullptr);
        g_commandPool = VK_NULL_HANDLE;
    }

    if (!g_swapchainFrameBuffers.empty()) {
        logInfoTagged(RENDERER_TAG, "Destroying FrameBuffers");
        for (addr_size i = 0; i < g_swapchainFrameBuffers.len(); i++) {
            vkDestroyFramebuffer(g_device, g_swapchainFrameBuffers[i], nullptr);
        }
        g_swapchainFrameBuffers.free();
    }

    Swapchain::destroy(g_device, g_swapchain);
    RenderPipeline::destroy(g_device, g_renderPipeline);

    if (g_surface != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Vulkan KHR surface");
        vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
        g_surface = VK_NULL_HANDLE;
    }

    if (g_device != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying logical device");
        vkDestroyDevice(g_device, nullptr);
        g_device = VK_NULL_HANDLE;
        g_graphicsQueue = {};
        g_presentQueue = {};
    }

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

    VkFlags instanceCreateInfoFlags = 0;

    // Setup Instance Extensions
    {
        extensions.push(VK_KHR_SURFACE_EXTENSION_NAME);

        #if defined(OS_MAC) && OS_MAC == 1
            // FIXME: Some of these might be optional ??
            // Enable portability extension for MacOS.
            extensions.push(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            extensions.push(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            instanceCreateInfoFlags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        #endif

        // Check required extensions
        {
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

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.flags = instanceCreateInfoFlags;
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = u32(extensions.len());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    // Setup Validation Layers
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        const char* layerNames[1];
        bool supported = checkSupportForInstLayer("VK_LAYER_KHRONOS_validation");
                        //  checkSupportForInstLayer("VK_LAYER_LUNARG_api_dump");
        if (supported) {
            layerNames[0] = "VK_LAYER_KHRONOS_validation";
            // layerNames[1] = "VK_LAYER_LUNARG_api_dump";
            instanceCreateInfo.enabledLayerCount = sizeof(layerNames) / sizeof(layerNames[0]);
            instanceCreateInfo.ppEnabledLayerNames = layerNames;
            logInfoTagged(RENDERER_TAG, "Enabling VK_LAYER_KHRONOS_validation layer");

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
    deviceCreateInfo.queueCreateInfoCount    = u32(queueInfos.len());
    deviceCreateInfo.pEnabledFeatures        = &deviceFeatures;

    // If you need device-specific extensions (like swapchain):
    deviceCreateInfo.enabledExtensionCount   = u32(deviceExts.len());
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

core::expected<FrameBufferList, AppError> createFrameBuffers(VkDevice logicalDevice,
                                                             const Swapchain& swapchain,
                                                             const RenderPipeline& pipeline) {
    auto ret = FrameBufferList(swapchain.imageViews.len(), VkFramebuffer{});

    for (size_t i = 0; i < swapchain.imageViews.len(); i++) {
        VkImageView attachments[] = {
            swapchain.imageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pipeline.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchain.extent.width;
        framebufferInfo.height = swapchain.extent.height;
        framebufferInfo.layers = 1;

        if (
            VkResult vres = vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &ret[i]);
            vres != VK_SUCCESS
        ) {
            return FAILED_TO_CREATE_VULKAN_FRAME_BUFFER_ERREXPR;
        }
    }

    return ret;
}

core::expected<VkCommandPool, AppError> createCommandPool(VkDevice logicalDevice, const PickedGPUDevice& picked) {
    VkCommandPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCreateInfo.queueFamilyIndex = picked.graphicsQueueIdx;

    VkCommandPool ret;
    if (
        VkResult vres =vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &ret);
        vres != VK_SUCCESS
    ) {
        return FAILED_TO_CREATE_VULKAN_COMMAND_POOL_ERREXPR;
    }

    return ret;
}

core::expected<core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>, AppError> createCommandBuffers(VkDevice logicalDevice, VkCommandPool pool) {
    VkCommandBufferAllocateInfo allocCreateInfo{};
    allocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocCreateInfo.commandPool = pool;
    allocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCreateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmdBuffers;
    if (vkAllocateCommandBuffers(logicalDevice, &allocCreateInfo, cmdBuffers.data()) != VK_SUCCESS) {
        return FAILED_TO_ALLOCATE_VULKAN_COMMAND_BUFFER_ERREXPR;
    }

    return cmdBuffers;
}

core::expected<core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT>, AppError> createSemaphores(VkDevice logicalDevice) {
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> ret;
    for (addr_size i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (
            VkResult vres = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &ret[i]);
            vres != VK_SUCCESS
        ) {
            return FAILED_TO_ALLOCATE_VULKAN_SEMAPHORE_ERREXPR;
        }
    }

    return ret;
}

core::expected<core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT>, AppError> createFences(VkDevice logicalDevice) {
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT> ret;
    for (addr_size i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (
            VkResult vres = vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &ret[i]);
            vres != VK_SUCCESS
        ) {
            return FAILED_TO_ALLOCATE_VULKAN_FENCE_ERREXPR;
        }
    }

    return ret;
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

void recordCommandBuffer(VkCommandBuffer cmdBuffer, u32 imageIdx,
                         const RenderPipeline& renderPipeline,
                         const FrameBufferList& frameBuffers,
                         const Swapchain& swapchain) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    Assert(vkBeginCommandBuffer(cmdBuffer, &beginInfo) == VK_SUCCESS);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPipeline.renderPass;
    renderPassInfo.framebuffer = frameBuffers[imageIdx];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain.extent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // BLACK
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    {
        vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        defer { vkCmdEndRenderPass(cmdBuffer); };

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipeline.graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = f32(swapchain.extent.width);
        viewport.height = f32(swapchain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchain.extent;
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
    }

    Assert(vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS);
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
