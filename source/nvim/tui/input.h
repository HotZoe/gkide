/// @file nvim/input.h

#ifndef NVIM_TUI_INPUT_H
#define NVIM_TUI_INPUT_H

#include <stdbool.h>

#include <termkey.h>
#include "nvim/event/stream.h"
#include "nvim/event/time.h"

#include "generated/config/config.h"

typedef struct terminal_input_s
{
    int in_fd;
    bool paste_enabled;
    bool waiting;
    TermKey *tk;

#if TERMKEY_VERSION_MAJOR > 0 || TERMKEY_VERSION_MINOR > 18
    /// libtermkey terminfo hook
    TermKey_Terminfo_Getstr_Hook *tk_ti_hook_fn;
#endif

    time_watcher_st timer_handle;
    main_loop_st *loop;

#ifdef HOST_OS_WINDOWS
    uv_tty_t tty_in;
#endif

    stream_st read_stream;
    ringbuf_st *key_buffer;
    uv_mutex_t key_buffer_mutex;
    uv_cond_t key_buffer_cond;
} terminal_input_st;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "tui/input.h.generated.h"
#endif

#endif  // NVIM_TUI_INPUT_H
