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

NO_MANGLE bool update(stlv::ApplicationState*) {
    // logInfoTagged(stlv::LogTag::T_APP, "%s: Update 2", appState->windowTitle);
    return true;
}
