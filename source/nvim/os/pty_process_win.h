/// @file nvim/os/pty_process_win.h

#ifndef NVIM_OS_PTY_PROCESS_WIN_H
#define NVIM_OS_PTY_PROCESS_WIN_H

#include "nvim/event/libuv_process.h"

typedef struct pty_process
{
    process_st process;
    char *term_name;
    uint16_t width;
    uint16_t height;
} PtyProcess;

#define pty_process_spawn(job)        libuv_process_spawn((libuv_process_st *)job)
#define pty_process_close(job)        libuv_process_close((libuv_process_st *)job)
#define pty_process_close_master(job) libuv_process_close((libuv_process_st *)job)

// windows do nothing
#define pty_process_teardown(loop)             (void)loop;
#define pty_process_resize(job, width, height) (void)job; (void)width; (void)height;

static inline PtyProcess pty_process_init(main_loop_st *loop, void *data)
{
    PtyProcess rv;

    rv.process = process_init(loop, kProcessTypePty, data);
    rv.term_name = NULL;
    rv.width = 80;
    rv.height = 24;

    return rv;
}

#endif // NVIM_OS_PTY_PROCESS_WIN_H
