#pragma once

#include <fwd_internal.h>

#if STLV_BACKEND_VULKAN
#include <renderer/backends/vulkan/vulkan_backend.h>
#include <renderer/backends/vulkan/vulkan_platform.h>
#endif

namespace stlv {

struct PlatformState;
struct RendererBackend;

bool initRendererBackend(RendererBackend& backend, PlatformState& pltState, u32 frameBufferWidth, u32 frameBufferHeight);
void shutdownRendererBackend(RendererBackend& backend);

void rendererOnResizeBackend(RendererBackend& backend, u32 width, u32 height);

bool beginFrameRendererBackend(RendererBackend& backend, f64 deltaTime);
bool endFrameRendererBackend(RendererBackend& backend, f64 deltaTime);

} // namespace stlv
