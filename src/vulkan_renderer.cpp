#include <app_logger.h>
#include <platform.h>
#include <renderer.h>
#include <vulkan_renderer.h>
// #include <vulkan_swapchain.h>
// #include <vulkan_renderpipeline.h>

// namespace {

// struct RecreateSwapchain {
//     u32 width = 0;
//     u32 height = 0;
//     bool recreate = false;
// };

// VkDevice g_device = VK_NULL_HANDLE;
// VulkanQueue g_graphicsQueue = {};
// VulkanQueue g_presentQueue = {};
// Swapchain g_swapchain = {};
// Swapchain::CreateInfo g_swapchainInfo = {};
// RecreateSwapchain g_swapchainRecreate = {};
// FrameBufferList g_swapchainFrameBuffers;
// RenderPipeline g_renderPipeline = {};
// VkCommandPool g_commandPool = VK_NULL_HANDLE;
// constexpr i32 MAX_FRAMES_IN_FLIGHT = 2;
// u32 g_currentFrame = 0;
// core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> g_cmdBufs;
// core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> g_imageAvailableSemaphores;
// core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> g_renderFinishedSemaphores;
// core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT> g_inFlightFences;

// core::expected<VkDevice, AppError> vulkanCreateLogicalDevice(const PickedGPUDevice& picked, core::Memory<const char*> deviceExts);
// core::expected<FrameBufferList, AppError> createFrameBuffers(VkDevice logicalDevice,
//                                                              const Swapchain& swapchain,
//                                                              const RenderPipeline& pipeline);
// core::expected<VkCommandPool, AppError> createCommandPool(VkDevice logicalDevice, const PickedGPUDevice& picked);
// core::expected<core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>, AppError> createCommandBuffers(VkDevice logicalDevice,
//                                                                                                       VkCommandPool pool);

// core::expected<core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT>, AppError> createSemaphores(VkDevice logicalDevice);
// core::expected<core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT>, AppError> createFences(VkDevice logicalDevice);

// void recordCommandBuffer(VkCommandBuffer cmdBuffer, u32 imageIdx,
//                          const RenderPipeline& renderPipeline,
//                          const FrameBufferList& frameBuffers,
//                          const Swapchain& swapchain);

// } // namespace

// core::expected<AppError> Renderer::init(const RendererInitInfo& info) {

//     // Create Swapchain
//     Swapchain swapchain;
//     Swapchain::CreateInfo swapchainInfo;
//     {
//         swapchainInfo.imageCount = pickedDevice.imageCount;
//         swapchainInfo.surfaceFormat = pickedDevice.surfaceFormat;
//         swapchainInfo.extent = pickedDevice.extent;
//         swapchainInfo.graphicsQueueIdx = pickedDevice.graphicsQueueIdx;
//         swapchainInfo.presentQueueIdx = pickedDevice.presentQueueIdx;
//         swapchainInfo.currentTransform = pickedDevice.currentTransform;
//         swapchainInfo.presentMode = pickedDevice.presentMode;
//         swapchainInfo.logicalDevice = logicalDevice;
//         swapchainInfo.surface = surface;
//         auto res = Swapchain::create(swapchainInfo);
//         if (res.hasErr()) {
//             return core::unexpected(res.err());
//         }
//         swapchain = std::move(res.value());
//         logInfoTagged(RENDERER_TAG, "Swapchain created");
//     }

//     // Create Rendering Pipeline
//     RenderPipeline renderPipeline;
//     {
//         RenderPipeline::CreateInfo pipelineCreateInfo;
//         pipelineCreateInfo.logicalDevice = logicalDevice;
//         pipelineCreateInfo.fragShaderPath = core::sv(STLV_ASSETS "/shaders/shader.frag.spirv");
//         pipelineCreateInfo.vertShaderPath = core::sv(STLV_ASSETS "/shaders/shader.vert.spirv");
//         pipelineCreateInfo.swapchainExtent = swapchain.extent;
//         pipelineCreateInfo.swapchainImageFormat = swapchain.imageFormat;
//         auto res = RenderPipeline::create(std::move(pipelineCreateInfo));
//         if (res.hasErr()) {
//             return core::unexpected(res.err());
//         }
//         renderPipeline = std::move(res.value());
//         logInfoTagged(RENDERER_TAG, "Render Pipeline created");
//     }

