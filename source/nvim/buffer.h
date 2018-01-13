/// @file nvim/buffer.h

#ifndef NVIM_BUFFER_H
#define NVIM_BUFFER_H

#include "nvim/vim.h"
#include "nvim/window.h"
#include "nvim/pos.h" // for linenum_kt
#include "nvim/ex_cmds_defs.h" // for exargs_st
#include "nvim/screen.h" // for StlClickRecord
#include "nvim/func_attr.h"
#include "nvim/eval.h"
#include "nvim/macros.h"

/// Values for buflist_getfile()
enum getf_values
{
    GETF_SETMARK = 0x01, ///< set pcmark before jumping
    GETF_ALT     = 0x02, ///< jumping to alternate file (not buf num)
    GETF_SWITCH  = 0x04, ///< respect 'switchbuf' settings when jumping
};

/// Values for buflist_new() flags
enum bln_values
{
    BLN_CURBUF = 1,   ///< May re-use curbuf for new buffer
    BLN_LISTED = 2,   ///< Put new buffer in buffer list
    BLN_DUMMY  = 4,   ///< Allocating dummy buffer
    BLN_NOOPT  = 16,  ///< Don't copy options to existing buffer
    /// @todo(mhinz): merge patch that introduces BLN_NEW
};

/// Values for action argument for do_buffer()
enum dobuf_action_values
{
    DOBUF_GOTO   = 0, ///< go to specified buffer
    DOBUF_SPLIT  = 1, ///< split window and go to specified buffer
    DOBUF_UNLOAD = 2, ///< unload specified buffer(s)
    DOBUF_DEL    = 3, ///< delete specified buffer(s) from buflist
    DOBUF_WIPE   = 4, ///< delete specified buffer(s) really
};

/// Values for start argument for do_buffer()
enum dobuf_start_values
{
    DOBUF_CURRENT = 0, ///< "count" buffer from current buffer
    DOBUF_FIRST   = 1, ///< "count" buffer from first buffer
    DOBUF_LAST    = 2, ///< "count" buffer from last buffer
    DOBUF_MOD     = 3, ///< "count" mod. buffer from current buffer
};

/// flags for buf_freeall()
enum bfa_values
{
    BFA_DEL       = 1, ///< buffer is going to be deleted
    BFA_WIPE      = 2, ///< buffer is going to be wiped out
    BFA_KEEP_UNDO = 4, ///< do not free undo information
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "buffer.h.generated.h"
#endif

// Find a window that contains `buf` and switch to it.
// If there is no such window, use the current window and change `curbuf`
// Caller must initialize save_curbuf to NULL.
// restore_win_for_buf() MUST be called later!
static inline void switch_to_win_for_buf(filebuf_st *buf,
                                         win_st **save_curwinp,
                                         tabpage_st **save_curtabp,
                                         bufref_st *save_curbuf)
{
    win_st *wp;
    tabpage_st *tp;

    if(!find_win_for_buf(buf, &wp, &tp)
       || switch_win(save_curwinp, save_curtabp, wp, tp, true) == FAIL)
    {
        switch_buffer(save_curbuf, buf);
    }
}

static inline void restore_win_for_buf(win_st *save_curwin,
                                       tabpage_st *save_curtab,
                                       bufref_st *save_curbuf)
{
    if(save_curbuf->br_buf == NULL)
    {
        restore_win(save_curwin, save_curtab, true);
    }
    else
    {
        restore_buffer(save_curbuf);
    }
}

static inline void buf_set_changedtick(filebuf_st *const buf, const int changedtick)
REAL_FATTR_NONNULL_ALL REAL_FATTR_ALWAYS_INLINE;

/// Set b_changedtick and corresponding variable
///
/// @param[out]  buf          Buffer to set changedtick in.
/// @param[in]   changedtick  New value.
static inline void buf_set_changedtick(filebuf_st *const buf, const int changedtick)
{
#ifndef NDEBUG
    dictitem_st *const changedtick_di = tv_dict_find(buf->b_vars, S_LEN("changedtick"));
    assert(changedtick_di != NULL);
    assert(changedtick_di->di_tv.v_type == kNvarNumber);
    assert(changedtick_di->di_tv.v_lock == kNvlVarFixed);

    // For some reason formatc does not like the below.
    #ifndef UNIT_TESTING_LUA_PREPROCESSING
    assert(changedtick_di->di_flags == (DI_FLAGS_RO|DI_FLAGS_FIX));
    #endif

    assert(changedtick_di == (dictitem_st *)&buf->changedtick_di);
    assert(&buf->b_changedtick == &buf->changedtick_di.di_tv.vval.v_number);
#endif

    buf->b_changedtick = changedtick;
}

#define WITH_BUFFER(b, code)                                                \
    do                                                                      \
    {                                                                       \
        win_st *save_curwin = NULL;                                          \
        tabpage_st *save_curtab = NULL;                                      \
        bufref_st save_curbuf = { NULL, 0 };                                 \
        switch_to_win_for_buf(b, &save_curwin, &save_curtab, &save_curbuf); \
        code;                                                               \
        restore_win_for_buf(save_curwin, save_curtab, &save_curbuf);        \
    } while(0)

#endif // NVIM_BUFFER_H
