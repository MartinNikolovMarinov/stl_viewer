#include <platform.h>
#include <core_logger.h>

const char* keyModifiersToCptr(KeyboardModifiers m) {
    using KeyboardModifiers::MODNONE;
    using KeyboardModifiers::MODSHIFT;
    using KeyboardModifiers::MODCONTROL;
    using KeyboardModifiers::MODALT;
    using KeyboardModifiers::MODSUPER;

    if (m == MODNONE) return "None";

    if (m == (MODSHIFT | MODCONTROL | MODALT | MODSUPER))  return "Shift + Control + Alt + Super";
    if (m == (MODSHIFT | MODCONTROL | MODALT))             return "Shift + Control + Alt";
    if (m == (MODSHIFT | MODCONTROL | MODSUPER))           return "Shift + Control + Super";
    if (m == (MODSHIFT | MODALT | MODSUPER))               return "Shift + Alt + Super";
    if (m == (MODCONTROL | MODALT | MODSUPER))             return "Control + Alt + Super";

    if (m == (MODSHIFT | MODCONTROL)) return "Shift + Control";
    if (m == (MODSHIFT | MODALT))     return "Shift + Alt";
    if (m == (MODSHIFT | MODSUPER))   return "Shift + Super";
    if (m == (MODCONTROL | MODALT))   return "Control + Alt";
    if (m == (MODCONTROL | MODSUPER)) return "Control + Super";
    if (m == (MODALT | MODSUPER))     return "Alt + Super";

    if (m == MODSHIFT)   return "Shift";
    if (m == MODCONTROL) return "Control";
    if (m == MODALT)     return "Alt";
    if (m == MODSUPER)   return "Super";

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

        case EvType::MOUSE_RELEASE:
            logTrace("EV_TYPE: MOUSE_RELEASE (x=%d, y=%d, button=%d)",
                     this->data.mouse.x,
                     this->data.mouse.y,
                     this->data.mouse.button);
            return;

        case EvType::MOUSE_SCROLL_START:
            logTrace("EV_TYPE: MOUSE_SCROLL_START (x=%d, y=%d, direction=%s)",
                     this->data.scroll.x,
                     this->data.scroll.y,
                     this->data.scroll.direction == MouseScrollDirection::UP ? "UP" : "DOWN");
            return;

        case EvType::MOUSE_SCROLL_STOP:
            logTrace("EV_TYPE: MOUSE_SCROLL_STOP (x=%d, y=%d, direction=%s)",
                     this->data.scroll.x,
                     this->data.scroll.y,
                     this->data.scroll.direction == MouseScrollDirection::UP ? "UP" : "DOWN");
            return;

        case EvType::MOUSE_MOVE:
            logTrace("EV_TYPE: MOUSE_MOVE (x=%d, y=%d)", this->data.mouse.x, this->data.mouse.y);
            return;

        case EvType::MOUSE_ENTER:
            logTrace("EV_TYPE: MOUSE_ENTER (x=%d, y=%d)", this->data.mouse.x, this->data.mouse.y);
            return;

        case EvType::MOUSE_LEAVE:
            logTrace("EV_TYPE: MOUSE_LEAVE (x=%d, y=%d)", this->data.mouse.x, this->data.mouse.y);
            return;

        case EvType::KEY_PRESS:
            logTrace("EV_TYPE: KEY_PRESS (vkcode=%d, scancode=%d, mods=%s)",
                     this->data.key.vkcode,
                     this->data.key.scancode,
                     keyModifiersToCptr(this->data.key.mods));
            return;

        case EvType::KEY_RELEASE:
            logTrace("EV_TYPE: KEY_RELEASE (vkcode=%d, scancode=%d, mods=%s)",
                     this->data.key.vkcode,
                     this->data.key.scancode,
                     keyModifiersToCptr(this->data.key.mods));
            return;

        case EvType::FOCUS_GAINED:
            logTrace("EV_TYPE: FOCUS_GAINED");
            return;

        case EvType::FOCUS_LOST:
            logTrace("EV_TYPE: FOCUS_LOST");
            return;

        case EvType::NOOP:
            logTrace("EV_TYPE: NOOP (Event Not Set By Platform)");
            return;

        case EvType::UNKNOWN:
            break;
    }

    logTrace("EV_TYPE: Unknown");
}
