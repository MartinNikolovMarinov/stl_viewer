#include <stlv_entry.inl>
#include "app/app.h"

#if defined(STLV_HOT_RELOADING) && STLV_HOT_RELOADING == 1

#include <application/logger.h> // This is an exeption. No other code from the engine should be used here.

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/inotify.h>

using InitFn = void (*)();
using ShutdownFn = void (*)();
using UpdateFn = void (*)();

constexpr const char* libapphotPath = STLV_BINARY_PATH "libstlv_app_hot.so";
void* libapphot = nullptr;
UpdateFn updatefn = nullptr;
InitFn initfn = nullptr;
ShutdownFn shutdownfn = nullptr;
i32 inotifyFd = -1;
char inotiftEventBuffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));

void closeDynLibraries() {
    if (updatefn) {
        if (dlclose(libapphot) != 0) {
            logErr("Failed to close libapphot; reason: %s", dlerror());
            return;
        }
    }
}

void loadDynLibraries() {
    logInfo("Hot reloading triggered");

    closeDynLibraries();

    libapphot = dlopen(libapphotPath, RTLD_LAZY);
    if (!libapphot) {
        logErr("Failed to load libapphot; reason: %s", dlerror());
        return;
    }

    updatefn = reinterpret_cast<UpdateFn>(dlsym(libapphot, "update"));
    if (!updatefn) {
        closeDynLibraries();
        logErr("Failed to load update function from libapphot; reason: %s", dlerror());
        return;
    }

    initfn = reinterpret_cast<InitFn>(dlsym(libapphot, "init"));
    if (!initfn) {
        closeDynLibraries();
        logErr("Failed to load init function from libapphot; reason: %s", dlerror());
        return;
    }

    shutdownfn = reinterpret_cast<ShutdownFn>(dlsym(libapphot, "shutdown"));
    if (!shutdownfn) {
        closeDynLibraries();
        logErr("Failed to load shutdown function from libapphot; reason: %s", dlerror());
        return;
    }
}

void closeWatcher() {
    if (inotifyFd != -1) {
        close(inotifyFd);
    }
}

void createWatcher() {
    logInfo("Creating hot reloading watcher");

    closeWatcher();

    inotifyFd = inotify_init1(IN_NONBLOCK);
    if (inotifyFd < 0) {
        logErr("Failed to create inotify instance; reason: %s", strerror(errno));
        return;
    }

    constexpr u32 watchedEvents = IN_ATTRIB | IN_MODIFY | IN_MOVE_SELF | IN_IGNORED;
    i32 wd = inotify_add_watch(inotifyFd, libapphotPath, watchedEvents);
    if (wd < 0) {
        logErr("Failed to add watch to inotify instance; reason: %s", strerror(errno));
        return;
    }
}

void createApp() {
    loadDynLibraries();
    createWatcher();

    initfn();
}

void updateApp() {
    addr_off len = read(inotifyFd, inotiftEventBuffer, sizeof(inotiftEventBuffer));
    if (len != -1) {
        bool shouldReload = false;
        bool recreateWatcher = false;

        char* iebPtr = inotiftEventBuffer;
        while (iebPtr < inotiftEventBuffer + len) {
            auto notifyEv = reinterpret_cast<struct inotify_event *>(iebPtr);

            if (notifyEv->mask & IN_ATTRIB) {
                // Attributes changed, reloading it is necessary.
                shouldReload = true;
                logInfo("Watcher: IN_ATTRIB");
            }
            if (notifyEv->mask & IN_MODIFY) {
                // The file was modified, reloading it is necessary.
                shouldReload = true;
                logInfo("Watcher: IN_MODIFY");
            }

            if (notifyEv->mask & IN_MOVE_SELF) {
                // If the files was moved the watcher is not longer valid, recreating it is necessary.
                logInfo("Watcher: IN_MOVE_SELF");
                recreateWatcher = true;
            }
            if (notifyEv->mask & IN_IGNORED) {
                // NOTE: If IN_DELETE_SELF is triggered IN_IGNORED is also triggered, so this handles that case.
                // The watcher was removed by the kernel, recreating it is necessary.
                logInfo("Watcher: IN_IGNORED");
                recreateWatcher = true;
            }

            iebPtr += sizeof(struct inotify_event) + notifyEv->len;
        }

        if (recreateWatcher) {
            createWatcher();
        }

        if (shouldReload) {
            loadDynLibraries();
        }
    }
    else if (errno != EAGAIN) {
        logErr("Failed to read from inotify instance; reason: %s", strerror(errno));
        return;
    }

    updatefn();
}

void shutdownApp() {
    shutdownfn();

    closeDynLibraries();
    closeWatcher();
}

#else

void createApp() {}

void updateApp() {
    update();
}

void shutdownApp() {}

#endif

