#include <platform.h>
#include <vulkan_include.h>

#include <wayland-client.h>
#include <cstring>

// Globals
static wl_display* g_display = nullptr;
static wl_registry* g_registry = nullptr;
static wl_compositor* g_compositor = nullptr;
static wl_surface* g_surface = nullptr;

// Registry listener callbacks
static void registry_handle_global(void* data, wl_registry* registry, u32 name,
                                   const char* interface, u32 version) {
    if (std::strcmp(interface, "wl_compositor") == 0) { // TODO: [REPLACE_WITH_CORE_IMPL]
        g_compositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    }
}

static void registry_handle_global_remove(void* data, wl_registry* registry, u32 name) {
    // Not needed for this minimal example
}

static const wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

// Platform Implementation

Platform::Error Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    // Connect to the Wayland display
    g_display = wl_display_connect(nullptr);
    if (!g_display) {
        std::cerr << "Failed to connect to Wayland display.\n"; // TODO: [REPLACE_WITH_CORE_IMPL]
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    // Get the registry and bind to compositor
    g_registry = wl_display_get_registry(g_display);
    wl_registry_add_listener(g_registry, &registry_listener, nullptr);
    wl_display_roundtrip(g_display);

    if (!g_compositor) {
        std::cerr << "Failed to get wl_compositor.\n"; // TODO: [REPLACE_WITH_CORE_IMPL]
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    // Create a surface
    g_surface = wl_compositor_create_surface(g_compositor);
    if (!g_surface) {
        std::cerr << "Failed to create wl_surface.\n"; // TODO: [REPLACE_WITH_CORE_IMPL]
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    // In a full app, you'd use xdg_wm_base to create a window and set title
    // For this minimal example, we have a surface but no window decorations.
    std::cout << "Wayland surface created. No shell integration for title: " << windowTitle << "\n"; // TODO: [REPLACE_WITH_CORE_IMPL]

    return Platform::Error::SUCCESS;
}

Platform::Error Platform::start() {
    // Minimal event loop: dispatch Wayland events indefinitely
    // Without shell support, we have no close events or title handling.
    while (true) {
        if (wl_display_dispatch(g_display) == -1) {
            // Display disconnected or error occurred
            break;
        }
    }

    // Cleanup
    if (g_surface) {
        wl_surface_destroy(g_surface);
        g_surface = nullptr;
    }
    if (g_compositor) {
        // wl_compositor is a global object managed by the registry, no direct destroy
    }
    if (g_registry) {
        wl_registry_destroy(g_registry);
        g_registry = nullptr;
    }
    if (g_display) {
        wl_display_disconnect(g_display);
        g_display = nullptr;
    }

    return Platform::Error::SUCCESS;
}

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char** extensions) {
    extensions[0] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
}

Platform::Error Platform::createVulkanSurface(VkInstance instance, VkSurfaceKHR& outSurface) {
    VkWaylandSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = g_display;
    createInfo.surface = g_surface;

    VkResult vres = vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &outSurface);
    if (vres != VK_SUCCESS) {
        std::cerr << "Failed to create Wayland Vulkan surface.\n"; // TODO: [REPLACE_WITH_CORE_IMPL]
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    return Platform::Error::SUCCESS;
}
