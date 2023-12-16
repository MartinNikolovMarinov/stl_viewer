#pragma once

#include <fwd_internal.h>

namespace stlv {

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

} // namespace stlv

