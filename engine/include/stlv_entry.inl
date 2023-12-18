#include <init_core.h>

namespace stlv {

struct ApplicationState;
struct AppCreateInfo;

ApplicationState* getAppState();
AppCreateInfo* getAppCreateInfo(ApplicationState* appState);
bool initAppEngine(i32 argc, char** argv);
void shutdownAppEngine();
bool preMainLoop();
bool updateAppState(i32& retCode);

} // namespace stlv

// IMPORTANT:
// The following entry point functions must be defined by the application executable that links to the app engine:
extern bool createApp(stlv::AppCreateInfo* createInfo);
extern bool updateApp(stlv::ApplicationState* appState);
extern void shutdownApp();

i32 main(i32 argc, char** argv) {
    if (!stlv::initAppEngine(argc, argv)) {
        // Might not have a logger available
        return -1;
    }

    if (!createApp(stlv::getAppCreateInfo(stlv::getAppState()))) {
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
