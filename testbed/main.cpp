#include <stlv_entry.inl>
#include "app/app.h"

#if defined(STLV_HOT_RELOADING) && STLV_HOT_RELOADING == 1

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/inotify.h>

using CreateAppFn = bool (*)(stlv::ApplicationState& appState);
using UpdateAppFn = bool (*)(stlv::ApplicationState& appState);
using ShutdownAppFn = void (*)();

constexpr const char* libapphotPath = STLV_BINARY_PATH "libstlv_app_hot.so";
void* libapphot = nullptr;
CreateAppFn createfn = nullptr;
UpdateAppFn updatefn = nullptr;
ShutdownAppFn shutdownfn = nullptr;
i32 inotifyFd = -1;
char inotiftEventBuffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));

void closeDynLibraries() {
    if (libapphot) {
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

    createfn = reinterpret_cast<CreateAppFn>(dlsym(libapphot, "create"));
    if (!createfn) {
        closeDynLibraries();
        logErr("Failed to load create function from libapphot; reason: %s", dlerror());
        return;
    }

    shutdownfn = reinterpret_cast<ShutdownAppFn>(dlsym(libapphot, "shutdown"));
    if (!shutdownfn) {
        closeDynLibraries();
        logErr("Failed to load shutdown function from libapphot; reason: %s", dlerror());
        return;
    }

    updatefn = reinterpret_cast<UpdateAppFn>(dlsym(libapphot, "update"));
    if (!updatefn) {
        closeDynLibraries();
        logErr("Failed to load update function from libapphot; reason: %s", dlerror());
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

bool createApp(stlv::ApplicationState& appState) {
    loadDynLibraries();
    createWatcher();

    bool res = createfn(appState);
    return res;
}

bool updateApp(stlv::ApplicationState& appState) {
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
        return false;
    }

    bool ret = updatefn(appState);
    return ret;
}

void shutdownApp() {
    shutdownfn();

    closeDynLibraries();
    closeWatcher();
}

#else

bool createApp(stlv::ApplicationState& appState) {
    bool ret = create(appState);
    return ret;
}

bool updateApp(stlv::ApplicationState& appState) {
    bool ret = update(appState);
    return ret;
}

void shutdownApp() {
    shutdown();
}

#endif

