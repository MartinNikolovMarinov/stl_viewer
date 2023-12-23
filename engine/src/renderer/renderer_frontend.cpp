#include <application/logger.h>
#include <renderer/renderer_frontend.h>
#include <renderer/renderer_backend.h>

namespace stlv {

struct PlatformState;

namespace {

RendererBackend g_backend;

} // namespace


bool initRenderer(PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight) {
    logInfoTagged(LogTag::T_RENDERER, "Initializing renderer frontend.");
    bool ret = initRendererBackend(g_backend, pltState, frameBufferWidth, frameBufferHeight);
    return ret;
}

void shutdownRenderer() {
    logInfoTagged(LogTag::T_RENDERER, "Shutting down renderer frontend.");
    shutdownRendererBackend(g_backend);
}

void rendererOnResize(u32 width, u32 height) {
    logTraceTagged(LogTag::T_RENDERER, "Frontend resizing to %ux%u.", width, height);
    rendererOnResizeBackend(g_backend, width, height);
}

void rendererDrawFrame(RenderPacket& packet) {
    bool ret = beginFrameRendererBackend(g_backend, packet.deltaTime);
    if (ret) {
        ret = endFrameRendererBackend(g_backend, packet.deltaTime);
        Panic(ret == true, "[BUG] Failed to end frame.");
    }
}

} // namespace stlv
