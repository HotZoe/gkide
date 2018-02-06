// Bridge for communication between a UI thread and nvim core.
// Used by the built-in TUI and libnvim-based UIs.
#ifndef NVIM_UI_BRIDGE_H
#define NVIM_UI_BRIDGE_H

#include <uv.h>

#include "nvim/ui.h"
#include "nvim/event/defs.h"

typedef struct ui_bridge_s ui_bridge_st;
typedef void(*ui_main_ft)(ui_bridge_st *bridge, ui_st *ui);

struct ui_bridge_s
{
    /// actual UI passed to nvim_ui_attach()
    ui_st bridge;

    /// UI pointer that will have its
    // callback called in another thread
    ui_st *ui;

    event_scheduler_ft scheduler;

    // thread handle
    // a handle is an abstract reference to a resource.
    uv_thread_t ui_thread;

    ui_main_ft ui_main;
    uv_mutex_t mutex;
    uv_cond_t cond;

    // When the UI thread is called, the main thread will suspend until
    // the call returns. This flag is used as a condition for the main
    // thread to continue.
    bool ready;

    // When a stop request is sent from the main thread, it must wait
    // until the UI thread finishes handling all events. This flag is
    // set by the UI thread as a signal that it will no longer send
    // messages to the main thread.
    bool stopped;
};

#define CONTINUE(b)                          \
    do                                       \
    {                                        \
        ui_bridge_st *d = (ui_bridge_st *)b; \
        uv_mutex_lock(&d->mutex);            \
        d->ready = true;                     \
        uv_cond_signal(&d->cond);            \
        uv_mutex_unlock(&d->mutex);          \
    } while(0)


#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "ui_bridge.h.generated.h"
#endif

#endif  // NVIM_UI_BRIDGE_H
