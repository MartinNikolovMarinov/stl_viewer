#include <platform.h>

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
