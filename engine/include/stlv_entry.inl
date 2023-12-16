#include <init_core.h>

namespace stlv {

struct ApplicationState;

ApplicationState* getAppState();
bool initAppEngine(i32 argc, char** argv);
void shutdownAppEngine();
bool preMainLoop();
bool updateAppState(i32& retCode);

} // namespace stlv

// IMPORTANT:
// The following entry point functions must be defined by the application executable that links to the app engine:
extern bool createApp(stlv::ApplicationState* appState);
extern bool updateApp(stlv::ApplicationState* appState);
extern void shutdownApp();

i32 main(i32 argc, char** argv) {
    if (!stlv::initAppEngine(argc, argv)) {
        // Might not have a logger available
        return -1;
    }

    // TODO: createApp expects some configurations to be set and might fail if they are in an invalid state.
    //       This neseccitates a validation check before proceeding to start application engine.
    //       Defer this for now.
    if (!createApp(stlv::getAppState())) {
        Assert(false, "Failed to create application.")
        return -1;
    }

    if (!stlv::preMainLoop()) {
        Assert(false, "Application failed before starting the main loop.")
        return -1;
    }

    i32 retCode = 0;
    while (stlv::updateAppState(retCode)) {
        if (!updateApp(stlv::getAppState())) {
            break;
        }
    }

    shutdownApp();
    stlv::shutdownAppEngine();

    return retCode;
}
