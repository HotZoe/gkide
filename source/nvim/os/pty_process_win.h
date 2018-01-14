/// @file nvim/os/pty_process_win.h

#ifndef NVIM_OS_PTY_PROCESS_WIN_H
#define NVIM_OS_PTY_PROCESS_WIN_H

#include "nvim/event/libuv_process.h"

typedef struct pty_process_s
{
    process_st process;
    char *term_name;
    uint16_t width;
    uint16_t height;
} pty_process_st;

#define pty_process_spawn(job)        libuv_process_spawn((libuv_process_st *)job)
#define pty_process_close(job)        libuv_process_close((libuv_process_st *)job)
#define pty_process_close_master(job) libuv_process_close((libuv_process_st *)job)

// windows do nothing
#define pty_process_teardown(loop)             (void)loop;
#define pty_process_resize(job, width, height) (void)job; (void)width; (void)height;

static inline pty_process_st pty_process_init(main_loop_st *loop, void *data)
{
    pty_process_st rv;

    rv.process = process_init(loop, kProcessTypePty, data);
    rv.term_name = NULL;
    rv.width = 80;
    rv.height = 24;

    return rv;
}

#endif // NVIM_OS_PTY_PROCESS_WIN_H
