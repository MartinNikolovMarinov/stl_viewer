#include "app.h"

NO_MANGLE bool create(stlv::ApplicationState& appState) {
    appState.startWindowHeight = 600;
    appState.startWindowWidth = 800;
    appState.windowTitle = "Stereolithography (STL) Viewer";

    logInfoTagged(stlv::LogTag::T_APP, "Application created successfully.");
    return true;
}

NO_MANGLE void shutdown() {
    logInfoTagged(stlv::LogTag::T_APP, "Application shutdown successfully.");
}

NO_MANGLE bool update(stlv::ApplicationState& appState) {
    logInfoTagged(stlv::LogTag::T_APP, "%s: Update 2", appState.windowTitle);
    return true;
}
