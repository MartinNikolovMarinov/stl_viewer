#include <init_core.h>

extern void createApp();
extern void updateApp();
extern void shutdownApp();

i32 main(i32, char**, char**) {
    createApp();

    while(true) {
        updateApp();
        core::threadingSleep(1000);
    }

    shutdownApp();

    return 0;
}
