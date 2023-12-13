#pragma once

#include <fwd_decl.h>

namespace stlv {

struct PlatformState {
    void* internal;
};

bool initPlatform(PlatformState& state, const char* appName, i32 x, i32 y, i32 width, i32 height);
void destroyPlatform(PlatformState& state);

bool pollOsEvents(PlatformState& state);

} // namespace stlv
