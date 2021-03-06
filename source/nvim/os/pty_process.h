/// @file nvim/os/pty_process.h

#ifndef NVIM_OS_PTY_PROCESS_H
#define NVIM_OS_PTY_PROCESS_H

#include "generated/config/config.h"

#ifdef HOST_OS_WINDOWS
    #include "nvim/os/pty_process_win.h"
#else
    #include "nvim/os/pty_process_unix.h"
#endif

#endif // NVIM_OS_PTY_PROCESS_H