//     // Create Frame Buffers
//     FrameBufferList frameBuffers;
//     {
//         auto res = createFrameBuffers(logicalDevice, swapchain, renderPipeline);
//         if (res.hasErr()) {
//             return core::unexpected(res.err());
//         }
//         frameBuffers = std::move(res.value());
//         logInfoTagged(RENDERER_TAG, "Frame Buffers created");
//     }

//     // Create Command Pool
//     VkCommandPool commandPool = VK_NULL_HANDLE;
//     {
//         auto res = createCommandPool(logicalDevice, pickedDevice);
//         if (res.hasErr()) {
//             return core::unexpected(res.err());
//         }
//         commandPool = res.value();
//         logInfoTagged(RENDERER_TAG, "Command Pool created");
//     }

//     // Create Command Buffer
//     core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmdBufs;
//     {
//         auto res = createCommandBuffers(logicalDevice, commandPool);
//         if (res.hasErr()) {
//             return core::unexpected(res.err());
//         }
//         cmdBufs = std::move(res.value());
//         logInfoTagged(RENDERER_TAG, "Command Buffer created");
//     }

//     // Create Synchronization Objects
//     core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
//     core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
//     core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;
//     {
//         {
//             auto res = createSemaphores(logicalDevice);
//             if (res.hasErr()) return core::unexpected(res.err());
//             imageAvailableSemaphores = std::move(res.value());
//         }
//         {
//             auto res = createSemaphores(logicalDevice);
//             if (res.hasErr()) return core::unexpected(res.err());
//             renderFinishedSemaphores = std::move(res.value());
//         }
//         {
//             auto res = createFences(logicalDevice);
//             if (res.hasErr()) return core::unexpected(res.err());
//             inFlightFences = std::move(res.value());
//         }

//         logInfoTagged(RENDERER_TAG, "Synchronization Objects created");
//     }

//     g_instance = instance;
//     g_surface = surface;
//     g_selectedGPU = pickedDevice.gpu;
//     g_device = logicalDevice;
//     g_graphicsQueue = graphicsQueue;
//     g_presentQueue = presentQueue;
//     g_swapchain = std::move(swapchain);
//     g_swapchainInfo = std::move(g_swapchainInfo);
//     g_swapchainRecreate = {};
//     g_renderPipeline = std::move(renderPipeline);
//     g_swapchainFrameBuffers = std::move(frameBuffers);
//     g_commandPool = commandPool;
//     g_cmdBufs = std::move(cmdBufs);
//     g_imageAvailableSemaphores = std::move(imageAvailableSemaphores);
//     g_renderFinishedSemaphores = std::move(renderFinishedSemaphores);
//     g_inFlightFences = std::move(inFlightFences);

//     return {};
// }

// void Renderer::drawFrame() {
//     Assert(vkWaitForFences(g_device, 1, &g_inFlightFences[g_currentFrame], VK_TRUE, core::limitMax<u64>()) == VK_SUCCESS);
//     vkResetFences(g_device, 1, &g_inFlightFences[g_currentFrame]);

//     u32 imageIdx;
//     // TODO: Needs different error handling!
//     vkAcquireNextImageKHR(g_device,
//                           g_swapchain.swapchain,
//                           core::limitMax<u64>(),
//                           g_imageAvailableSemaphores[g_currentFrame],
//                           VK_NULL_HANDLE,
//                           &imageIdx);

//     Assert(vkResetCommandBuffer(g_cmdBufs[g_currentFrame], 0) == VK_SUCCESS);
//     recordCommandBuffer(g_cmdBufs[g_currentFrame], imageIdx, g_renderPipeline, g_swapchainFrameBuffers, g_swapchain);

//     // Submit to the command buffer to the Graphics Queue
//     {
//         VkSubmitInfo submitInfo{};
//         submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//         submitInfo.commandBufferCount = 1;
//         submitInfo.pCommandBuffers = &g_cmdBufs[g_currentFrame];

//         VkSemaphore waitSemaphores[] = { g_imageAvailableSemaphores[g_currentFrame] };
//         constexpr addr_size waitSemaphoresLen = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]);
//         VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
//         submitInfo.waitSemaphoreCount = waitSemaphoresLen;
//         submitInfo.pWaitSemaphores = waitSemaphores;
//         submitInfo.pWaitDstStageMask = waitStages;

//         VkSemaphore signalSemaphores[] = { g_renderFinishedSemaphores[g_currentFrame] };
//         constexpr addr_size signalSemaphoresLen = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);
//         submitInfo.signalSemaphoreCount = signalSemaphoresLen;
//         submitInfo.pSignalSemaphores = signalSemaphores;

