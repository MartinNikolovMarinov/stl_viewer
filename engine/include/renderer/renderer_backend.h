#pragma once

#include <fwd_internal.h>

#if STLV_BACKEND_VULKAN
#include <renderer/backend/renderer_vulkan.h>
#endif

namespace stlv {

struct PlatformState;
struct RendererBackend;

bool initRendererBE(RendererBackend& backend, PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight);
void shutdownRendererBE(RendererBackend& backend);

void rendererOnResizeBE(RendererBackend& backend, u32 width, u32 height);

bool beginFrameRendererBE(RendererBackend& backend, f64 deltaTime);
bool endFrameRendererBE(RendererBackend& backend, f64 deltaTime);

} // namespace stlv
