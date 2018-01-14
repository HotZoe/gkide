/// @file nvim/event/time.c

#include <stdint.h>

#include <uv.h>

#include "nvim/event/loop.h"
#include "nvim/event/time.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/time.c.generated.h"
#endif

void time_watcher_init(main_loop_st *loop, time_watcher_st *watcher, void *data)
FUNC_ATTR_NONNULL_ARG(1)
FUNC_ATTR_NONNULL_ARG(2)
{
    uv_timer_init(&loop->uv, &watcher->uv);
    watcher->uv.data = watcher;
    watcher->data = data;
    watcher->events = loop->fast_events;
    watcher->blockable = false;
}

void time_watcher_start(time_watcher_st *watcher,
                        time_cb cb,
                        uint64_t timeout,
                        uint64_t repeat)
FUNC_ATTR_NONNULL_ALL
{
    watcher->cb = cb;
    uv_timer_start(&watcher->uv, time_watcher_cb, timeout, repeat);
}

void time_watcher_stop(time_watcher_st *watcher)
FUNC_ATTR_NONNULL_ALL
{
    uv_timer_stop(&watcher->uv);
}

void time_watcher_close(time_watcher_st *watcher, time_cb cb)
FUNC_ATTR_NONNULL_ARG(1)
{
    watcher->close_cb = cb;
    uv_close((uv_handle_t *)&watcher->uv, close_cb);
}

static void time_event(void **argv)
{
    time_watcher_st *watcher = argv[0];
    watcher->cb(watcher, watcher->data);
}

static void time_watcher_cb(uv_timer_t *handle)
FUNC_ATTR_NONNULL_ALL
{
    time_watcher_st *watcher = handle->data;

    if(watcher->blockable && !multiqueue_empty(watcher->events))
    {
        // the timer blocked and there already
        // is an unprocessed event waiting
        return;
    }

    CREATE_EVENT(watcher->events, time_event, 1, watcher);
}

static void close_cb(uv_handle_t *handle)
{
    time_watcher_st *watcher = handle->data;

    if(watcher->close_cb)
    {
        watcher->close_cb(watcher, watcher->data);
    }
}
