#include "app.h"

NO_MANGLE bool create(stlv::AppCreateInfo* createInfo) {
    createInfo->startWindowHeight = 600;
    createInfo->startWindowWidth = 800;
    createInfo->windowTitle = "Stereolithography (STL) Viewer";

    createInfo->capFrameRate = true;
    createInfo->targetFramesPerSecond = 61;

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
            out = core::cptrAppendInt(out, key.scancode);
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

char* cptrCopyUnsafeFloat(char* out, f64 value) {
    int ret = snprintf(out, 64, "%.2f", value);
    Assert(ret > 0, "Failed to convert float to string.");
    out += ret;
    return out;
}

void mouseStateToCptr(const stlv::Mouse& mouse, char* out) {
    out = core::cptrCopyUnsafe(out, "Mouse State");
    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "{");

    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "\tPosition: ");
    out = cptrCopyUnsafeFloat(out, mouse.x);
    out = core::cptrCopyUnsafe(out, ", ");
    out = cptrCopyUnsafeFloat(out, mouse.y);

    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "\tScroll: ");
    out = cptrCopyUnsafeFloat(out, mouse.scrollX);
    out = core::cptrCopyUnsafe(out, ", ");
    out = cptrCopyUnsafeFloat(out, mouse.scrollY);

    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "\tButtons: ");
    if (mouse.leftButtonIsDown) {
        out = core::cptrCopyUnsafe(out, "LEFT ");
    }
    if (mouse.middleButtonIsDown) {
        out = core::cptrCopyUnsafe(out, "MIDDLE ");
    }
    if (mouse.rightButtonIsDown) {
        out = core::cptrCopyUnsafe(out, "RIGHT ");
    }

    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "\tInside Window: ");
    out = core::cptrCopyUnsafe(out, mouse.insideWindow ? "true" : "false");

    out = core::cptrCopyUnsafe(out, "\n");
    out = core::cptrCopyUnsafe(out, "}");
    *out = '\0';
}

void printMouseState(const stlv::Mouse& mouse) {
    char buff[core::KILOBYTE];
    mouseStateToCptr(mouse, buff);
    logTraceTagged(stlv::LogTag::T_APP, "%s", buff);
}

#define STLV_MEMORY_METRIC_ENTRY_TO_CPTR_MACRO(buffname, aname) \
    buffname = core::cptrCopyUnsafe(buff, "\t"#aname": "); \
    buffname = core::cptrCopyUnsafe(buff, "{ IN_USE: "); \
    buffname = writeStorageValue(memoryMetrics.memoryInUse_##aname, buff); \
    buffname = core::cptrCopyUnsafe(buff, ", ALLOCATED: "); \
    buffname = writeStorageValue(memoryMetrics.memoryAllocated_##aname, buff); \
    buffname = core::cptrCopyUnsafe(buff, " }\n"); \

void memoryMetricsToCptr(stlv::MemoryMetrics& memoryMetrics, char* buff) {
    auto writeStorageValue = [](u64 value, char *_buff) -> char* {

        if (value < 1024) {
            _buff = cptrCopyUnsafeFloat(_buff, f64(value));
            _buff = core::cptrCopyUnsafe(_buff, " B");
        }
        else if (value < 1024 * 1024) {
            _buff = cptrCopyUnsafeFloat(_buff, f64(value) / 1024.0);
            _buff = core::cptrCopyUnsafe(_buff, " KB");
        }
        else if (value < 1024 * 1024 * 1024) {
            _buff = cptrCopyUnsafeFloat(_buff, f64(value) / 1024.0 / 1024.0);
            _buff = core::cptrCopyUnsafe(_buff, " MB");
        }
        else {
            _buff = cptrCopyUnsafeFloat(_buff, f64(value) / 1024.0 / 1024.0 / 1024.0);
            _buff = core::cptrCopyUnsafe(_buff, " GB");
        }

        return _buff;
    };

    buff = core::cptrCopyUnsafe(buff, "Memory Metrics");
    buff = core::cptrCopyUnsafe(buff, "\n");
    buff = core::cptrCopyUnsafe(buff, "{");
    buff = core::cptrCopyUnsafe(buff, "\n");

    STLV_MEMORY_METRIC_ENTRY_TO_CPTR_MACRO(buff, UNTAGGED);
    STLV_MEMORY_METRIC_ENTRY_TO_CPTR_MACRO(buff, PLATFORM);
    STLV_MEMORY_METRIC_ENTRY_TO_CPTR_MACRO(buff, RENDERER_BACKEND);

    buff = core::cptrCopyUnsafe(buff, "}");
    *buff = '\0';
}

void printMemoryMetrics(stlv::MemoryMetrics& memoryMetrics) {
    char buff[core::KILOBYTE];
    memoryMetricsToCptr(memoryMetrics, buff);
    logTraceTagged(stlv::LogTag::T_APP, "%s", buff);
}

NO_MANGLE bool update([[maybe_unused]] stlv::ApplicationState* appState) {
    // stlv::Keyboard& keyboard = appState->keyboard;
    // printKeyboardState(keyboard);

    stlv::CurrentFrameMetrics& metrics = appState->frameMetrics;
    printMetrics(metrics);

    // stlv::Mouse& mouse = appState->mouse;
    // printMouseState(mouse);

    // stlv::MemoryMetrics memoryMetrics = appState->memoryMetrics;
    // printMemoryMetrics(memoryMetrics);

    // stlv::stopApp();
    return true;
}
