#pragma once

#include <fwd_internal.h>

namespace stlv {

struct KeyboardKey;

struct PlatformState {
    void* internal; // platform specific data
};

struct PlatformStartInfo {
    i32 windowWidth;
    i32 windowHeight;
    const char* windowTitle;
};

bool initPlt(PlatformState& pstate);
bool startPlt(const PlatformStartInfo& pstartInfo, PlatformState& pstate);
void shutdownPlt(PlatformState& pstate);

bool pltPollEvents(PlatformState& pstate, f64 timeoutSeconds = -1);

/**
 * @brief Get the time since the application was launched.
 *        Uses the monotonic high resolution clock available on the platform.
 *
 * @return f64 The current time in seconds.
*/
f64 pltGetMonotinicTime();

bool pltGetKey(i32 pltKeyCode, KeyboardKey& key);
bool pltGetKey(i32 pltKeyCode, bool& isLeft, bool& isMiddle, bool& isRight);

void pltGetRequiredExtensionNames(ExtensionNames& names);

} // namespace stlv

