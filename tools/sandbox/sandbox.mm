#include "./sandbox.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <vulkan/vulkan.h>
#import <vulkan/vulkan_macos.h>

int runSandbox() {
    @autoreleasepool {
        // Create the application and main event loop
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];

        NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                                       styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        [window setTitle:@"MoltenVK Vulkan Application"];
        [window makeKeyAndOrderFront:nil];

        // Configure the content view with a CAMetalLayer
        NSView *contentView = [window contentView];
        [contentView setWantsLayer:YES];
        contentView.layer = [CAMetalLayer layer];

        // Vulkan instance creation
        VkInstance instance;
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "MoltenVK Vulkan App";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
            NSLog(@"Failed to create Vulkan instance");
            return -1;
        }

        // Create a Vulkan surface for the window
        VkSurfaceKHR surface;
        VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        surfaceCreateInfo.pNext = NULL;
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.pView = (__bridge const void *)(contentView.layer);

        if (vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, NULL, &surface) != VK_SUCCESS) {
            NSLog(@"Failed to create Vulkan surface");
            vkDestroyInstance(instance, NULL);
            return -1;
        }

        // Gracefully handle the close button
        [window setReleasedWhenClosed:NO];
        [window setDelegate:[NSClassFromString(@"NSObject") new]];

        [app activateIgnoringOtherApps:YES];
        [app run];

        // Clean up Vulkan resources on shutdown
        vkDestroySurfaceKHR(instance, surface, NULL);
        vkDestroyInstance(instance, NULL);
    }
    return 0;
}
