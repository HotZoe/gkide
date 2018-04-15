/// @file nvim/state.c

#include <assert.h>

#include "nvim/lib/kvec.h"

#include "nvim/ascii.h"
#include "nvim/state.h"
#include "nvim/nvim.h"
#include "nvim/main.h"
#include "nvim/getchar.h"
#include "nvim/option_defs.h"
#include "nvim/ui.h"
#include "nvim/os/input.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "state.c.generated.h"
#endif

void state_enter(nvim_state_st *s)
{
    while(1)
    {
        int check_result = s->check ? s->check(s) : 1;

        if(kNSCC_ExitNvim == check_result)
        {
            break;
        }
        else if(kNSCC_LoopNext == check_result)
        {
            continue;
        }

        int key;
getkey:

        if(char_avail() || using_script() || input_available())
        {
            // Don't block for events if there's a character already
            // available for processing. Characters can come from mappings,
            // scripts and other sources, so this scenario is very common.
            key = safe_vgetc();
        }
        else if(!multiqueue_empty(main_loop.events))
        {
            // Event was made available after the last
            // multiqueue_process_events call
            key = K_EVENT;
        }
        else
        {
            input_enable_events();
            ui_flush(); // Flush screen updates before blocking

            // Call `os_inchar` directly to block for events or
            // user input without consuming anything from
            // `input_buffer`(os/input.c) or calling the mapping
            // engine. If an event was put into the queue, we send
            // K_EVENT directly.
            (void)os_inchar(NULL, 0, -1, 0);
            input_disable_events();
            key = !multiqueue_empty(main_loop.events) ? K_EVENT : safe_vgetc();
        }

        if(key == K_EVENT)
        {
            may_sync_undo();
        }

        int execute_result = s->execute(s, key);

        if(!execute_result)
        {
            break;
        }
        else if(execute_result == -1)
        {
            goto getkey;
        }
    }
}

/// Return TRUE if in the current mode we need to use virtual.
int virtual_active(void)
{
    // While an operator is being executed we return "virtual_op", because
    // VIsual_active has already been reset, thus we can't check for "block"
    // being used.
    if(virtual_op != MAYBE)
    {
        return virtual_op;
    }

    return ve_flags == VE_ALL
           || ((ve_flags & VE_BLOCK)
               && VIsual_active
               && VIsual_mode == Ctrl_V)
           || ((ve_flags & VE_INSERT)
               && (curmod & kInsertMode));
}

/// @b kVisualMode, @b kMapSelectMode and @b kOpPendMode, @b curmod are never
/// set, they are equal to @b curmod (@b kNormalMode) with a condition.
///
/// @return
/// This function returns the real @b curmod.
int get_real_state(void)
{
    if(curmod & kNormalMode)
    {
        if(VIsual_active)
        {
            if(VIsual_select)
            {
                return kMapSelectMode;
            }

            return kVisualMode;
        }
        else if(finish_op)
        {
            return kOpPendMode;
        }
    }

    return curmod;
}

/// @returns[allocated] mode string
char *get_mode(void)
{
    char *buf = xcalloc(3, sizeof(char));

    if(VIsual_active)
    {
        if(VIsual_select)
        {
            buf[0] = (char)(VIsual_mode + 's' - 'v');
        }
        else
        {
            buf[0] = (char)VIsual_mode;
        }
    }
    else if(curmod == kNormalWaitMode
            || curmod == kAskMoreMode
            || curmod == kSetWinSizeMode
            || curmod == kConfirmMode)
    {
        buf[0] = 'r';

        if(curmod == kAskMoreMode)
        {
            buf[1] = 'm';
        }
        else if(curmod == kConfirmMode)
        {
            buf[1] = '?';
        }
    }
    else if(curmod == kExecExtCmdMode)
    {
        buf[0] = '!';
    }
    else if(curmod & kInsertMode)
    {
        if(curmod & kModFlgVReplace)
        {
            buf[0] = 'R';
            buf[1] = 'v';
        }
        else if(curmod & kModFlgReplace)
        {
            buf[0] = 'R';
        }
        else
        {
            buf[0] = 'i';
        }
    }
    else if(curmod & kCmdLineMode)
    {
        buf[0] = 'c';

        if(exmode_active)
        {
            buf[1] = 'v';
        }
    }
    else if(exmode_active)
    {
        buf[0] = 'c';
        buf[1] = 'e';
    }
    else if(curmod & kTermFocusMode)
    {
        buf[0] = 't';
    }
    else
    {
        buf[0] = 'n';

        if(finish_op)
        {
            buf[1] = 'o';
        }
    }

    return buf;
}
