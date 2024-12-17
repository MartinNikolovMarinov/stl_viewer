#include <platform.h>
#include <vulkan_include.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

static NSApplication* app;
static NSWindow* window;
static CAMetalLayer* metalLayer;

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow* window;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    [NSApp stop:nil]; // Exit the app loop for this minimal example
}
@end

using PlatformErr = Platform::Error;

PlatformErr Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    @autoreleasepool {
        app = [NSApplication sharedApplication];
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];

        // Create the window
        NSRect frame = NSMakeRect(0, 0, windowWidth, windowHeight);
        window = [[NSWindow alloc]
            initWithContentRect:frame
                      styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                        backing:NSBackingStoreBuffered
                          defer:NO];
        NSString* nsTitle = [NSString stringWithUTF8String:windowTitle];
        [window setTitle:nsTitle];
        [window center];
        [window makeKeyAndOrderFront:nil];

        // Set up Metal layer for Vulkan surface creation
        NSView* contentView = [window contentView];
        metalLayer = [CAMetalLayer layer];
        [contentView setLayer:metalLayer];
        [contentView setWantsLayer:YES];

        return PlatformErr::SUCCESS;
    }
}

PlatformErr Platform::start() {
    while (YES) {
        @autoreleasepool {
            NSEvent* event = [app nextEventMatchingMask:NSEventMaskAny
                                              untilDate:nil
                                                 inMode:NSDefaultRunLoopMode
                                                dequeue:YES];
            if (event) {
                [app sendEvent:event];
            }
        }
    }
    return PlatformErr::SUCCESS;
}

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char** extensions) {
    extensions[0] = VK_EXT_METAL_SURFACE_EXTENSION_NAME;
}

PlatformErr Platform::createVulkanSurface(VkInstance instance, VkSurfaceKHR surface) {
    VkMetalSurfaceCreateInfoEXT metalSurfaceInfo{};
    metalSurfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    metalSurfaceInfo.pLayer = metalLayer;

    if (vkCreateMetalSurfaceEXT(instance, &metalSurfaceInfo, nullptr, &surface) != VK_SUCCESS) {
        return PlatformErr::FAILED_TO_CREATE_SURFACE;
    }

    return PlatformErr::SUCCESS;
}
