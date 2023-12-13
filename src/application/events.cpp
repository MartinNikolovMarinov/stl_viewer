#include <application/events.h>
#include <application/logger.h>

namespace stlv {

namespace {

struct EventEntry {
    void* context;
    OnEventHandler handler;
};

struct EventSystemState {
    EventEntry entries[addr_size(EventCode::SENTINEL)];
};

EventSystemState state;
bool isInitialized = false;
core::Mutex eventMutex;

} // namespace

bool initEventSystem() {
    if (isInitialized) return false;

    auto res = core::mutexInit(eventMutex);
    if (res.hasErr()) {
        return false;
    }

    core::memset(&state, 0, sizeof(state));

    isInitialized = true;
    return true;
}

void destroyEventSystem() {}

bool eventRegister(EventCode code, void* context, OnEventHandler handler) {
    if (!isInitialized) {
        logWarn("Trying to register event handler without initializing the event system!");
        return false;
    }
    if (i32(code) <= i32(EventCode::NONE) || i32(code) >= i32(EventCode::SENTINEL)) {
        logWarn("Invalid event code: %d", i32(code));
        return false;
    }

    core::mutexLock(eventMutex);
    defer { core::mutexUnlock(eventMutex); };

    EventEntry& entry = state.entries[addr_size(code)];

    if (entry.handler == handler && entry.context == context) {
        logWarn("Event handler already registered for code: %d", i32(code));
        return false;
    }

    if (entry.handler) {
        // TODO: Might never be necessary.
        // If this is ever a problem, just extend the EventEntry members with a fixed size arrays.
        logWarn("Event system supports only one handler per event code!");
        return false;
    }

    entry.handler = handler;
    entry.context = context;

    return true;
}

bool eventUnregister(EventCode code, void* context, OnEventHandler handler) {
    if (!isInitialized) {
        logWarn("Trying to unregister event handler without initializing the event system!");
        return false;
    }
    if (i32(code) <= i32(EventCode::NONE) || i32(code) >= i32(EventCode::SENTINEL)) {
        logWarn("Invalid event code: %d", i32(code));
        return false;
    }

    core::mutexLock(eventMutex);
    defer { core::mutexUnlock(eventMutex); };

    EventEntry& entry = state.entries[addr_size(code)];

    if (!entry.handler) {
        logWarn("No event handler registered for code: %d", i32(code));
        return false;
    }

    if (entry.handler != handler || entry.context != context) {
        logWarn("Event handler registered for code: %d is not the same as the one trying to unregister!", i32(code));
        return false;
    }

    entry.handler = nullptr;
    entry.context = nullptr;

    return true;
}

bool eventFire(EventCode code, Event event) {
    if (!isInitialized) {
        logWarn("Trying to fire event without initializing the event system!");
        return false;
    }
    if (i32(code) <= i32(EventCode::NONE) || i32(code) >= i32(EventCode::SENTINEL)) {
        logWarn("Invalid event code: %d", i32(code));
        return false;
    }

    EventEntry& entry = state.entries[addr_size(code)];
    if (!entry.handler) {
        logWarn("No event handler registered for code: %d", i32(code));
        return false;
    }

    bool ret = entry.handler(code, event, entry.context);
    return ret;
}

bool eventFireSlow(EventCode code, Event event) {
    core::mutexLock(eventMutex);
    defer { core::mutexUnlock(eventMutex); };
    return eventFire(code, event);
}

} // namespace stlv
