#pragma once

#include <fwd_internal.h>

namespace stlv {

struct PlatformState;

bool initRenderer(PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight);
void shutdownRenderer();

void rendererOnResize(u32 width, u32 height);

struct RenderPacket {
    f64 deltaTime;
};

void rendererDrawFrame(RenderPacket& packet);

} // namespace stlv

#if STLV_BACKEND_VULKAN
#include <renderer/backend/renderer_vulkan.h>
#endif
