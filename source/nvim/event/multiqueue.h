/// @file nvim/event/multiqueue.h

#ifndef NVIM_EVENT_MULTIQUEUE_H
#define NVIM_EVENT_MULTIQUEUE_H

#include <uv.h>

#include "nvim/event/defs.h"
#include "nvim/lib/queue.h"

typedef struct multiqueue_s  multiqueue_st;
typedef void (*put_callback_ft)(multiqueue_st *multiq, void *data);

#define multiqueue_put(q, h, ...) \
    multiqueue_put_event(q, event_create(h, __VA_ARGS__));

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/multiqueue.h.generated.h"
#endif

#endif // NVIM_EVENT_MULTIQUEUE_H