//         Assert(vkQueueSubmit(g_graphicsQueue.queue, 1, &submitInfo, g_inFlightFences[g_currentFrame]) == VK_SUCCESS);
//     }

//     // Present
//     {
//         VkPresentInfoKHR presentInfo{};
//         presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

//         VkSemaphore signalSemaphores[] = { g_renderFinishedSemaphores[g_currentFrame] };
//         constexpr addr_size signalSemaphoresLen = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);

//         presentInfo.waitSemaphoreCount = signalSemaphoresLen;
//         presentInfo.pWaitSemaphores = signalSemaphores;

//         VkSwapchainKHR swapchains[] = { g_swapchain.swapchain };
//         constexpr addr_size swapchainsLen = sizeof(swapchains) / sizeof(swapchains[0]);
//         presentInfo.swapchainCount = swapchainsLen;
//         presentInfo.pSwapchains = swapchains;
//         presentInfo.pImageIndices = &imageIdx;
//         presentInfo.pResults = nullptr;

//         // TODO: Needs different error handling!
//         vkQueuePresentKHR(g_presentQueue.queue, &presentInfo);
//     }

//     g_currentFrame = (g_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
// }

// void Renderer::resizeTarget(u32 width, u32 height) {
//     g_swapchainRecreate.height = height;
//     g_swapchainRecreate.width = width;
//     g_swapchainRecreate.recreate = true;
// }

// void Renderer::shutdown() {
//     logInfoTagged(RENDERER_TAG, "Destroying Synchronization objects");
//     for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
//         vkDestroySemaphore(g_device, g_imageAvailableSemaphores[i], nullptr);
//         vkDestroySemaphore(g_device, g_renderFinishedSemaphores[i], nullptr);
//         vkDestroyFence(g_device, g_inFlightFences[i], nullptr);
//     }

//     if (g_commandPool != VK_NULL_HANDLE) {
//         logInfoTagged(RENDERER_TAG, "Destroying Command Pool");
//         vkDestroyCommandPool(g_device, g_commandPool, nullptr);
//         g_commandPool = VK_NULL_HANDLE;
//     }

//     if (!g_swapchainFrameBuffers.empty()) {
//         logInfoTagged(RENDERER_TAG, "Destroying FrameBuffers");
//         for (addr_size i = 0; i < g_swapchainFrameBuffers.len(); i++) {
//             vkDestroyFramebuffer(g_device, g_swapchainFrameBuffers[i], nullptr);
//         }
//         g_swapchainFrameBuffers.free();
//     }

//     Swapchain::destroy(g_device, g_swapchain);
//     RenderPipeline::destroy(g_device, g_renderPipeline);
// }

// namespace {

// core::expected<FrameBufferList, AppError> createFrameBuffers(VkDevice logicalDevice,
//                                                              const Swapchain& swapchain,
//                                                              const RenderPipeline& pipeline) {
//     auto ret = FrameBufferList(swapchain.imageViews.len(), VkFramebuffer{});

//     for (size_t i = 0; i < swapchain.imageViews.len(); i++) {
//         VkImageView attachments[] = {
//             swapchain.imageViews[i]
//         };

//         VkFramebufferCreateInfo framebufferInfo{};
//         framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//         framebufferInfo.renderPass = pipeline.renderPass;
//         framebufferInfo.attachmentCount = 1;
//         framebufferInfo.pAttachments = attachments;
//         framebufferInfo.width = swapchain.extent.width;
//         framebufferInfo.height = swapchain.extent.height;
//         framebufferInfo.layers = 1;

//         if (
//             VkResult vres = vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &ret[i]);
//             vres != VK_SUCCESS
//         ) {
//             return FAILED_TO_CREATE_VULKAN_FRAME_BUFFER_ERREXPR;
//         }
//     }

//     return ret;
// }

// core::expected<VkCommandPool, AppError> createCommandPool(VkDevice logicalDevice, const PickedGPUDevice& picked) {
//     VkCommandPoolCreateInfo poolCreateInfo{};
//     poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//     poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//     poolCreateInfo.queueFamilyIndex = picked.graphicsQueueIdx;

//     VkCommandPool ret;
//     if (
//         VkResult vres =vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &ret);
//         vres != VK_SUCCESS
//     ) {
//         return FAILED_TO_CREATE_VULKAN_COMMAND_POOL_ERREXPR;
//     }

//     return ret;
// }

