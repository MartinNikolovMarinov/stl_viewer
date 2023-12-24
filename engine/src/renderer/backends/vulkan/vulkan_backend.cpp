#include <application/logger.h>
#include <renderer/renderer_backend.h>
#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

bool initRendererBackend(RendererBackend& backend, PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight) {
    logInfoTagged(LogTag::T_RENDERER, "Initializing Vulkan Backend...");
    logInfoTagged(LogTag::T_RENDERER, "Vulkan Backend Initialized SUCCESSFULLY.");
    return true;
}

void shutdownRendererBackend(RendererBackend& backend) {
    logInfoTagged(LogTag::T_RENDERER, "Renderer Vulkan Backend Shutting Down...");
}

void rendererOnResizeBackend(RendererBackend& backend, u32 width, u32 height) {
    logTraceTagged(LogTag::T_RENDERER, "Renderer Vulkan Backend Resizing to %ux%u.", width, height);
}

bool beginFrameRendererBackend(RendererBackend& backend, f64 deltaTime) {
    return true;
}

bool endFrameRendererBackend(RendererBackend& backend, f64 deltaTime) {
    return true;
}

} // namespace stlv

