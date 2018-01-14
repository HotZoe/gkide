/// @file nvim/os/pty_process_unix.h

#ifndef NVIM_OS_PTY_PROCESS_UNIX_H
#define NVIM_OS_PTY_PROCESS_UNIX_H

#include <sys/ioctl.h>

#include "nvim/event/process.h"

typedef struct pty_process_s
{
    process_st process;
    char *term_name;
    uint16_t width;
    uint16_t height;
    struct winsize winsize;
    int tty_fd;
} pty_process_st;

static inline pty_process_st pty_process_init(main_loop_st *loop, void *data)
{
    pty_process_st rv;
    rv.process = process_init(loop, kProcessTypePty, data);
    rv.term_name = NULL;
    rv.width = 80;
    rv.height = 24;
    rv.tty_fd = -1;
    return rv;
}

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "os/pty_process_unix.h.generated.h"
#endif

#endif // NVIM_OS_PTY_PROCESS_UNIX_H
