#include <platform.h>
#include <core_logger.h>

const char* keyModifiersToCptr(KeyboardModifiers m) {
    using KeyboardModifiers::MOD_NONE;
    using KeyboardModifiers::MOD_SHIFT;
    using KeyboardModifiers::MOD_CONTROL;
    using KeyboardModifiers::MOD_ALT;
    using KeyboardModifiers::MOD_SUPER;

    if (m == MOD_NONE) return "None";

    if (m == (MOD_SHIFT | MOD_CONTROL | MOD_ALT | MOD_SUPER)) return "Shift + Control + Alt + Super";
    if (m == (MOD_SHIFT | MOD_CONTROL | MOD_ALT))             return "Shift + Control + Alt";
    if (m == (MOD_SHIFT | MOD_CONTROL | MOD_SUPER))           return "Shift + Control + Super";
    if (m == (MOD_SHIFT | MOD_ALT | MOD_SUPER))               return "Shift + Alt + Super";
    if (m == (MOD_CONTROL | MOD_ALT | MOD_SUPER))             return "Control + Alt + Super";

    if (m == (MOD_SHIFT | MOD_CONTROL)) return "Shift + Control";
    if (m == (MOD_SHIFT | MOD_ALT))     return "Shift + Alt";
    if (m == (MOD_SHIFT | MOD_SUPER))   return "Shift + Super";
    if (m == (MOD_CONTROL | MOD_ALT))   return "Control + Alt";
    if (m == (MOD_CONTROL | MOD_SUPER)) return "Control + Super";
    if (m == (MOD_ALT | MOD_SUPER))     return "Alt + Super";

    if (m == MOD_SHIFT)   return "Shift";
    if (m == MOD_CONTROL) return "Control";
    if (m == MOD_ALT)     return "Alt";
    if (m == MOD_SUPER)   return "Super";

    return "Unknown";
}

void PlatformEvent::logTraceEv() {
    using EvType = PlatformEvent::Type;

    switch (this->type) {
        case EvType::WINDOW_CLOSE:
            logTrace("EV_TYPE: WINDOW_CLOSE");
            return;

        case EvType::WINDOW_RESIZE:
            logTrace("EV_TYPE: WINDOW_RESIZE (w=%d, h=%d)",
                     this->data.resize.width,
                     this->data.resize.height);
            return;

        case EvType::MOUSE_PRESS:
            logTrace("EV_TYPE: MOUSE_PRESS (x=%d, y=%d, button=%d)",
                     this->data.mouse.x,
                     this->data.mouse.y,
                     this->data.mouse.button);
            return;

        case EvType::MOUSE_SCROLL:
            logTrace("EV_TYPE: MOUSE_SCROLL (x=%d, y=%d, direction=%s)",
                     this->data.scroll.x,
                     this->data.scroll.y,
                     this->data.scroll.direction == MouseScrollDirection::UP ? "UP" : "DOWN");
            return;

        case EvType::MOUSE_RELEASE:
            logTrace("EV_TYPE: MOUSE_RELEASE (x=%d, y=%d, button=%d)",
                     this->data.mouse.x,
                     this->data.mouse.y,
                     this->data.mouse.button);
            return;

        case EvType::KEY_PRESS:
            logTrace("EV_TYPE: KEY_PRESS (raw=%d, scancode=%d, mods=%s)",
                     this->data.key.raw,
                     this->data.key.scancode,
                     keyModifiersToCptr(this->data.key.mods));
            return;

        case EvType::KEY_RELEASE:
            logTrace("EV_TYPE: KEY_RELEASE (raw=%d, scancode=%d, mods=%s)",
                     this->data.key.raw,
                     this->data.key.scancode,
                     keyModifiersToCptr(this->data.key.mods));
            return;

        case EvType::NOOP:
            logTrace("EV_TYPE: NOOP (Event Not Set By Platform)");
            return;

        case EvType::UNKNOWN:
            break;
    }

    logTrace("EV_TYPE: Unknown");
}
