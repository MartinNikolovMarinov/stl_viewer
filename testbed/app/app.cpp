#include "app.h"

NO_MANGLE bool create(stlv::AppCreateInfo* createInfo) {
    createInfo->startWindowHeight = 600;
    createInfo->startWindowWidth = 800;
    createInfo->windowTitle = "Stereolithography (STL) Viewer";

    logInfoTagged(stlv::LogTag::T_APP, "Application created successfully.");
    return true;
}

NO_MANGLE void shutdown() {
    logInfoTagged(stlv::LogTag::T_APP, "Application shutdown successfully.");
}

void keyboardStateToCptr(const stlv::Keyboard& keyboard, char* out) {
    out = core::cptrCopyUnsafe(out, "Keyboard State");
    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "{");

    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "\tKeys: ");
    for (addr_size i = 0; i < addr_size(stlv::KeyInfo::SENTINEL); ++i) {
        auto& key = keyboard.keys[i];
        if (key.isDown) {
            out = core::cptrCopyUnsafe(out, "\t");
            out = core::cptrCopyUnsafe(out, key.scancode);
            out = core::cptrCopyUnsafe(out, ", ");
        }
    }

    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "\tMods: ");
    if (keyboard.mods != 0) {
        if (bool(keyboard.mods & stlv::KeyboardModifiers::SHIFT)) {
            out = core::cptrCopyUnsafe(out, "SHIFT ");
        }
        if (bool(keyboard.mods & stlv::KeyboardModifiers::CTRL)) {
            out = core::cptrCopyUnsafe(out, "CTRL ");
        }
        if (bool(keyboard.mods & stlv::KeyboardModifiers::ALT)) {
            out = core::cptrCopyUnsafe(out, "ALT ");
        }
    }

    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "}");
    *out = '\0';
}

void printKeyboardState(const stlv::Keyboard& keyboard) {
    char buff[core::KILOBYTE * 2];
    keyboardStateToCptr(keyboard, buff);
    logTraceTagged(stlv::LogTag::T_APP, "%s", buff);
}

void printMetrics(stlv::CurrentFrameMetrics& metrics) {
    logInfoTagged(stlv::LogTag::T_APP, "Frame %llu: %f ms (%f fps)", metrics.frameCount, metrics.frameTime, metrics.fps);
}

NO_MANGLE bool update(stlv::ApplicationState* appState) {
    stlv::Keyboard& keyboard = appState->keyboard;
    printKeyboardState(keyboard);

    stlv::CurrentFrameMetrics& metrics = appState->metrics;
    printMetrics(metrics);

    return true;
}
