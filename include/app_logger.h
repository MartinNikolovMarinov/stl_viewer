#pragma once

#include <basic.h>

// Logger tags for the different subsystems.
enum AppLogTags : u8 {
    APP_TAG = 0, // Default
    INPUT_EVENTS_TAG = 1,
    RENDERER_TAG = 2,
    VULKAN_VALIDATION_TAG = 3
};

constexpr core::StrView appLogTagsToCStr(AppLogTags t) {
    switch (t) {
        case APP_TAG:               return "APP"_sv;
        case INPUT_EVENTS_TAG:      return "INPUT_EVENTS"_sv;
        case RENDERER_TAG:          return "RENDERER"_sv;
        case VULKAN_VALIDATION_TAG: return "VK_VALIDATION"_sv;
    }

    return "UNKNOWN"_sv;
}
