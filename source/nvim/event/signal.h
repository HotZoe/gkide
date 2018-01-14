/// @file nvim/event/signal.h

#ifndef NVIM_EVENT_SIGNAL_H
#define NVIM_EVENT_SIGNAL_H

#include <uv.h>

#include "nvim/event/loop.h"

typedef struct signal_watcher_s signal_watcher_st;
typedef void (*signal_ft)(signal_watcher_st *watcher, int signum, void *data);
typedef void (*signal_close_cb)(signal_watcher_st *watcher, void *data);

struct signal_watcher_s
{
    uv_signal_t uv;
    void *data;
    signal_ft cb;
    signal_close_cb close_cb;
    multiqueue_st *events;
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/signal.h.generated.h"
#endif

#endif // NVIM_EVENT_SIGNAL_H
