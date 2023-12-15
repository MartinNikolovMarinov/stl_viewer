#include <stlv_entry.inl>

#if defined(STLV_HOT_RELOADING) && STLV_HOT_RELOADING == 1

#include <dlfcn.h>
#include <errno.h>
#include <iostream>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/inotify.h>

using UpdateFn = void (*)();

constexpr const char* libapphotPath = STLV_BINARY_PATH "libstlv_app_hot.so";
void* libapphot = nullptr;
UpdateFn updatefn = nullptr;
i32 inotifyFd = -1;
char inotiftEventBuffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));

void closeDynLibraries() {
    if (updatefn) {
        if (dlclose(libapphot) != 0) {
            std::cout << dlerror() << std::endl;
            return;
        }
    }
}

void loadDynLibraries() {
    closeDynLibraries();

    libapphot = dlopen(libapphotPath, RTLD_LAZY);
    if (!libapphot) {
        std::cout << dlerror() << std::endl;
        return;
    }

    updatefn = reinterpret_cast<UpdateFn>(dlsym(libapphot, "update"));
    if (!updatefn) {
        std::cout << dlerror() << std::endl;
        return;
    }
}

void closeWatcher() {
    if (inotifyFd != -1) {
        close(inotifyFd);
    }
}

void createWatcher() {
    closeWatcher();

    inotifyFd = inotify_init1(IN_NONBLOCK);
    if (inotifyFd < 0) {
        perror("inotify_init");
        return;
    }

    constexpr u32 watchedEvents = IN_ATTRIB | IN_MODIFY | IN_MOVE_SELF | IN_IGNORED;
    i32 wd = inotify_add_watch(inotifyFd, libapphotPath, watchedEvents);
    if (wd < 0) {
        perror("inotify_add_watch");
        return;
    }
}

void createApp() {
    loadDynLibraries();
    createWatcher();
}

void updateApp() {
    i32 len = read(inotifyFd, inotiftEventBuffer, sizeof(inotiftEventBuffer));
    if (len != -1) {
        bool shouldReload = false;
        bool recreateWatcher = false;

        char* iebPtr = inotiftEventBuffer;
        while (iebPtr < inotiftEventBuffer + len) {
            auto notifyEv = reinterpret_cast<struct inotify_event *>(iebPtr);

            if (notifyEv->mask & IN_ATTRIB) {
                // Attributes changed, reloading it is necessary.
                shouldReload = true;
                std::cout << "IN_ATTRIB" << std::endl;
            }
            if (notifyEv->mask & IN_MODIFY) {
                // The file was modified, reloading it is necessary.
                shouldReload = true;
                std::cout << "IN_MODIFY" << std::endl;
            }

            if (notifyEv->mask & IN_MOVE_SELF) {
                // If the files was moved the watcher is not longer valid, recreating it is necessary.
                std::cout << "IN_MOVE_SELF" << std::endl;
                recreateWatcher = true;
            }
            if (notifyEv->mask & IN_IGNORED) {
                // NOTE: If IN_DELETE_SELF is triggered IN_IGNORED is also triggered, so this handles that case.
                // The watcher was removed by the kernel, recreating it is necessary.
                std::cout << "IN_IGNORED" << std::endl;
                recreateWatcher = true;
            }

            iebPtr += sizeof(struct inotify_event) + notifyEv->len;
        }

        if (recreateWatcher) {
            std::cout << "Recreating watcher" << std::endl;
            createWatcher();
        }

        if (shouldReload) {
            std::cout << "Hot reloading triggered" << std::endl;
            loadDynLibraries();
        }
    }
    else if (errno != EAGAIN) {
        perror("read");
        return;
    }

    updatefn();
}

void shutdownApp() {
    closeDynLibraries();
    closeWatcher();
}

#else

#include "app/app.h"

void createApp() {}

void updateApp() {
    update();
}

void shutdownApp() {}

#endif