// core::expected<core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>, AppError> createCommandBuffers(VkDevice logicalDevice, VkCommandPool pool) {
//     VkCommandBufferAllocateInfo allocCreateInfo{};
//     allocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//     allocCreateInfo.commandPool = pool;
//     allocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//     allocCreateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

//     core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmdBuffers;
//     if (vkAllocateCommandBuffers(logicalDevice, &allocCreateInfo, cmdBuffers.data()) != VK_SUCCESS) {
//         return FAILED_TO_ALLOCATE_VULKAN_COMMAND_BUFFER_ERREXPR;
//     }

//     return cmdBuffers;
// }

// core::expected<core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT>, AppError> createSemaphores(VkDevice logicalDevice) {
//     VkSemaphoreCreateInfo semaphoreCreateInfo{};
//     semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

//     core::ArrStatic<VkSemaphore, MAX_FRAMES_IN_FLIGHT> ret;
//     for (addr_size i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
//         if (
//             VkResult vres = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &ret[i]);
//             vres != VK_SUCCESS
//         ) {
//             return FAILED_TO_ALLOCATE_VULKAN_SEMAPHORE_ERREXPR;
//         }
//     }

//     return ret;
// }

// core::expected<core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT>, AppError> createFences(VkDevice logicalDevice) {
//     VkFenceCreateInfo fenceCreateInfo{};
//     fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//     fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

//     core::ArrStatic<VkFence, MAX_FRAMES_IN_FLIGHT> ret;
//     for (addr_size i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
//         if (
//             VkResult vres = vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &ret[i]);
//             vres != VK_SUCCESS
//         ) {
//             return FAILED_TO_ALLOCATE_VULKAN_FENCE_ERREXPR;
//         }
//     }

//     return ret;
// }

// void recordCommandBuffer(VkCommandBuffer cmdBuffer, u32 imageIdx,
//                          const RenderPipeline& renderPipeline,
//                          const FrameBufferList& frameBuffers,
//                          const Swapchain& swapchain) {
//     VkCommandBufferBeginInfo beginInfo{};
//     beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//     beginInfo.flags = 0;
//     beginInfo.pInheritanceInfo = nullptr;

//     Assert(vkBeginCommandBuffer(cmdBuffer, &beginInfo) == VK_SUCCESS);

//     VkRenderPassBeginInfo renderPassInfo{};
//     renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//     renderPassInfo.renderPass = renderPipeline.renderPass;
//     renderPassInfo.framebuffer = frameBuffers[imageIdx];
//     renderPassInfo.renderArea.offset = {0, 0};
//     renderPassInfo.renderArea.extent = swapchain.extent;

//     VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // BLACK
//     renderPassInfo.clearValueCount = 1;
//     renderPassInfo.pClearValues = &clearColor;

//     {
//         vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//         defer { vkCmdEndRenderPass(cmdBuffer); };

//         vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipeline.graphicsPipeline);

//         VkViewport viewport{};
//         viewport.x = 0.0f;
//         viewport.y = 0.0f;
//         viewport.width = f32(swapchain.extent.width);
//         viewport.height = f32(swapchain.extent.height);
//         viewport.minDepth = 0.0f;
//         viewport.maxDepth = 1.0f;
//         vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

//         VkRect2D scissor{};
//         scissor.offset = {0, 0};
//         scissor.extent = swapchain.extent;
//         vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

//         vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
//     }

//     Assert(vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS);
// }

// core::expected<ExtPropsList*, AppError> getAllSupportedInstExtensions(bool useCache) {
//     if (useCache && !g_allSupportedInstExts.empty()) {
//         return &g_allSupportedInstExts;
//     }

//     u32 extensionCount = 0;
//     if (VkResult vres = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
//         vres != VK_SUCCESS) {
//         return FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES_ERREXPR;
//     }

//     auto extList = ExtPropsList(extensionCount, VkExtensionProperties{});
//     if (VkResult vres = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extList.data());
//         vres != VK_SUCCESS) {
//         return FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES_ERREXPR;
//     }

//     g_allSupportedInstExts = std::move(extList);
//     return &g_allSupportedInstExts;
// }

// void logInstExtPropsList(const ExtPropsList& list) {
//     logInfoTagged(RENDERER_TAG, "Extensions (%llu)", list.len());
//     for (addr_size i = 0; i < list.len(); i++) {
//         logInfoTagged(RENDERER_TAG, "\t%s v%u", list[i].extensionName, list[i].specVersion);
//     }
// }

