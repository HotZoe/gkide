/// @file nvim/event/socket.h

#ifndef NVIM_EVENT_SOCKET_H
#define NVIM_EVENT_SOCKET_H

#include <uv.h>

#include "nvim/event/loop.h"
#include "nvim/event/rstream.h"
#include "nvim/event/wstream.h"

#define ADDRESS_MAX_SIZE 256

typedef struct socket_watcher_s socket_watcher_st;
typedef void (*socket_ft)(socket_watcher_st *watcher, int result, void *data);
typedef void (*socket_close_cb)(socket_watcher_st *watcher, void *data);

struct socket_watcher_s
{
    /// Pipe/socket path, or TCP address string
    char addr[ADDRESS_MAX_SIZE];

    union
    {
        struct
        {
            uv_tcp_t handle;
            struct addrinfo *addrinfo;
        } tcp;
        struct
        {
            uv_pipe_t handle;
        } pipe;
    } uv; ///< TCP server or unix socket (named pipe on Windows)

    uv_stream_t *stream;
    void *data;
    socket_ft cb;
    socket_close_cb close_cb;
    multiqueue_st *events;
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "event/socket.h.generated.h"
#endif

#endif // NVIM_EVENT_SOCKET_H
