/// @file nvim/event/time.h

#ifndef NVIM_EVENT_TIME_H
#define NVIM_EVENT_TIME_H

#include <uv.h>

#include "nvim/event/loop.h"

typedef struct time_watcher_s time_watcher_st;
typedef void (*time_cb)(time_watcher_st *watcher, void *data);
struct time_watcher_s
{
    uv_timer_t uv;
    void *data;
    time_cb cb;
    time_cb close_cb;
    multiqueue_st *events;
    bool blockable;
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/time.h.generated.h"
#endif

#endif // NVIM_EVENT_TIME_H
