/// @file nvim/event/process.h

#ifndef NVIM_EVENT_PROCESS_H
#define NVIM_EVENT_PROCESS_H

#include "nvim/event/loop.h"
#include "nvim/event/rstream.h"
#include "nvim/event/wstream.h"

typedef enum process_type_e
{
    kProcessTypeUv,
    kProcessTypePty
} process_type_et;

typedef struct process_s process_st;
typedef void (*internal_process_ft)(process_st *proc);
typedef void (*process_exit_ft)(process_st *proc, int status, void *data);

struct process_s
{
    process_type_et type;
    main_loop_st *loop;
    void *data;
    int pid;
    int status;
    int refcount;

    // set to the hrtime of when
    // process_stop was called for the process.
    uint64_t stopped_time;
    const char *cwd;
    char **argv;
    stream_st *in;
    stream_st *out;
    stream_st *err;
    process_exit_ft cb;
    internal_process_ft internal_exit_cb;
    internal_process_ft internal_close_cb;

    bool closed;
    bool term_sent;
    bool detach;
    multiqueue_st *events;
};

static inline process_st process_init(main_loop_st *loop,
                                      process_type_et type,
                                      void *data)
{
    return (process_st) {
        .type = type,
        .data = data,
        .loop = loop,
        .events = NULL,
        .pid = 0,
        .status = 0,
        .refcount = 0,
        .stopped_time = 0,
        .cwd = NULL,
        .argv = NULL,
        .in = NULL,
        .out = NULL,
        .err = NULL,
        .cb = NULL,
        .closed = false,
        .term_sent = false,
        .internal_close_cb = NULL,
        .internal_exit_cb = NULL,
        .detach = false
    };
}

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/process.h.generated.h"
#endif

#endif // NVIM_EVENT_PROCESS_H
