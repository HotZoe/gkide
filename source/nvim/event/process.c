/// @file nvim/event/process.c

#include <assert.h>
#include <stdlib.h>

#include <uv.h>

#include "nvim/os/shell.h"
#include "nvim/event/loop.h"
#include "nvim/event/rstream.h"
#include "nvim/event/wstream.h"
#include "nvim/event/process.h"
#include "nvim/event/libuv_process.h"
#include "nvim/os/pty_process.h"
#include "nvim/globals.h"
#include "nvim/macros.h"
#include "nvim/log.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/process.c.generated.h"
#endif

// Time (ns) for a process to exit cleanly before we send TERM/KILL.
#define TERM_TIMEOUT  1000000000
#define KILL_TIMEOUT  (TERM_TIMEOUT * 2)

#define CLOSE_PROC_STREAM(proc, stream)             \
    do                                              \
    {                                               \
        if(proc->stream && !proc->stream->closed)   \
        {                                           \
            stream_close(proc->stream, NULL, NULL); \
        }                                           \
    } while(0)

static bool process_is_tearing_down = false;

/// @returns zero on success, or negative error code
int process_spawn(process_st *proc)
FUNC_ATTR_NONNULL_ALL
{
    if(proc->in)
    {
        uv_pipe_init(&proc->loop->uv, &proc->in->uv.pipe, 0);
    }

    if(proc->out)
    {
        uv_pipe_init(&proc->loop->uv, &proc->out->uv.pipe, 0);
    }

    if(proc->err)
    {
        uv_pipe_init(&proc->loop->uv, &proc->err->uv.pipe, 0);
    }

    int status;

    switch(proc->type)
    {
        case kProcessTypeUv:
            status = libuv_process_spawn((libuv_process_st *)proc);
            break;

        case kProcessTypePty:
            status = pty_process_spawn((pty_process_st *)proc);
            break;

        default:
            abort();
    }

    if(status)
    {
        if(proc->in)
        {
            uv_close((uv_handle_t *)&proc->in->uv.pipe, NULL);
        }

        if(proc->out)
        {
            uv_close((uv_handle_t *)&proc->out->uv.pipe, NULL);
        }

        if(proc->err)
        {
            uv_close((uv_handle_t *)&proc->err->uv.pipe, NULL);
        }

        if(proc->type == kProcessTypeUv)
        {
            uv_close((uv_handle_t *)&(((libuv_process_st *)proc)->uv), NULL);
        }
        else
        {
            process_close(proc);
        }

        shell_free_argv(proc->argv);
        proc->status = -1;
        return status;
    }

    if(proc->in)
    {
        stream_init(NULL, proc->in, -1, STRUCT_CAST(uv_stream_t,
                                                    &proc->in->uv.pipe));

        proc->in->events = proc->events;
        proc->in->internal_data = proc;
        proc->in->internal_close_cb = on_process_stream_close;
        proc->refcount++;
    }

    if(proc->out)
    {
        stream_init(NULL, proc->out, -1, STRUCT_CAST(uv_stream_t,
                                                     &proc->out->uv.pipe));

        proc->out->events = proc->events;
        proc->out->internal_data = proc;
        proc->out->internal_close_cb = on_process_stream_close;
        proc->refcount++;
    }

    if(proc->err)
    {
        stream_init(NULL, proc->err, -1, STRUCT_CAST(uv_stream_t,
                                                     &proc->err->uv.pipe));

        proc->err->events = proc->events;
        proc->err->internal_data = proc;
        proc->err->internal_close_cb = on_process_stream_close;
        proc->refcount++;
    }

    proc->internal_exit_cb = on_process_exit;
    proc->internal_close_cb = decref;
    proc->refcount++;

    kl_push(watcher_ptr_kt, proc->loop->children, proc);

    return 0;
}

void process_teardown(main_loop_st *loop)
FUNC_ATTR_NONNULL_ALL
{
    process_is_tearing_down = true;

    kl_iter(watcher_ptr_kt, loop->children, current)
    {
        process_st *proc = (*current)->data;

        if(proc->detach || proc->type == kProcessTypePty)
        {
            // Close handles to process without killing it.
            CREATE_EVENT(loop->events, process_close_handles, 1, proc);
        }
        else
        {
            uv_kill(proc->pid, SIGTERM);
            proc->term_sent = true;
            process_stop(proc);
        }
    }

    // Wait until all children exit and all close events are processed.
    LOOP_PROCESS_EVENTS_UNTIL(loop,
                              loop->events,
                              -1,
                              kl_empty(loop->children)
                              && multiqueue_empty(loop->events));

    pty_process_teardown(loop);
}

