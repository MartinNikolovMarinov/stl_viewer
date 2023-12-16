#pragma once

#include <fwd_internal.h>

namespace stlv {

enum struct EventCode : u16 {
    NONE = 0,
    APP_QUIT,
    APP_KEY_DOWN,
    APP_KEY_UP,
    APP_MOUSE_DOWN,
    APP_MOUSE_UP,
    APP_MOUSE_SCROLL,
    APP_MOUSE_MOVE,
    APP_WINDOW_RESIZE,
    APP_WINDOW_MOVE,
    APP_WINDOW_FOCUS,
    APP_WINDOW_HIDDEN,

    SENTINEL
};

struct Event {
    union {
        i64 _i64[2];
        u64 _u64[2];

        f64 _f64[2];

        i32 _i32[4];
        u32 _u32[4];
        f32 _f32[4];

        i16 _i16[8];
        u16 _u16[8];

        i8 _i8[16];
        u8 _u8[16];

        char bytes[16];
    } data;
};

// Should return true if handled.
typedef bool (*OnEventHandler)(Event event, void* context);

bool initEventSystem();
void destroyEventSystem();

/**
 * @brief Registers an event handler for the given event code.
 *
 * @remark [NOT_THREAD_SAFE]
 *
 * @param code The event code.
 * @param ctx The event context.
 * @param handler The event handler.
 *
 * @return true If the event handler was registered successfully.
*/
bool eventRegister(EventCode code, void* context, OnEventHandler handler);

/**
 * @brief Unregisters an event handler for the given event code.
 *
 * @remark [NOT_THREAD_SAFE]
 *
 * @param code The event code.
 * @param ctx The event context.
 * @param handler The event handler.
 *
 * @return true If the event handler was unregistered successfully.
*/
bool eventUnregister(EventCode code, void* context, OnEventHandler handler);

/**
 * @brief Fires an event.
 *
 * @remark [NOT_THREAD_SAFE]
 *
 * @param code The event code.
 * @param event The event.
 *
 * @return true If the event was handled.
*/
bool eventFire(EventCode code, Event event);

/**
 * @brief Fires an event. Should not be used for events that are fired frequently.
 *
 * @remark [THREAD_SAFE]
 *
 * @param code The event code.
 * @param event The event.
 *
 * @return true If the event was handled.
*/
bool eventFireSlow(EventCode code, Event event);

} // namespace stlv
