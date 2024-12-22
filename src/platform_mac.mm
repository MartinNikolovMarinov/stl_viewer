#include <platform.h>
#include <vulkan_include.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

namespace {

// Global variables
NSApplication* app = nullptr;
NSWindow* window = nullptr;
CAMetalLayer* metalLayer = nullptr;

// Callbacks
WindowCloseCallback windowCloseCallbackMacOS = nullptr;
WindowResizeCallback windowResizeCallbackMacOS = nullptr;
WindowFocusCallback windowFocusCallbackMacOS = nullptr;

KeyCallback keyCallbackMacOS = nullptr;

MouseClickCallback mouseClickCallbackMacOS = nullptr;
MouseMoveCallback mouseMoveCallbackMacOS = nullptr;
MouseScrollCallback mouseScrollCallbackMacOS = nullptr;
MouseEnterOrLeaveCallback mouseEnterOrLeaveCallbackMacOS = nullptr;

KeyboardModifiers getModifiersOSX(NSEvent* event) {
    KeyboardModifiers mods = KeyboardModifiers::MODNONE;
    NSEventModifierFlags flags = [event modifierFlags];

    if (flags & NSEventModifierFlagShift)   mods |= KeyboardModifiers::MODSHIFT;
    if (flags & NSEventModifierFlagControl) mods |= KeyboardModifiers::MODCONTROL;
    if (flags & NSEventModifierFlagOption)  mods |= KeyboardModifiers::MODALT;
    if (flags & NSEventModifierFlagCommand) mods |= KeyboardModifiers::MODSUPER;

    return mods;
}

} // namespace

@interface KeyHandlingView : NSView
@end

@implementation KeyHandlingView
- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    if (keyCallbackMacOS) {
        keyCallbackMacOS(true, (u32)[event keyCode], (u32)[event keyCode], getModifiersOSX(event));
    }
}

- (void)keyUp:(NSEvent*)event {
    if (keyCallbackMacOS) {
        keyCallbackMacOS(false, (u32)[event keyCode], (u32)[event keyCode], getModifiersOSX(event));
    }
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    // App is ready
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    if (windowCloseCallbackMacOS) {
        windowCloseCallbackMacOS();
    }
}
@end

AppError Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    @autoreleasepool {
        app = [NSApplication sharedApplication];
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];

        NSRect frame = NSMakeRect(0, 0, windowWidth, windowHeight);
        window = [[NSWindow alloc]
                  initWithContentRect:frame
                            styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                              backing:NSBackingStoreBuffered
                                defer:NO];
        if (!window) {
            return createPltErr(PlatformError::Type::FAILED_TO_CREATE_OSX_WINDOW, "Failed to create OSX window.");
        }
        NSString* nsTitle = [NSString stringWithUTF8String:windowTitle];
        [window setTitle:nsTitle];
        [window center];

        KeyHandlingView* keyView = [[KeyHandlingView alloc] initWithFrame:frame];
        [window setContentView:keyView];
        [window makeFirstResponder:keyView];
        [window makeKeyAndOrderFront:nil];

        metalLayer = [CAMetalLayer layer];
        if (!metalLayer) {
            return createPltErr(PlatformError::Type::FAILED_TO_CREATE_OSX_METAL_LAYER, "Failed to create Metal layer.");
        }
        [keyView setLayer:metalLayer];
        [keyView setWantsLayer:YES];

        [NSApp activateIgnoringOtherApps:YES];
    }
    return APP_OK;
}

