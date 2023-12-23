#pragma once

#include <fwd_internal.h>

namespace stlv {

struct PlatformState;
struct RendererBackend;

void pltGetRequiredExtensionNames_vulkan(ExtensionNames& names);
bool pltCreateVulkanSurface_vulkan(PlatformState& pstate, RendererBackend& backend);

} // namespace stlv
