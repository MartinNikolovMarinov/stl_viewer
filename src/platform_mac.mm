#include <platform.h>
#include <vulkan_include.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

namespace {

// Global variables
NSApplication* app = nullptr;
NSWindow* window = nullptr;
CAMetalLayer* metalLayer = nullptr;
bool g_isCocoaAppRunning = false;

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

void postEmptyEvent() {
    NSEvent* dummyEvent = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                             location:NSMakePoint(0, 0)
                                        modifierFlags:0
                                            timestamp:0
                                         windowNumber:0
                                              context:nil
                                              subtype:0
                                                data1:0
                                                data2:0];
    [NSApp postEvent:dummyEvent atStart:YES];
}

} // namespace


// Application Delegate
@interface ApplicationDelegate : NSObject <NSApplicationDelegate>
@end

@implementation ApplicationDelegate
// - (void)applicationDidResignActive:(NSNotification*)notification {
// }

// - (void)applicationDidBecomeActive:(NSNotification*)notification {
// }

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // NSLog(@"[ApplicationDelegate] DidFinishLaunching");

    // Register to observe for keyboard layout changes.
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(selectedKeyboardInputSourceChanged:)
                                                 name:NSTextInputContextKeyboardSelectionDidChangeNotification
                                               object:nil];
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // NSLog(@"[ApplicationDelegate] WillTerminate");

    // Unregister for keyboard layout change events.
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:NSTextInputContextKeyboardSelectionDidChangeNotification
                                                  object:nil];
}

- (void)applicationDidHide:(NSNotification *)notification {
    // TODO: Handle keyboard layout changes here. Might need to rebuild scancode tables here.
}
@end

// Window Delegate
@interface WindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation WindowDelegate
- (void)windowDidResize:(NSNotification*)notification {
    if (windowResizeCallbackMacOS) {
        NSRect frame = [window frame];
        windowResizeCallbackMacOS((i32)frame.size.width, (i32)frame.size.height);
    }
}

- (void)windowDidResignKey:(NSNotification*)notification {
    if (windowFocusCallbackMacOS) {
        windowFocusCallbackMacOS(false); // Window lost focus
    }
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    if (windowFocusCallbackMacOS) {
        windowFocusCallbackMacOS(true); // Window gained focus
    }
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    if (windowCloseCallbackMacOS) {
        windowCloseCallbackMacOS();
    }

    // This is necessary when the event polling is in blocking mode. This triggers a dummy event to wakeup the thread.
    g_isCocoaAppRunning = false;
    postEmptyEvent();

    return NO; // Allow the window to close
}

// - (void)windowDidMove:(NSNotification *)notification {
// }

// - (void)windowDidMiniaturize:(NSNotification *)notification {
// }

// - (void)windowDidDeminiaturize:(NSNotification *)notification {
// }
@end

// Custom NSView subclass to handle keyboard events
@interface AppView : NSView {
    NSTrackingArea* _trackingArea;
}
@end

@implementation AppView

+ (Class)layerClass {
    // Tells Cocoa that this view’s backing layer should be a CAMetalLayer
    return [CAMetalLayer class];
}

// By default, NSView might refuse to become first responder.
// Override this to allow the view to receive key events.
- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    if (keyCallbackMacOS) {
        int vkcode = [event keyCode];
        int scancode = 0; // TODO: Map to my own scancodes.
        KeyboardModifiers mods = getModifiersOSX(event);
        keyCallbackMacOS(true, vkcode, scancode, mods);
    }
}

- (void)keyUp:(NSEvent*)event {
    if (keyCallbackMacOS) {
        int vkcode = [event keyCode];
        int scancode = 0; // TODO: Map to my own scancodes.
        KeyboardModifiers mods = getModifiersOSX(event);
        keyCallbackMacOS(false, vkcode, scancode, mods);
    }
}

- (void)flagsChanged:(NSEvent*)event {
    // TODO: to make this work, I will need a virtual keyboard with all currently pressed buttons.
}

