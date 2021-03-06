/// @file nvim/event/wstream.h

#ifndef NVIM_EVENT_WSTREAM_H
#define NVIM_EVENT_WSTREAM_H

#include <stdint.h>
#include <stdbool.h>

#include <uv.h>

#include "nvim/event/loop.h"
#include "nvim/event/stream.h"

typedef struct wbuffer_s wbuffer_st;
typedef void (*wbuffer_finalizer_ft)(void *data);

struct wbuffer_s
{
    size_t size;
    size_t refcount;
    char *data;
    wbuffer_finalizer_ft cb;
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/wstream.h.generated.h"
#endif

#endif // NVIM_EVENT_WSTREAM_H
