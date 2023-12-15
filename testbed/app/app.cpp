#include "app.h"

NO_MANGLE void init() {
    logInfoTagged(stlv::LogTag::T_APP, "Application initialized successfully.");
}

NO_MANGLE void shutdown() {
    logInfoTagged(stlv::LogTag::T_APP, "Application shutdown successfully.");
}

NO_MANGLE void update() {
    logInfoTagged(stlv::LogTag::T_APP, "Update 1");
}
