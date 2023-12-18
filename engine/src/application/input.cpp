#include <application/input.h>
#include <platform/platform.h>

namespace stlv {

void keyboardClear(Keyboard& keyboard) {
    keyboard = {};
}

void keyboardUpdate(Keyboard& keyboard, i32 pltKeyCode, i32 scancode, bool isDown) {
    KeyboardKey key;
    bool ok = pltGetKey(pltKeyCode, key);
    Assert(ok, "Failed to get key info for key code.");

    key.isDown = isDown;
    key.scancode = scancode;
    keyboard.keys[addr_size(key.info)] = key;

    if (keyIsModifier(key.info)) {
        KeyboardModifiers modFlag;

        if (key.info == KeyInfo::KEY_LEFT_SHIFT || key.info == KeyInfo::KEY_RIGHT_SHIFT) {
            modFlag = KeyboardModifiers::SHIFT;
        }
        else if (key.info == KeyInfo::KEY_LEFT_CONTROL || key.info == KeyInfo::KEY_RIGHT_CONTROL) {
            modFlag = KeyboardModifiers::CTRL;
        }
        else if (key.info == KeyInfo::KEY_LEFT_ALT || key.info == KeyInfo::KEY_RIGHT_ALT) {
            modFlag = KeyboardModifiers::ALT;
        }

        if (isDown) {
            keyboard.mods = keyboard.mods | modFlag;
        } else {
            keyboard.mods = keyboard.mods & ~modFlag;
        }
    }
}

bool keyIsModifier(KeyInfo key) {
    bool ret = key == KeyInfo::KEY_LEFT_SHIFT    ||
               key == KeyInfo::KEY_LEFT_CONTROL  ||
               key == KeyInfo::KEY_LEFT_ALT      ||
               key == KeyInfo::KEY_RIGHT_SHIFT   ||
               key == KeyInfo::KEY_RIGHT_CONTROL ||
               key == KeyInfo::KEY_RIGHT_ALT;
    return ret;
}

} // namespace stlv
