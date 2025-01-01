#include "./sandbox.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <vulkan/vulkan.h>
#import <vulkan/vulkan_metal.h>

int runSandbox() {
    @autoreleasepool {
        // Create the application and main event loop
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];

        NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                                       styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        [window setTitle:@"Vulkan with Metal Surface"];
        [window makeKeyAndOrderFront:nil];

        // Configure the content view with a CAMetalLayer
        NSView *contentView = [window contentView];
        [contentView setWantsLayer:YES];
        CAMetalLayer *metalLayer = [CAMetalLayer layer];
        contentView.layer = metalLayer;

        // Vulkan instance creation
        VkInstance instance;
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Metal Surface App";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        const char* instanceExtensions[] = {
            VK_EXT_METAL_SURFACE_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        };

        const char* instanceLayers[] = {
            "VK_LAYER_LUNARG_api_dump"
        };

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; // Enable portability
        instanceCreateInfo.enabledExtensionCount = sizeof(instanceExtensions) / sizeof(instanceExtensions[0]);
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions;
        instanceCreateInfo.enabledLayerCount = sizeof(instanceLayers) / sizeof(instanceLayers[0]);
        instanceCreateInfo.ppEnabledLayerNames = instanceLayers;

        if (vkCreateInstance(&instanceCreateInfo, NULL, &instance) != VK_SUCCESS) {
            NSLog(@"Failed to create Vulkan instance");
            return -1;
        }

        // Create Vulkan Metal surface
        VkSurfaceKHR surface;
        VkMetalSurfaceCreateInfoEXT surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        surfaceCreateInfo.pLayer = metalLayer;

        PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT =
            (PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(instance, "vkCreateMetalSurfaceEXT");

        if (!vkCreateMetalSurfaceEXT) {
            NSLog(@"vkCreateMetalSurfaceEXT not found");
            vkDestroyInstance(instance, NULL);
            return -1;
        }

        if (vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, NULL, &surface) != VK_SUCCESS) {
            NSLog(@"Failed to create Metal surface");
            vkDestroyInstance(instance, NULL);
            return -1;
        }

        // Query physical device
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

        if (deviceCount == 0) {
            NSLog(@"Failed to find GPUs with Vulkan support");
            vkDestroySurfaceKHR(instance, surface, NULL);
            vkDestroyInstance(instance, NULL);
            return -1;
        }

        VkPhysicalDevice physicalDevices[deviceCount];
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices);
        VkPhysicalDevice physicalDevice = physicalDevices[0];

        // Logical device creation
        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = 0; // Simplified; ideally query for suitable queue family
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        const char* requiredDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = sizeof(requiredDeviceExtensions) / sizeof(requiredDeviceExtensions[0]);
        deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions;

        VkDevice device;
        if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) != VK_SUCCESS) {
            NSLog(@"Failed to create Vulkan logical device");
            vkDestroySurfaceKHR(instance, surface, NULL);
            vkDestroyInstance(instance, NULL);
            return -1;
        }

        // Create a swapchain
        VkSwapchainKHR swapchain;
        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = 2;
        swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainCreateInfo.imageExtent = {800, 600};
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchainCreateInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, NULL, &swapchain) != VK_SUCCESS) {
            NSLog(@"Failed to create Vulkan swapchain");
            vkDestroyDevice(device, NULL);
            vkDestroySurfaceKHR(instance, surface, NULL);
            vkDestroyInstance(instance, NULL);
            return -1;
        }

        [app run]; // Start the main event loop

        // Clean up resources
        vkDestroySwapchainKHR(device, swapchain, NULL);
        vkDestroyDevice(device, NULL);
        vkDestroySurfaceKHR(instance, surface, NULL);
        vkDestroyInstance(instance, NULL);
    }
    return 0;
}
