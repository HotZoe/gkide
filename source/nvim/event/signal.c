/// @file nvim/event/signal.c

#include <uv.h>

#include "nvim/event/loop.h"
#include "nvim/event/signal.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/signal.c.generated.h"
#endif

void signal_watcher_init(main_loop_st *loop,
                         signal_watcher_st *watcher,
                         void *data)
FUNC_ATTR_NONNULL_ARG(1)
FUNC_ATTR_NONNULL_ARG(2)
{
    uv_signal_init(&loop->uv, &watcher->uv);
    watcher->uv.data = watcher;
    watcher->data = data;
    watcher->cb = NULL;
    watcher->events = loop->fast_events;
}

void signal_watcher_start(signal_watcher_st *watcher, signal_ft cb, int signum)
FUNC_ATTR_NONNULL_ALL
{
    watcher->cb = cb;
    uv_signal_start(&watcher->uv, signal_watcher_cb, signum);
}

void signal_watcher_stop(signal_watcher_st *watcher)
FUNC_ATTR_NONNULL_ALL
{
    uv_signal_stop(&watcher->uv);
}

void signal_watcher_close(signal_watcher_st *watcher, signal_close_ft cb)
FUNC_ATTR_NONNULL_ARG(1)
{
    watcher->close_cb = cb;
    uv_close((uv_handle_t *)&watcher->uv, close_cb);
}

static void signal_event(void **argv)
{
    signal_watcher_st *watcher = argv[0];
    watcher->cb(watcher, watcher->uv.signum, watcher->data);
}

static void signal_watcher_cb(uv_signal_t *handle,
                              int FUNC_ARGS_UNUSED_MATCH(signum))
{
    signal_watcher_st *watcher = handle->data;
    CREATE_EVENT(watcher->events, signal_event, 1, watcher);
}

static void close_cb(uv_handle_t *handle)
{
    signal_watcher_st *watcher = handle->data;

    if(watcher->close_cb)
    {
        watcher->close_cb(watcher, watcher->data);
    }
}
