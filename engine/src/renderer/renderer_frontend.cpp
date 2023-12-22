#include <renderer/renderer_frontend.h>
#include <renderer/renderer_backend.h>

namespace stlv {

namespace {

RendererBackend backend;

}; // namespace

bool initRenderer(PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight) {
    logInfoTagged(LogTag::T_RENDERER, "Initializing renderer frontend.");
    bool ret = initRendererBE(backend, pltState, frameBufferWidth, frameBufferHeight);
    return ret;
}

void shutdownRenderer() {
    logInfoTagged(LogTag::T_RENDERER, "Shutting down renderer frontend.");
    shutdownRendererBE(backend);
}

void rendererOnResize(i32, i32) {
    Assert(false, "TODO: Not implemented.");
}

void rendererDrawFrame(RenderPacket& packet) {
    bool ret = beginFrameRendererBE(backend, packet.deltaTime);
    if (ret) {
        ret = endFrameRendererBE(backend, packet.deltaTime);
        Panic(ret, "[BUG] Failed to end frame.");
    }
}


} // namespace stlv
