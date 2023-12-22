#include <renderer/renderer_frontend.h>
#include <renderer/renderer_backend.h>

namespace stlv {

namespace {

RendererBackend g_backend;

}; // namespace

bool initRenderer(PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight) {
    logInfoTagged(LogTag::T_RENDERER, "Initializing renderer frontend.");
    bool ret = initRendererBE(g_backend, pltState, frameBufferWidth, frameBufferHeight);
    return ret;
}

void shutdownRenderer() {
    logInfoTagged(LogTag::T_RENDERER, "Shutting down renderer frontend.");
    shutdownRendererBE(g_backend);
}

void rendererOnResize(u32 width, u32 height) {
    rendererOnResizeBE(g_backend, width, height);
}

void rendererDrawFrame(RenderPacket& packet) {
    bool ret = beginFrameRendererBE(g_backend, packet.deltaTime);
    if (ret) {
        ret = endFrameRendererBE(g_backend, packet.deltaTime);
        Panic(ret, "[BUG] Failed to end frame.");
    }
}


} // namespace stlv
