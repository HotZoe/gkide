/// @file nvim/event/rstream.c

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <uv.h>

#include "nvim/event/rstream.h"
#include "nvim/ascii.h"
#include "nvim/error.h"
#include "nvim/nvim.h"
#include "nvim/memory.h"
#include "nvim/log.h"
#include "nvim/misc1.h"
#include "nvim/event/loop.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/rstream.c.generated.h"
#endif

void rstream_init_fd(main_loop_st *loop,
                     stream_st *stream,
                     int fd,
                     size_t bufsize)
FUNC_ATTR_NONNULL_ARG(1)
FUNC_ATTR_NONNULL_ARG(2)
{
    stream_init(loop, stream, fd, NULL);
    rstream_init(stream, bufsize);
}

void rstream_init_stream(stream_st *stream,
                         uv_stream_t *uvstream,
                         size_t bufsize)
FUNC_ATTR_NONNULL_ARG(1)
FUNC_ATTR_NONNULL_ARG(2)
{
    stream_init(NULL, stream, -1, uvstream);
    rstream_init(stream, bufsize);
}

void rstream_init(stream_st *stream, size_t bufsize)
FUNC_ATTR_NONNULL_ARG(1)
{
    stream->buffer = rbuffer_new(bufsize);
    stream->buffer->data = stream;
    stream->buffer->full_cb = on_rbuffer_full;
    stream->buffer->nonfull_cb = on_rbuffer_nonfull;
}


/// Starts watching for events from a stream_st instance.
///
/// @param stream   The stream_st instance
void rstream_start(stream_st *stream, stream_read_ft cb, void *data)
FUNC_ATTR_NONNULL_ARG(1)
{
    stream->read_cb = cb;
    stream->cb_data = data;

    if(stream->uvstream)
    {
        uv_read_start(stream->uvstream, alloc_cb, read_cb);
    }
    else
    {
        uv_idle_start(&stream->uv.idle, fread_idle_cb);
    }
}

/// Stops watching for events from a stream_st instance.
///
/// @param stream The stream_st instance
void rstream_stop(stream_st *stream)
FUNC_ATTR_NONNULL_ALL
{
    if(stream->uvstream)
    {
        uv_read_stop(stream->uvstream);
    }
    else
    {
        uv_idle_stop(&stream->uv.idle);
    }
}

static void on_rbuffer_full(ringbuf_st *FUNC_ARGS_UNUSED_MATCH(buf), void *data)
{
    rstream_stop(data);
}

static void on_rbuffer_nonfull(ringbuf_st *FUNC_ARGS_UNUSED_MATCH(buf),
                               void *data)
{
    stream_st *stream = data;
    assert(stream->read_cb);
    rstream_start(stream, stream->read_cb, stream->cb_data);
}

/// Callbacks used by libuv
///
/// Called by libuv to allocate memory for reading.
static void alloc_cb(uv_handle_t *handle,
                     size_t FUNC_ARGS_UNUSED_MATCH(suggested),
                     uv_buf_t *buf)
{
    stream_st *stream = handle->data;

    // 'uv_buf_t.len' happens to have different size on Windows.
    size_t write_count;
    buf->base = rbuffer_write_ptr(stream->buffer, &write_count);

#ifdef HOST_OS_WINDOWS
    buf->len = (ULONG) write_count;
#else
    buf->len = write_count;
#endif
}

/// Callback invoked by libuv after it copies the data into the buffer
/// provided by `alloc_cb`. This is also called on EOF or when `alloc_cb`
/// returns a 0-length buffer.
static void read_cb(uv_stream_t *uvstream,
                    ssize_t cnt,
                    const uv_buf_t *FUNC_ARGS_UNUSED_MATCH(buf))
{
    stream_st *stream = uvstream->data;

    if(cnt > 0)
    {
        stream->num_bytes += (size_t)cnt;
    }

    if(cnt <= 0)
    {
        // cnt == 0 means libuv asked for a buffer and decided it wasn't needed:
        // http://docs.libuv.org/en/latest/stream.html#c.uv_read_start.
        //
        // We don't need to do anything with the ringbuf_st because the next call
        // to `alloc_cb` will return the same unused pointer(`rbuffer_produced`
        // won't be called)
        if(cnt != UV_ENOBUFS && cnt != 0)
        {
            DEBUG_LOG("Closing Stream (%p): %s (%s)",
                      stream,
                      uv_err_name((int)cnt),
                      os_strerror((int)cnt));

            // Read error or EOF, either way stop the stream
            // and invoke the callback with eof == true
            uv_read_stop(uvstream);
            invoke_read_cb(stream, 0, true);
        }

        return;
    }

    // at this point we're sure that cnt is positive, no error occurred
    size_t nread = (size_t)cnt;

    // Data was already written, so all we need is to update 'wpos' to reflect
    // the space actually used in the buffer.
    rbuffer_produced(stream->buffer, nread);
    invoke_read_cb(stream, nread, false);
}

/// Called by the by the `idle` handle to emulate a reading event
static void fread_idle_cb(uv_idle_t *handle)
{
    uv_fs_t req;
    stream_st *stream = handle->data;

    // 'uv_buf_t.len' happens to have different size on Windows.
    size_t write_count;
    stream->uvbuf.base = rbuffer_write_ptr(stream->buffer, &write_count);

#ifdef HOST_OS_WINDOWS
    stream->uvbuf.len = (ULONG) write_count;
#else
    stream->uvbuf.len = write_count;
#endif

    // the offset argument to uv_fs_read is int64_t, could someone really try
    // to read more than 9 quintillion (9e18) bytes?
    // upcast is meant to avoid tautological condition warning on 32 bits
    uintmax_t fpos_intmax = stream->fpos;

    if(fpos_intmax > INT64_MAX)
    {
        ERROR_LOG("stream offset overflow");
        preserve_exit(kNEStatusFailure);
    }

    // Synchronous read
    uv_fs_read(handle->loop,
               &req,
               stream->fd,
               &stream->uvbuf,
               1,
               (int64_t) stream->fpos,
               NULL);

    uv_fs_req_cleanup(&req);

    if(req.result <= 0)
    {
        uv_idle_stop(&stream->uv.idle);
        invoke_read_cb(stream, 0, true);
        return;
    }

    // no errors (req.result (ssize_t) is positive), it's safe to cast.
    size_t nread = (size_t)req.result;
    rbuffer_produced(stream->buffer, nread);
    stream->fpos += nread;
    invoke_read_cb(stream, nread, false);
}

static void read_event(void **argv)
{
    stream_st *stream = argv[0];

    if(stream->read_cb)
    {
        size_t count = (uintptr_t)argv[1];
        bool eof = (uintptr_t)argv[2];
        stream->read_cb(stream, stream->buffer, count, stream->cb_data, eof);
    }

    stream->pending_reqs--;

    if(stream->closed && !stream->pending_reqs)
    {
        stream_close_handle(stream);
    }
}

static void invoke_read_cb(stream_st *stream, size_t count, bool eof)
{
    // Don't let the stream be closed before the event is processed.
    stream->pending_reqs++;

    CREATE_EVENT(stream->events,
                 read_event,
                 3,
                 stream,
                 (void *)(uintptr_t *)count,
                 (void *)(uintptr_t)eof);
}
