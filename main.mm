#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#define VK_USE_PLATFORM_MACOS_MVK
#include <vulkan/vulkan.h>
#include <iostream>

// Custom AppDelegate
@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow* window;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    [NSApp stop:nil]; // Exit the app loop for this minimal example
}
@end

// Create a Vulkan-compatible macOS window
NSWindow* createVulkanWindow(NSApplication* app, int width, int height) {
    NSRect frame = NSMakeRect(0, 0, width, height);

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:(NSWindowStyleMaskTitled |
                             NSWindowStyleMaskClosable |
                             NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    [window setTitle:@"Vulkan Window"];
    [window center];
    [window makeKeyAndOrderFront:nil];
    return window;
}

int main() {
    // Vulkan Initialization
    uint32_t version = 0;
    VkResult result = vkEnumerateInstanceVersion(&version);
    if (result == VK_SUCCESS) {
        std::cout << "Vulkan version: " << VK_VERSION_MAJOR(version) << "."
                  << VK_VERSION_MINOR(version) << "."
                  << VK_VERSION_PATCH(version) << "\n";
    } else {
        std::cerr << "Failed to enumerate Vulkan version.\n";
        return -1;
    }

#ifdef __APPLE__
    setenv("VK_ICD_FILENAMES", "./lib/MoltenVK/sdk-1.3.296.0/MoltenVK_icd.json", 1);
    setenv("VK_LAYER_PATH", "./lib/MoltenVK/sdk-1.3.296.0/etc/vulkan/explicit_layer.d", 1);
#endif

    // Cocoa Application
    NSApplication* app = [NSApplication sharedApplication];
    AppDelegate* delegate = [[AppDelegate alloc] init];
    [app setDelegate:delegate];

    // Create Vulkan-compatible window
    NSWindow* window = createVulkanWindow(app, 800, 600);

    // Add CAMetalLayer to window contentView
    NSView* contentView = [window contentView];
    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    [contentView setLayer:metalLayer];
    [contentView setWantsLayer:YES];

    // Vulkan Instance Creation
    VkInstance instance;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Native Example";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_MVK_MACOS_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = 2;
    instanceCreateInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance.\n";
        return -1;
    }

    std::cout << "Vulkan instance successfully created.\n";

    // Create macOS surface
    VkSurfaceKHR surface;
    VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    surfaceCreateInfo.pView = (__bridge void*)contentView;

    if (vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan surface.\n";
        vkDestroyInstance(instance, nullptr);
        return -1;
    }

    std::cout << "Vulkan surface successfully created.\n";

    // Simulate a simple event loop
    while (YES) {
        NSEvent* event = [app nextEventMatchingMask:NSEventMaskAny
                                          untilDate:nil
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES];
        if (event) {
            [app sendEvent:event];
        }
    }

    // Cleanup
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    return 0;
}
