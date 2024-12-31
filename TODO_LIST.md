# Intent for this document (note to self)

This document is for detailed explanation of missing features, listing of tradeoffs and specifics, which I am pushing
for consideration at a later stage when I have a deeper understanding of the problem/s.

This is not a replacement for a Trello board.

# Platform Leftovers

## Window Drag events should probably be handled.

There is a todo for this in the code.

```c++
using WindowDrapCallback = void (*)(i32 x, i32 y);
```

I am not entirely sure there is a need to do anything here. Vulkan swapchain probably does not need to be recreated
just because the window is dragged. It should have the same VkExtent. I need to verify this ofc.

## Other events that might be missing

There is an event for Reveal/Expose on X11 systems. I might need to handle that?
Should I care about windows beeing dragged on top of the STLV window?
Should clipping happen in that case?

Probably can safely ignore these questions for a while.

## Extend the platform API with a function for centent scaling factor.

It's important to note that in high-DPI environments, the logical size of the client area (measured in
device-independent pixels) may differ from its physical size (measured in actual pixels).

```c++
// Example implementation for Windows could be:
bool Platform::getContentScaling(f32& scaleX, f32& scaleY) {
    HDC hdc = GetDC(g_window);
    if (!hdc) {
        scaleX = scaleY = 1.0f; // Default scaling
        return false;
    }
    defer { ReleaseDC(g_window, hdc); };

    i32 dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    i32 dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

    // NOTE: USER_DEFAULT_SCREEN_DPI macro is available only ony Windows 10+
    constexpr i32 baseDpi = USER_DEFAULT_SCREEN_DPI; // Standard DPI
    scaleX = f32(dpiX) / baseDpi;
    scaleY = f32(dpiY) / baseDpi;

    return true;
}
```