// Wrappers around `stream_close` that protect against double-closing.
void process_close_streams(process_st *proc)
FUNC_ATTR_NONNULL_ALL
{
    process_close_in(proc);
    process_close_out(proc);
    process_close_err(proc);
}

void process_close_in(process_st *proc)
FUNC_ATTR_NONNULL_ALL
{
    CLOSE_PROC_STREAM(proc, in);
}

void process_close_out(process_st *proc)
FUNC_ATTR_NONNULL_ALL
{
    CLOSE_PROC_STREAM(proc, out);
}

void process_close_err(process_st *proc)
FUNC_ATTR_NONNULL_ALL
{
    CLOSE_PROC_STREAM(proc, err);
}

/// Synchronously wait for a process to finish
///
/// @param process  process_st instance
/// @param ms       Time in milliseconds to wait for the process.
///                 0 for no wait. -1 to wait until the process quits.
/// @return
/// Exit code of the process.
/// - -1 if the timeout expired while the process is still running.
/// - -2 if the user interruped the wait.
int process_wait(process_st *proc, int ms, multiqueue_st *events)
FUNC_ATTR_NONNULL_ARG(1)
{
    int status = -1;
    bool interrupted = false;

    if(!proc->refcount)
    {
        status = proc->status;
        LOOP_PROCESS_EVENTS(proc->loop, proc->events, 0);
        return status;
    }

    if(!events)
    {
        events = proc->events;
    }

    // Increase refcount to stop the exit callback from being called
    // (and possibly freed) before we have a chance to get the status.
    proc->refcount++;
    LOOP_PROCESS_EVENTS_UNTIL(proc->loop,
                              events,
                              ms,
                              // Until: interrupted by the user or job exited
                              got_int || proc->refcount == 1);

    // we'll assume that a user frantically hitting interrupt doesn't like
    // the current job. Signal that it has to be killed.
    if(got_int)
    {
        interrupted = true;
        got_int = false;
        process_stop(proc);

        if(ms == -1)
        {
            // We can only return if all streams/handles
            // are closed and the job exited.
            LOOP_PROCESS_EVENTS_UNTIL(proc->loop, events,
                                      -1, proc->refcount == 1);
        }
        else
        {
            LOOP_PROCESS_EVENTS(proc->loop, events, 0);
        }
    }

    if(proc->refcount == 1)
    {
        // Job exited, collect status and manually
        // invoke close_cb to free the job resources
        status = interrupted ? -2 : proc->status;
        decref(proc);

        if(events)
        {
            // the decref call created an exit event, process it now
            multiqueue_process_events(events);
        }
    }
    else
    {
        proc->refcount--;
    }

    return status;
}

/// Ask a process to terminate and eventually kill if it doesn't respond
void process_stop(process_st *proc)
FUNC_ATTR_NONNULL_ALL
{
    if(proc->stopped_time)
    {
        return;
    }

    proc->stopped_time = os_hrtime();

    switch(proc->type)
    {
        case kProcessTypeUv:
            // Close the process's stdin. If the process
            // doesn't close its own stdout/stderr, they
            // will be closed when it exits(possibly due
            // to being terminated after a timeout)
            process_close_in(proc);
            break;

        case kProcessTypePty:
            // close all streams for pty processes
            // to send SIGHUP to the process
            process_close_streams(proc);
            pty_process_close_master((pty_process_st *)proc);
            break;

        default:
            abort();
    }

   main_loop_st *loop = proc->loop;

    if(!loop->children_stop_requests++)
    {
        // When there's at least one stop request pending, start a timer that
        // will periodically check if a signal should be send to a to the job
        DEBUG_LOG("Starting job kill timer");
        uv_timer_start(&loop->children_kill_timer, children_kill_cb, 100, 100);
    }
}

/// Iterates the process list sending SIGTERM to stopped processes and SIGKILL
/// to those that didn't die from SIGTERM after a while(exit_timeout is 0).
static void children_kill_cb(uv_timer_t *handle)
{
  main_loop_st *loop = handle->loop->data;
    uint64_t now = os_hrtime();

    kl_iter(watcher_ptr_kt, loop->children, current)
    {
        process_st *proc = (*current)->data;

        if(!proc->stopped_time)
        {
            continue;
        }

        uint64_t elapsed = now - proc->stopped_time;

        if(!proc->term_sent && elapsed >= TERM_TIMEOUT)
        {
            STATE_LOG("Sending SIGTERM to pid %d", proc->pid);
            uv_kill(proc->pid, SIGTERM);
            proc->term_sent = true;
        }
        else if(elapsed >= KILL_TIMEOUT)
        {
            STATE_LOG("Sending SIGKILL to pid %d", proc->pid);
            uv_kill(proc->pid, SIGKILL);
        }
    }
}

