#include <application/logger.h>
#include <renderer/renderer_backend.h>
#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

bool initRendererBackend(RendererBackend& backend, PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight) {
    return true;
}

void shutdownRendererBackend(RendererBackend& backend) {
}

void rendererOnResizeBackend(RendererBackend& backend, u32 width, u32 height) {
}

bool beginFrameRendererBackend(RendererBackend& backend, f64 deltaTime) {
    return true;
}

bool endFrameRendererBackend(RendererBackend& backend, f64 deltaTime) {
    return true;
}

} // namespace stlv

