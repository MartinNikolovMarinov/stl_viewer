#include <application/app_manager.h>
#include <application/logger.h>

namespace stlv {

bool initSubsystems(i32 argc, char** argv) {
    if (!initCore(argc, argv)) {
        // no logger available
        Assert(false, "Failed to initialize core system.");
        return false;
    }

    if (!initLoggingSystem(LogLevel::L_INPUT_TRACE)) {
        // no logger available
        Assert(false, "Failed to initialize logging system.");
        return false;
    }
    logInfo("Logging system initialized successfully.");

    logInfo("Application subsystems initialized successfully.");
    return true;
}

bool shutdownSubsystems() {
    logInfo("Shutting down application subsystems.");

    shutdownLoggingSystem(); // keep this last, assume no logger availabe after this
    return true;
}

} // namespace stlv
