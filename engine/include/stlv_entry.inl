#include <init_core.h>

extern void createApp();
extern void updateApp();
extern void shutdownApp();

namespace stlv {

bool initSubsystems(i32 argc, char** argv);

} // namespace stlv

i32 main(i32 argc, char** argv) {
    if (!stlv::initSubsystems(argc, argv)) {
        return -1;
    }

    createApp();

    while(true) {
        updateApp();
        core::threadingSleep(1000);
    }

    shutdownApp();

    return 0;
}
