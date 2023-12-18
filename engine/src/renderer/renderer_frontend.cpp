#include <renderer/renderer_frontend.h>
#include <renderer/renderer_backend.h>

namespace stlv {

namespace {

RendererBackend backend;

}; // namespace

bool initRenderer(PlatformState& pltState) {
    bool ret = initRendererBE(backend, pltState);
    return ret;
}

void shutdownRenderer() {
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