// bool checkSupportForInstExtension(const char* extensionName) {
//     auto res = getAllSupportedInstExtensions();
//     if (res.hasErr()) {
//         logWarnTagged(RENDERER_TAG, "Query for all supported extensions failed, reason: %s", res.err().toCStr());
//         return false;
//     }

//     const ExtPropsList& extensions = *res.value();
//     for (addr_size i = 0; i < extensions.len(); i++) {
//         if (core::memcmp(extensions[i].extensionName, extensionName, core::cstrLen(extensionName)) == 0) {
//             return true;
//         }
//     }

//     return false;
// }

// core::expected<LayerPropsList*, AppError> getAllSupportedInstLayers(bool useCache) {
//     if (useCache && !g_allSupportedInstLayers.empty()) {
//         return &g_allSupportedInstLayers;
//     }

//     u32 layerCount;
//     if (VkResult vres = vkEnumerateInstanceLayerProperties(&layerCount, nullptr); vres != VK_SUCCESS) {
//         return FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES_ERREXPR;
//     }

//     auto layList = LayerPropsList(layerCount, VkLayerProperties{});
//     if (VkResult vres = vkEnumerateInstanceLayerProperties(&layerCount, layList.data()); vres != VK_SUCCESS) {
//         return FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES_ERREXPR;
//     }

//     g_allSupportedInstLayers = std::move(layList);
//     return &g_allSupportedInstLayers;
// }

// void logInstLayersList(const LayerPropsList& list) {
//     logInfoTagged(RENDERER_TAG, "Layers (%llu)", list.len());
//     for (addr_size i = 0; i < list.len(); i++) {
//         logInfoTagged(RENDERER_TAG, "\tname: %s, description: %s, spec version: %u, impl version: %u",
//                       list[i].layerName, list[i].description, list[i].specVersion, list[i].implementationVersion);
//     }
// }

// bool checkSupportForInstLayer(const char* name) {
//     auto res = getAllSupportedInstLayers();
//     if (res.hasErr()) {
//         logWarnTagged(RENDERER_TAG, "Query for all supported layers failed, reason: %s", res.err().toCStr());
//         return false;
//     }

//     const LayerPropsList& layers = *res.value();
//     for (addr_size i = 0; i < layers.len(); i++) {
//         VkLayerProperties p = layers[i];
//         if (core::memcmp(p.layerName, name, core::cstrLen(name)) == 0) {
//             return true;
//         }
//     }

//     return false;
// }

// core::expected<AppError> getVulkanVersion(char out[VERSION_BUFFER_SIZE]) {
//     u32 version = 0;
//     if(VkResult vres = vkEnumerateInstanceVersion(&version); vres != VK_SUCCESS) {
//         return FAILED_TO_GET_VULKAN_VERSION_ERREXPR;
//     }

//     u32 n;

//     out = core::memcopy(out, "Vulkan v", core::cstrLen("Vulkan v"));
//     n = core::Unpack(core::intToCstr(VK_VERSION_MAJOR(version), out, VERSION_BUFFER_SIZE));
//     out[n] = '.';
//     out += n + 1;

//     n = core::Unpack(core::intToCstr(VK_VERSION_MINOR(version), out, VERSION_BUFFER_SIZE));
//     out[n] = '.';
//     out += n + 1;

//     n = core::Unpack(core::intToCstr(VK_VERSION_PATCH(version), out, VERSION_BUFFER_SIZE));
//     out[n] = '\0';
//     out += n;

//     return {};
// }

// core::expected<AppError> logVulkanVersion() {
//     char buff[VERSION_BUFFER_SIZE];
//     if (auto res = getVulkanVersion(buff); res.hasErr()) {
//         return res;
//     }
//     logInfo(buff);
//     return {};
// }

// } // namespace

namespace {

VulkanContext g_vkctx;

}

core::expected<AppError> Renderer::init(const RendererInitInfo& info) {
    core::setLoggerTag(VULKAN_VALIDATION_TAG, appLogTagsToCStr(VULKAN_VALIDATION_TAG));

    g_vkctx.device = core::Unpack(Device::create(info), "Failed to create a device");

    return {};
}

void Renderer::drawFrame() {

}

void Renderer::resizeTarget(u32 width, u32 height) {

}

void Renderer::shutdown() {
    // VK_MUST(vkDeviceWaitIdle(g_vkctx.device.logicalDevice));

    Device::destroy(g_vkctx.device);
}
