/// @file nvim/event/stream.h

#ifndef NVIM_EVENT_STREAM_H
#define NVIM_EVENT_STREAM_H

#include <stdbool.h>
#include <stddef.h>

#include <uv.h>

#include "nvim/event/loop.h"
#include "nvim/rbuffer.h"

typedef struct stream_s stream_st;
/// Type of function called when the stream_st buffer is filled with data
///
/// @param stream
/// The stream_st instance
///
/// @param rbuffer
/// The associated ringbuf_st instance
///
/// @param count
/// Number of bytes to read. This must be respected if keeping
/// the order of events is a requirement. This is because events
/// may be queued and only processed later when more data is copied
/// into to the buffer, so one read may starve another.
///
/// @param data
/// User-defined data
///
/// @param eof
/// If the stream reached EOF.
typedef void (*stream_read_cb)(stream_st *stream,
                               ringbuf_st *buf,
                               size_t count,
                               void *data,
                               bool eof);

/// Type of function called when the stream_st
/// has information about a write request.
///
/// @param wstream  The stream_st instance
/// @param data     User-defined data
/// @param status   0 on success, anything else indicates failure
typedef void (*stream_write_cb)(stream_st *stream, void *data, int status);

/// Type of function called when the stream_st
/// has information about a write request.
///
/// @param wstream  The stream_st instance
/// @param data     User-defined data
typedef void (*stream_close_cb)(stream_st *stream, void *data);

struct stream_s
{
    union
    {
        uv_pipe_t pipe;
        uv_tcp_t tcp;
        uv_idle_t idle;
    } uv;

    uv_stream_t *uvstream;
    uv_buf_t uvbuf;
    ringbuf_st *buffer;
    uv_file fd;
    stream_read_cb read_cb;
    stream_write_cb write_cb;
    void *cb_data;
    stream_close_cb close_cb;
    stream_close_cb internal_close_cb;
    void *close_cb_data;
    void *internal_data;
    size_t fpos;
    size_t curmem;
    size_t maxmem;
    size_t pending_reqs;
    size_t num_bytes;
    bool closed;
    multiqueue_st *events;
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/stream.h.generated.h"
#endif

#endif // NVIM_EVENT_STREAM_H