// Called when the system thinks the tracking areas need updating
- (void)updateTrackingAreas {
    [super updateTrackingAreas];

    // Remove the old tracking area if it exists
    if (_trackingArea != nil) {
        [self removeTrackingArea:_trackingArea];
        _trackingArea = nil;
    }

    // Create new tracking area covering the entire view
    // NSTrackingInVisibleRect makes the tracking area automatically update
    // if the view is resized or moved, so we don't have to recalc the rect.
    NSTrackingAreaOptions options =
        (NSTrackingMouseEnteredAndExited |
         NSTrackingActiveAlways |
         NSTrackingInVisibleRect);

    _trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                 options:options
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

// These get called when the mouse enters or leaves the tracking area
- (void)mouseEntered:(NSEvent*)event {
    if (mouseEnterOrLeaveCallbackMacOS) {
        NSPoint location = [event locationInWindow];
        NSRect frame = [self frame];
        // i32 x = (i32)location.x;
        // i32 y = (i32)(frame.size.height - location.y);
        mouseEnterOrLeaveCallbackMacOS(true);
    }
}

- (void)mouseExited:(NSEvent*)event {
    if (mouseEnterOrLeaveCallbackMacOS) {
        NSPoint location = [event locationInWindow];
        NSRect frame = [self frame];
        // i32 x = (i32)location.x;
        // i32 y = (i32)(frame.size.height - location.y);
        mouseEnterOrLeaveCallbackMacOS(false);
    }
}

@end // AppView

AppError Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    @autoreleasepool {
        app = [NSApplication sharedApplication];

        // Set the application type to Regular GUI Application
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        // Disable press-and-hold accent
        NSDictionary* defaults = @{@"ApplePressAndHoldEnabled": @NO};
        [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];

        // Set up the application delegate
        ApplicationDelegate* appDelegate = [[ApplicationDelegate alloc] init];
        [app setDelegate:appDelegate];

        // Create window
        NSRect frame = NSMakeRect(0, 0, windowWidth, windowHeight);
        window = [[NSWindow alloc]
                  initWithContentRect:frame
                            styleMask:(NSWindowStyleMaskTitled |
                                       NSWindowStyleMaskClosable |
                                       NSWindowStyleMaskResizable |
                                       NSWindowStyleMaskMiniaturizable)
                              backing:NSBackingStoreBuffered
                                defer:NO];
        if (!window) {
            return createPltErr(PlatformError::Type::FAILED_TO_CREATE_OSX_WINDOW,
                                "Failed to create OSX window.");
        }

        // Set window title, center, make key/front
        NSString* nsTitle = [NSString stringWithUTF8String:windowTitle];
        [window setTitle:nsTitle];
        [window center];
        [window makeKeyAndOrderFront:nil];

        // Attach window delegate
        WindowDelegate* windowDelegate = [[WindowDelegate alloc] init];
        [window setDelegate:windowDelegate];

        // Create content view
        NSRect contentRect = [[window contentView] frame];
        AppView* contentView = [[AppView alloc] initWithFrame:contentRect];
        [contentView setWantsLayer:YES];
        [window setContentView:contentView];

        // Store a pointer to the Metal layer
        metalLayer = (CAMetalLayer*)[contentView layer];
        // metalLayer.device = MTLCreateSystemDefaultDevice();
        // metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        // metalLayer.framebufferOnly = YES;

        // Make content view first responder to receive key events
        [window makeFirstResponder:contentView];

        // Finish launching
        [NSApp finishLaunching]; // If you need it explicitly

        // Bring app to the front
        [NSApp activateIgnoringOtherApps:YES];

        // Mark Cocoa app running
        g_isCocoaAppRunning = true;

        return APP_OK;
    }
}

AppError Platform::pollEvents(bool block) {
    @autoreleasepool {
        while (g_isCocoaAppRunning) {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:block ? [NSDate distantFuture] : [NSDate date]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            if (!event) break;

            switch ([event type]) {
                case NSEventTypeMouseMoved: [[fallthrough]];
                case NSEventTypeLeftMouseDragged: [[fallthrough]];
                case NSEventTypeRightMouseDragged: {
                    if (mouseMoveCallbackMacOS) {
                        NSPoint location = [event locationInWindow];
                        NSRect frame = [window contentView].frame;
                        // Cocoa's coordinate system is flipped; we adjust Y accordingly.
                        i32 x = (i32)location.x;
                        i32 y = (i32)(frame.size.height - location.y);
                        mouseMoveCallbackMacOS(x, y);
                    }
                    break;
                }

                case NSEventTypeScrollWheel: {
                    if (mouseScrollCallbackMacOS) {
                        CGFloat deltaX = [event scrollingDeltaX];
                        CGFloat deltaY = [event scrollingDeltaY];
                        MouseScrollDirection direction = MouseScrollDirection::NONE;

                        if (deltaY > 0) direction = MouseScrollDirection::UP;
                        else if (deltaY < 0) direction = MouseScrollDirection::DOWN;

                        NSPoint location = [event locationInWindow];
                        i32 x = (i32)location.x;
                        i32 y = (i32)location.y;

                        mouseScrollCallbackMacOS(direction, x, y);
                    }
                    break;
                }

                case NSEventTypeLeftMouseDown: [[fallthrough]];
                case NSEventTypeLeftMouseUp: [[fallthrough]];
                case NSEventTypeRightMouseDown: [[fallthrough]];
                case NSEventTypeRightMouseUp: [[fallthrough]];
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
                        NSRect frame = [window contentView].frame;
                        i32 x = (i32)location.x;
                        i32 y = (i32)(frame.size.height - location.y);

                        KeyboardModifiers mods = getModifiersOSX(event);
                        mouseClickCallbackMacOS(isPress, button, x, y, mods);
                    }
                    break;
                }

                default:
                    break;
            }

            [NSApp sendEvent:event];
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

        g_isCocoaAppRunning = false;
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