static void process_close_event(void **argv)
{
    process_st *proc = argv[0];
    shell_free_argv(proc->argv);

    if(proc->type == kProcessTypePty)
    {
        xfree(((pty_process_st *)proc)->term_name);
    }

    if(proc->cb)
    {
        proc->cb(proc, proc->status, proc->data);
    }
}

static void decref(process_st *proc)
{
    if(--proc->refcount != 0)
    {
        return;
    }

   main_loop_st *loop = proc->loop;
    kliter_t(watcher_ptr_kt) **node = NULL;

    kl_iter(watcher_ptr_kt, loop->children, current)
    {
        if((*current)->data == proc)
        {
            node = current;
            break;
        }
    }

    assert(node);

    kl_shift_at(watcher_ptr_kt, loop->children, node);

    CREATE_EVENT(proc->events, process_close_event, 1, proc);
}

static void process_close(process_st *proc)
FUNC_ATTR_NONNULL_ARG(1)
{
    if(process_is_tearing_down
       && (proc->detach || proc->type == kProcessTypePty)
       && proc->closed)
    {
        // If a detached/pty process dies while
        // tearing down it might get closed twice.
        return;
    }

    assert(!proc->closed);
    proc->closed = true;

    switch(proc->type)
    {
        case kProcessTypeUv:
            libuv_process_close((libuv_process_st *)proc);
            break;

        case kProcessTypePty:
            pty_process_close((pty_process_st *)proc);
            break;

        default:
            abort();
    }
}

/// Flush output stream.
///
/// @param proc     process, for which an output stream should be flushed.
/// @param stream   stream to flush.
static void flush_stream(process_st *proc, stream_st *stream)
FUNC_ATTR_NONNULL_ARG(1)
{
    if(!stream || stream->closed)
    {
        return;
    }

    // Maximal remaining data size of terminated process is system buffer size.
    // Also helps with a child process that keeps the output streams open. If it
    // keeps sending data, we only accept as much data as the system buffer size.
    // Otherwise this would block cleanup/teardown.
    int system_buffer_size = 0;
    int err = uv_recv_buffer_size((uv_handle_t *)&stream->uv.pipe,
                                  &system_buffer_size);

    if(err)
    {
        system_buffer_size = (int)rbuffer_capacity(stream->buffer);
    }

    size_t max_bytes = stream->num_bytes + (size_t)system_buffer_size;

    // Read remaining data.
    while(!stream->closed && stream->num_bytes < max_bytes)
    {
        // Remember number of bytes before polling
        size_t num_bytes = stream->num_bytes;

        // Poll for data and process the generated events.
        loop_poll_events(proc->loop, 0);

        if(proc->events)
        {
            multiqueue_process_events(proc->events);
        }

        // Stream can be closed if it is empty.
        if(num_bytes == stream->num_bytes)
        {
            if(stream->read_cb)
            {
                // Stream callback could miss EOF handling
                // if a child keeps the stream open.
                stream->read_cb(stream, stream->buffer,
                                0, stream->cb_data, true);
            }

            break;
        }
    }
}

static void process_close_handles(void **argv)
{
    process_st *proc = argv[0];
    flush_stream(proc, proc->out);
    flush_stream(proc, proc->err);
    process_close_streams(proc);
    process_close(proc);
}

static void on_process_exit(process_st *proc)
{
  main_loop_st *loop = proc->loop;

    if(proc->stopped_time
       && loop->children_stop_requests
       && !--loop->children_stop_requests)
    {
        // Stop the timer if no more stop requests are pending
        DEBUG_LOG("Stopping process kill timer");
        uv_timer_stop(&loop->children_kill_timer);
    }

    // Process has terminated, but there could still be data to be read from
    // the OS. We are still in the libuv loop, so we cannot call code that
    // polls for more data directly. Instead delay the reading after the libuv
    // loop by queueing process_close_handles() as an event.
    multiqueue_st *queue = proc->events ? proc->events : loop->events;
    CREATE_EVENT(queue, process_close_handles, 1, proc);
}

static void on_process_stream_close(stream_st *FUNC_ARGS_UNUSED_MATCH(stream_ptr),
                                    void *data)
{
    process_st *proc = data;
    decref(proc);
}