AppError Platform::pollEvents(bool block) {
    @autoreleasepool {
        NSEvent* event;
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:block ? [NSDate distantFuture] : [NSDate date]
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES])) {
            [NSApp sendEvent:event];
            [NSApp updateWindows];

            switch ([event type]) {
                case NSEventTypeMouseMoved: [[fallthrough]];
                case NSEventTypeLeftMouseDragged: [[fallthrough]];
                case NSEventTypeRightMouseDragged: {
                    if (mouseMoveCallbackMacOS) {
                        NSPoint location = [event locationInWindow];
                        mouseMoveCallbackMacOS(i32(location.x), i32(location.y));
                    }
                    break;
                }

                case NSEventTypeScrollWheel: {
                    if (mouseScrollCallbackMacOS) {
                        MouseScrollDirection direction = MouseScrollDirection::NONE;
                        CGFloat deltaY = [event scrollingDeltaY];
                        if (deltaY > 0) direction = MouseScrollDirection::UP;
                        if (deltaY < 0) direction = MouseScrollDirection::DOWN;
                        NSPoint location = [event locationInWindow];
                        mouseScrollCallbackMacOS(direction, i32(location.x), i32(location.y));
                    }
                    break;
                }

                case NSEventTypeLeftMouseDown:  [[fallthrough]];
                case NSEventTypeLeftMouseUp:    [[fallthrough]];
                case NSEventTypeRightMouseDown: [[fallthrough]];
                case NSEventTypeRightMouseUp:   [[fallthrough]];
                case NSEventTypeOtherMouseDown: [[fallthrough]];
                case NSEventTypeOtherMouseUp: {
                    if (mouseClickCallbackMacOS) {
                        bool isPress = ([event type] == NSEventTypeLeftMouseDown ||
                                        [event type] == NSEventTypeRightMouseDown ||
                                        [event type] == NSEventTypeOtherMouseDown);
                        MouseButton button = MouseButton::NONE;
                        switch ([event buttonNumber]) {
                            case 0: button = MouseButton::LEFT; break;
                            case 1: button = MouseButton::RIGHT; break;
                            case 2: button = MouseButton::MIDDLE; break;
                        }
                        NSPoint location = [event locationInWindow];
                        KeyboardModifiers mods = getModifiersOSX(event);
                        mouseClickCallbackMacOS(isPress, button, i32(location.x), i32(location.y), mods);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    return APP_OK;
}

void Platform::shutdown() {
    @autoreleasepool {
        if (window) {
            [window close];
            window = nullptr;
        }
        if (app) {
            [app terminate:nil];
            app = nullptr;
        }
    }
}

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char** extensions) {
    extensions[0] = VK_EXT_METAL_SURFACE_EXTENSION_NAME;
}

AppError Platform::createVulkanSurface(VkInstance instance, VkSurfaceKHR& surface) {
    VkMetalSurfaceCreateInfoEXT metalSurfaceInfo{};
    metalSurfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    metalSurfaceInfo.pLayer = metalLayer;

    if (vkCreateMetalSurfaceEXT(instance, &metalSurfaceInfo, nullptr, &surface) != VK_SUCCESS) {
        return createPltErr(PlatformError::Type::FAILED_TO_CREATE_OSX_KHR_XLIB_SURFACE, "Failed to create Metal Vulkan surface.");
    }

    return APP_OK;
}

// Callback registration
void Platform::registerWindowCloseCallback(WindowCloseCallback cb) { windowCloseCallbackMacOS = cb; }
void Platform::registerWindowResizeCallback(WindowResizeCallback cb) { windowResizeCallbackMacOS = cb; }
void Platform::registerWindowFocusCallback(WindowFocusCallback cb) { windowFocusCallbackMacOS = cb; }

void Platform::registerKeyCallback(KeyCallback cb) { keyCallbackMacOS = cb; }

void Platform::registerMouseClickCallback(MouseClickCallback cb) { mouseClickCallbackMacOS = cb; }
void Platform::registerMouseMoveCallback(MouseMoveCallback cb) { mouseMoveCallbackMacOS = cb; }
void Platform::registerMouseScrollCallback(MouseScrollCallback cb) { mouseScrollCallbackMacOS = cb; }
void Platform::registerMouseEnterOrLeaveCallback(MouseEnterOrLeaveCallback cb) { mouseEnterOrLeaveCallbackMacOS = cb; }
