/// @file nvim/syntax_defs.h

#ifndef NVIM_SYNTAX_DEFS_H
#define NVIM_SYNTAX_DEFS_H

#include "nvim/regexp_defs.h"

#define SST_MIN_ENTRIES  150    ///< minimal size for state stack array
#define SST_MAX_ENTRIES  1000   ///< maximal size for state stack array
#define SST_FIX_STATES   7      ///< size of sst_stack[].
#define SST_DIST         16     ///< normal distance between entries

/// invalid syn_state pointer
#define SST_INVALID     (synstate_st *)-1

typedef int32_t rgb_color_kt;

/// display tick type
typedef unsigned short disptick_kt;

/// struct passed to in_id_list()
struct syn_args_s
{
    int inc_tag;         ///< ":syn include" unique tag
    short id;            ///< highlight group ID of item
    short *cont_in_list; ///< cont.in group IDs, if non-zero
};
typedef struct syn_args_s syn_args_st;

/// Each keyword has one keyentry, which is linked in a hash list.
typedef struct keyentry_s keyentry_st;
struct keyentry_s
{
    keyentry_st *ke_next; ///< next entry with identical "keyword[]"
    syn_args_st k_syn;    ///< struct passed to in_id_list()
    short *next_list;     ///< ID list for next match (if non-zero)
    int flags;
    int k_char;           ///< conceal substitute character
    uchar_kt keyword[1];  ///< actually longer
};

/// Struct used to store one state of the state stack.
typedef struct bufstate_s
{
    int bs_idx;    ///< index of pattern
    int bs_flags;  ///< flags for pattern
    int bs_seqnr;  ///< stores si_seqnr
    int bs_cchar;  ///< stores si_cchar

    /// external matches from start pattern
    reg_extmatch_st *bs_extmatch;
} bufstate_st;

/// syn_state contains the syntax state stack for
/// the start of one line. Used by b_sst_array[].
typedef struct synstate_s synstate_st;
struct synstate_s
{
    /// next entry in used or free list
    synstate_st *sst_next;
    /// line number for this state
    linenum_kt sst_lnum;
    union
    {
        /// short state stack
        bufstate_st sst_stack[SST_FIX_STATES];
        /// growarray for long state stack
        garray_st sst_ga;
    } sst_union;
    /// flags for sst_next_list
    int sst_next_flags;
    /// number of states on the stack
    int sst_stacksize;
    /// "nextgroup" list in this state (this is a copy, don't free it!)
    short *sst_next_list;
    /// tick when last displayed
    disptick_kt sst_tick;
    /// when non-zero, change in this line may have made the state invalid
    linenum_kt sst_change_lnum;
};

/// Structure shared between syntax.c, screen.c
typedef struct attrinfo_s
{
    // HL_BOLD, etc.
    int16_t rgb_ae_attr;
    int16_t cterm_ae_attr;

    rgb_color_kt rgb_fg_color;
    rgb_color_kt rgb_bg_color;
    rgb_color_kt rgb_sp_color;

    int cterm_fg_color;
    int cterm_bg_color;
} attrinfo_st;

#endif // NVIM_SYNTAX_DEFS_H
