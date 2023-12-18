#pragma once

#include <fwd_internal.h>

#include <vulkan/vulkan.h>

namespace stlv {

struct RendererBackend {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
};

} // namespace stlv
