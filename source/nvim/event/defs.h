/// @file nvim/event/defs.h

#ifndef NVIM_EVENT_DEFS_H
#define NVIM_EVENT_DEFS_H

#include <assert.h>
#include <stdarg.h>

#define EVENT_HANDLER_MAX_ARGC 6

typedef void (*argv_callback)(void **argv);

typedef struct event_msg_s
{
    argv_callback handler;
    void *argv[EVENT_HANDLER_MAX_ARGC];
} event_msg_st;

typedef void(*event_scheduler)(event_msg_st event, void *data);

#define VA_EVENT_INIT(event, h, a)                       \
    do                                                   \
    {                                                    \
        assert(a <= EVENT_HANDLER_MAX_ARGC);             \
        (event)->handler = h;                            \
        if(a)                                            \
        {                                                \
            va_list args;                                \
            va_start(args, a);                           \
            for(int i = 0; i < a; i++)                   \
            {                                            \
                (event)->argv[i] = va_arg(args, void *); \
            }                                            \
            va_end(args);                                \
        }                                                \
    } while(0)

static inline event_msg_st event_create(argv_callback cb, int argc, ...)
{
    assert(argc <= EVENT_HANDLER_MAX_ARGC);
    event_msg_st event;
    VA_EVENT_INIT(&event, cb, argc);
    return event;
}

#endif // NVIM_EVENT_DEFS_H
