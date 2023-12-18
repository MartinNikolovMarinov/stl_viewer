#pragma once

#include <fwd_internal.h>

#if STLV_BACKEND_VULKAN
#include <renderer/backend/renderer_vulkan.h>
#endif

namespace stlv {

struct PlatformState;
struct RendererBackend;

bool initRendererBE(RendererBackend& backend, PlatformState& pltState);
void shutdownRendererBE(RendererBackend& backend);

bool beginFrameRendererBE(RendererBackend& renderer, f64 deltaTime);
bool endFrameRendererBE(RendererBackend& renderer, f64 deltaTime);

} // namespace stlv
