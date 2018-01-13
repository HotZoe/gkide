/// @file nvim/buffer_defs.h

#ifndef NVIM_BUFFER_DEFS_H
#define NVIM_BUFFER_DEFS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> // for FILE

typedef struct filebuf_s filebuf_st;

/// Reference to a buffer that stores the value of buf_free_count.
/// bufref_valid() only needs to check "buf" when the count differs.
typedef struct
{
    filebuf_st *br_buf;
    int br_buf_free_count;
} bufref_st;

#include "nvim/garray.h"
#include "nvim/pos.h"
#include "nvim/option_defs.h"
#include "nvim/mark_defs.h"
#include "nvim/undo_defs.h"
#include "nvim/hashtab.h"
#include "nvim/eval/typval.h"
#include "nvim/profile.h"
#include "nvim/api/private/defs.h"
#include "nvim/map.h"

#define MODIFIABLE(buf) (buf->b_p_ma)

// Flags for w_valid.
// These are set when something in a window structure becomes invalid, except
// when the cursor is moved.  Call check_cursor_moved() before testing one of
// the flags.
// These are reset when that thing has been updated and is valid again.
//
// Every function that invalidates one of these must call one of the
// invalidate_* functions.
//
// w_valid is supposed to be used only in screen.c.  From other files, use the
// functions that set or reset the flags.
//
// VALID_BOTLINE    VALID_BOTLINE_AP
//  on      on      w_botline valid
//  off     on      w_botline approximated
//  off     off     w_botline not valid
//  on      off     not possible

#define VALID_WROW       0x01  ///< w_wrow (window row) is valid
#define VALID_WCOL       0x02  ///< w_wcol (window col) is valid
#define VALID_VIRTCOL    0x04  ///< w_virtcol (file col) is valid
#define VALID_CHEIGHT    0x08  ///< w_cline_height and w_cline_folded valid
#define VALID_CROW       0x10  ///< w_cline_row is valid
#define VALID_BOTLINE    0x20  ///< w_botine and w_empty_rows are valid
#define VALID_BOTLINE_AP 0x40  ///< w_botine is approximated
#define VALID_TOPLINE    0x80  ///< w_topline is valid (for cursor position)

// flags for b_flags
#define BF_RECOVERED    0x01  ///< buffer has been recovered
#define BF_CHECK_RO     0x02  ///< need to check readonly when loading file
                              ///< into buffer (set by ":e", may be reset by ":buf")
#define BF_NEVERLOADED  0x04  ///< file has never been loaded into buffer,
                              ///< many variables still need to be set
#define BF_NOTEDITED    0x08  ///< Set when file name is changed after
                              ///< starting to edit, reset when file is written out.
#define BF_NEW          0x10  ///< file didn't exist when editing started
#define BF_NEW_W        0x20  ///< Warned for BF_NEW and file created
#define BF_READERR      0x40  ///< got errors while reading the file
#define BF_DUMMY        0x80  ///< dummy buffer, only used internally
#define BF_PRESERVED    0x100 ///< ":preserve" was used

// Mask to check for flags that prevent normal writing
#define BF_WRITE_MASK   (BF_NOTEDITED + BF_NEW + BF_READERR)

typedef struct window_s  win_st;
typedef struct frame_s   frame_st;
typedef struct wininfo_s wininfo_st;

#include "nvim/memline_defs.h"
#include "nvim/memfile_defs.h"
#include "nvim/regexp_defs.h"
#include "nvim/syntax_defs.h"
#include "nvim/sign_defs.h"
#include "nvim/bufhl_defs.h"

typedef Map(linenum_kt, bufhl_vec_st) bufhl_info_T;

#include "nvim/os/fs_defs.h"
#include "nvim/terminal.h"

/// The taggy struct is used to store the information about a :tag command.
typedef struct taggy_s
{
    uchar_kt *tagname;  ///< tag name
    filemark_st fmark;  ///< cursor position BEFORE ":tag"
    int cur_match;      ///< match number
    int cur_fnum;       ///< buffer number used for cur_match
} taggy_st;

typedef struct buffblock_s  buffblock_st;
typedef struct buffheader_s buffheader_st;

/// structure used to store one block of the stuff/redo/recording buffers
struct buffblock_s
{
    buffblock_st *b_next;  ///< pointer to next buffblock
    uchar_kt b_str[1];      ///< contents (actually longer)
};

/// header used for the stuff buffer and the redo buffer
struct buffheader_s
{
    buffblock_st bh_first;  ///< first (dummy) block of list
    buffblock_st *bh_curr;  ///< buffblock for appending
    size_t bh_index;       ///< index for reading
    size_t bh_space;       ///< space in bh_curr for appending
};

/// Structure that contains all options that are local to a window.
/// Used twice in a window: for the current buffer and for all buffers.
/// Also used in wininfo_st.
typedef struct
{
    int wo_arab;                ///< 'arabic'
#define w_p_arab                w_onebuf_opt.wo_arab

    int wo_bri;                 ///< 'breakindent'
#define w_p_bri                 w_onebuf_opt.wo_bri

    uchar_kt *wo_briopt;          ///< 'breakindentopt'
#define w_p_briopt              w_onebuf_opt.wo_briopt

    int wo_diff;                ///< 'diff'
#define w_p_diff                w_onebuf_opt.wo_diff

    long wo_fdc;                ///< 'foldcolumn'
#define w_p_fdc                 w_onebuf_opt.wo_fdc

    int wo_fdc_save;            ///< 'foldenable' saved for diff mode
#define w_p_fdc_save            w_onebuf_opt.wo_fdc_save

    int wo_fen;                 ///< 'foldenable'
#define w_p_fen                 w_onebuf_opt.wo_fen

    int wo_fen_save;            ///< 'foldenable' saved for diff mode
#define w_p_fen_save            w_onebuf_opt.wo_fen_save

    uchar_kt *wo_fdi;             ///< 'foldignore'
#define w_p_fdi                 w_onebuf_opt.wo_fdi

    long wo_fdl;                ///< 'foldlevel'
#define w_p_fdl                 w_onebuf_opt.wo_fdl

    int wo_fdl_save;            ///< 'foldlevel' state saved for diff mode
#define w_p_fdl_save            w_onebuf_opt.wo_fdl_save

    uchar_kt *wo_fdm;             ///< 'foldmethod'
#define w_p_fdm                 w_onebuf_opt.wo_fdm

    uchar_kt *wo_fdm_save;        ///< 'fdm' saved for diff mode
#define w_p_fdm_save            w_onebuf_opt.wo_fdm_save

    long wo_fml;                ///< 'foldminlines'
#define w_p_fml                 w_onebuf_opt.wo_fml

    long wo_fdn;                ///< 'foldnestmax'
#define w_p_fdn                 w_onebuf_opt.wo_fdn

    uchar_kt *wo_fde;             ///< 'foldexpr'
#define w_p_fde                 w_onebuf_opt.wo_fde

    uchar_kt *wo_fdt;             ///< 'foldtext'
#define w_p_fdt                 w_onebuf_opt.wo_fdt

    uchar_kt *wo_fmr;             ///< 'foldmarker'
#define w_p_fmr                 w_onebuf_opt.wo_fmr

    int wo_lbr;                 ///< 'linebreak'
#define w_p_lbr                 w_onebuf_opt.wo_lbr

    int wo_list;                ///< 'list'
#define w_p_list                w_onebuf_opt.wo_list

    int wo_nu;                  ///< 'number'
#define w_p_nu                  w_onebuf_opt.wo_nu

    int wo_rnu;                 ///< 'relativenumber'
#define w_p_rnu                 w_onebuf_opt.wo_rnu

    long wo_nuw;                ///< 'numberwidth'
#define w_p_nuw                 w_onebuf_opt.wo_nuw

    int wo_wfh;                 ///< 'winfixheight'
#define w_p_wfh                 w_onebuf_opt.wo_wfh

    int wo_wfw;                 ///< 'winfixwidth'
#define w_p_wfw                 w_onebuf_opt.wo_wfw

    int wo_pvw;                 ///< 'previewwindow'
#define w_p_pvw                 w_onebuf_opt.wo_pvw

    int wo_rl;                  ///< 'rightleft'
#define w_p_rl                  w_onebuf_opt.wo_rl

    uchar_kt *wo_rlc;             ///< 'rightleftcmd'
#define w_p_rlc                 w_onebuf_opt.wo_rlc

    long wo_scr;                ///< 'scroll'
#define w_p_scr                 w_onebuf_opt.wo_scr

    int wo_spell;               ///< 'spell'
#define w_p_spell               w_onebuf_opt.wo_spell

    int wo_cuc;                 ///< 'cursorcolumn'
#define w_p_cuc                 w_onebuf_opt.wo_cuc

    int wo_cul;                 ///< 'cursorline'
#define w_p_cul                 w_onebuf_opt.wo_cul

    uchar_kt *wo_cc;              ///< 'colorcolumn'
#define w_p_cc                  w_onebuf_opt.wo_cc

    uchar_kt *wo_stl;             ///< 'statusline'
#define w_p_stl                 w_onebuf_opt.wo_stl

    int wo_scb;                 ///< 'scrollbind'
#define w_p_scb                 w_onebuf_opt.wo_scb

    int wo_diff_saved;          ///< options were saved for starting diff mode
#define w_p_diff_saved          w_onebuf_opt.wo_diff_saved

    int wo_scb_save;            ///< 'scrollbind' saved for diff mode
#define w_p_scb_save            w_onebuf_opt.wo_scb_save

    int wo_wrap;                ///< 'wrap'
#define w_p_wrap                w_onebuf_opt.wo_wrap

    int wo_wrap_save;           ///< 'wrap' state saved for diff mode
#define w_p_wrap_save           w_onebuf_opt.wo_wrap_save

    uchar_kt *wo_cocu;            ///< 'concealcursor'
#define w_p_cocu                w_onebuf_opt.wo_cocu

    long wo_cole;               ///< 'conceallevel'
#define w_p_cole                w_onebuf_opt.wo_cole

    int wo_crb;                 ///< 'cursorbind'
#define w_p_crb                 w_onebuf_opt.wo_crb

    int wo_crb_save;            ///< 'cursorbind' state saved for diff mode
#define w_p_crb_save            w_onebuf_opt.wo_crb_save

    uchar_kt *wo_scl;             ///< 'signcolumn'
#define w_p_scl                 w_onebuf_opt.wo_scl

    uchar_kt *wo_winhl;           ///< 'winhighlight'
#define w_p_winhl               w_onebuf_opt.wo_winhl

    int wo_scriptID[WV_COUNT];  ///< SIDs for window-local options
#define w_p_scriptID            w_onebuf_opt.wo_scriptID
} winopt_T;

/// Window info stored with a buffer.
///
/// Two types of info are kept for a buffer which are associated with a
/// specific window:
/// 1. Each window can have a different line number associated with a buffer.
/// 2. The window-local options for a buffer work in a similar way.
///    The window-info is kept in a list at b_wininfo. It is kept in
///    most-recently-used order.
struct wininfo_s
{
    wininfo_st *wi_next;  ///< next entry or NULL for last entry
    wininfo_st *wi_prev;  ///< previous entry or NULL for first entry
    win_st *wi_win;       ///< pointer to window that did set wi_fpos
    apos_st wi_fpos;       ///< last cursor position in the file
    bool wi_optset;      ///< true when wi_opt has useful values
    winopt_T wi_opt;     ///< local window options
    bool wi_fold_manual; ///< copy of w_fold_manual
    garray_st wi_folds;   ///< clone of w_folds
};

/// Argument list: Array of file names.
/// Used for the global argument list and the argument lists local to a window.
///
/// @todo move arglist_s to another header
typedef struct arglist_s
{
    garray_st al_ga;  ///< growarray with the array of file names
    int al_refcount;  ///< number of windows using this arglist_st
    int id;           ///< id of this arglist_st
} arglist_st;

/// For each argument remember the file name as it was given, and the buffer
/// number that contains the expanded file name (required for when ":cd" is
/// used.
///
/// @todo: move aentry_st to another header
typedef struct argentry_s
{
    uchar_kt *ae_fname; ///< file name as specified
    int ae_fnum;        ///< buffer number with expanded file name
} aentry_st;

#define ALIST(win)      (win)->w_alist
#define GARGLIST        ((aentry_st *)global_alist.al_ga.ga_data)
#define ARGLIST         ((aentry_st *)ALIST(curwin)->al_ga.ga_data)
#define WARGLIST(wp)    ((aentry_st *)ALIST(wp)->al_ga.ga_data)
#define AARGLIST(al)    ((aentry_st *)((al)->al_ga.ga_data))
#define GARGCOUNT       (global_alist.al_ga.ga_len)
#define ARGCOUNT        (ALIST(curwin)->al_ga.ga_len)
#define WARGCOUNT(wp)   (ALIST(wp)->al_ga.ga_len)

/// Used for the typeahead buffer: typebuf.
typedef struct
{
    uchar_kt *tb_buf;     ///< buffer for typed characters
    uchar_kt *tb_noremap; ///< mapping flags for characters in tb_buf[]
    int tb_buflen;        ///< size of tb_buf[]
    int tb_off;           ///< current position in tb_buf[]
    int tb_len;           ///< number of valid bytes in tb_buf[]
    int tb_maplen;        ///< nr of mapped bytes in tb_buf[]
    int tb_silent;        ///< nr of silently mapped bytes in tb_buf[]
    int tb_no_abbr_cnt;   ///< nr of bytes without abbrev. in tb_buf[]
    int tb_change_cnt;    ///< nr of time tb_buf was changed; never zero
} typebuf_T;

/// Struct to hold the saved typeahead for save_typeahead().
typedef struct
{
    typebuf_T save_typebuf;
    int typebuf_valid; ///< TRUE when save_typebuf valid
    int old_char;
    int old_mod_mask;
    buffheader_st save_readbuf1;
    buffheader_st save_readbuf2;
    String save_inputbuf;
} tasave_T;

/// Structure used for mappings and abbreviations.
typedef struct mapblock mapblock_T;
struct mapblock
{
    mapblock_T *m_next;  ///< next mapblock in list
    uchar_kt *m_keys;      ///< mapped from, lhs
    uchar_kt *m_str;       ///< mapped to, rhs
    uchar_kt *m_orig_str;  ///< rhs as entered by the user
    int m_keylen;        ///< strlen(m_keys)
    int m_mode;          ///< valid mode
    int m_noremap;       ///< if non-zero no re-mapping for m_str
    char m_silent;       ///< <silent> used, don't echo commands
    char m_nowait;       ///< <nowait> used
    char m_expr;         ///< <expr> used, m_str is an expression
    script_id_kt m_script_ID;  ///< ID of script where map was defined
};

/// Used for highlighting in the status line.
struct stl_hlrec
{
    uchar_kt *start;
    int userhl;     ///< 0: no HL, 1-9: User HL, < 0 for syn ID
};

// values for b_syn_spell: what to do with toplevel text
#define SYNSPL_DEFAULT  0   ///< spell check if @Spell not defined
#define SYNSPL_TOP      1   ///< spell check toplevel text
#define SYNSPL_NOTOP    2   ///< don't spell check toplevel text

/// avoid #ifdefs for when b_spell is not available
#define B_SPELL(buf)   ((buf)->b_spell)

typedef struct qf_info_S qf_info_T;

/// Used for :syntime: timing of executing a syntax pattern.
typedef struct
{
    proftime_kt total;   ///< total time used
    proftime_kt slowest; ///< time of slowest call
    long count;         ///< nr of times used
    long match;         ///< nr of times matched
} syn_time_T;

/// These are items normally related to a buffer.
/// But when using ":ownsyntax" a window may have its own instance.
typedef struct
{
    hashtable_st b_keywtab;             ///< syntax keywords hash table
    hashtable_st b_keywtab_ic;          ///< idem, ignore case
    int b_syn_error;                 ///< TRUE when error occurred in HL
    int b_syn_ic;                    ///< ignore case for :syn cmds
    int b_syn_spell;                 ///< SYNSPL_ values
    garray_st b_syn_patterns;         ///< table for syntax patterns
    garray_st b_syn_clusters;         ///< table for syntax clusters
    int b_spell_cluster_id;          ///< @Spell cluster ID or 0
    int b_nospell_cluster_id;        ///< @NoSpell cluster ID or 0
    int b_syn_containedin;           ///< TRUE when there is an item with a
                                     ///< "containedin" argument
    int b_syn_sync_flags;            ///< flags about how to sync
    short b_syn_sync_id;             ///< group to sync on
    long b_syn_sync_minlines;        ///< minimal sync lines offset
    long b_syn_sync_maxlines;        ///< maximal sync lines offset
    long b_syn_sync_linebreaks;      ///< offset for multi-line pattern
    uchar_kt *b_syn_linecont_pat;      ///< line continuation pattern
    regprog_st *b_syn_linecont_prog;  ///< line continuation program
    syn_time_T b_syn_linecont_time;  ///<
    int b_syn_linecont_ic;           ///< ignore-case flag for above
    int b_syn_topgrp;                ///< for ":syntax include"
    int b_syn_conceal;               ///< auto-conceal for :syn cmds
    int b_syn_folditems;             ///< number of patterns with the HL_FOLD flag set

    // b_sst_array[] contains the state stack for a number of lines, for the
    // start of that line (col == 0). This avoids having to recompute the
    // syntax state too often.
    // b_sst_array[] is allocated to hold the state for all displayed lines,
    // and states for 1 out of about 20 other lines.
    synstate_st *b_sst_array;     ///< pointer to an array of synstate_st
    int b_sst_len;               ///< number of entries in b_sst_array[]
    synstate_st *b_sst_first;     ///< pointer to first used entry in b_sst_array[] or NULL
    synstate_st *b_sst_firstfree; ///< pointer to first free entry in b_sst_array[] or NULL
    int b_sst_freecount;         ///< number of free entries in b_sst_array[]
    linenum_kt b_sst_check_lnum;   ///< entries after this lnum need to be checked for
                                 ///< validity (MAXLNUM means no check needed)
    uint16_t b_sst_lasttick;     ///< last display tick

    // for spell checking
    garray_st b_langp;         ///< list of pointers to slang_T, see spell.c
    bool b_spell_ismw[256];   ///< flags: is midword char
    uchar_kt *b_spell_ismw_mb;  ///< multi-byte midword chars
    uchar_kt *b_p_spc;          ///< 'spellcapcheck'
    regprog_st *b_cap_prog;    ///< program for 'spellcapcheck'
    uchar_kt *b_p_spf;          ///< 'spellfile'
    uchar_kt *b_p_spl;          ///< 'spelllang'
    int b_cjk;                ///< all CJK letters as OK
    uchar_kt b_syn_chartab[32]; ///< syntax iskeyword option
    uchar_kt *b_syn_isk;        ///< iskeyword option
} synblock_T;

/// Type used for changedtick_di member in filebuf_st
///
/// Primary exists so that literals of relevant type can be made.
typedef TV_DICTITEM_STRUCT(sizeof("changedtick")) ChangedtickDictItem;

#define BUF_HAS_QF_ENTRY   1
#define BUF_HAS_LL_ENTRY   2

/// Maximum number of maphash blocks we will have
#define MAX_MAPHASH        256

/// buffer: structure that holds information about one file
///
/// Several windows can share a single Buffer
/// A buffer is unallocated if there is no memfile for it.
/// A buffer is new if the associated file has never been loaded yet.
struct filebuf_s
{
    #define b_fnum handle
    handle_kt handle;  ///< unique id for the buffer (buffer number)
    memline_T b_ml;    ///< associated memline (also contains line count
    filebuf_st *b_next;     ///< links in list of buffers
    filebuf_st *b_prev;

    int b_nwindows;   ///< nr of windows open on this buffer
    int b_flags;      ///< various BF_ flags
    int b_locked;     ///< Buffer is being closed or referenced, don't
                      ///< let autocommands wipe it out.

    // b_ffname has the full path of the file (NULL for no name).
    // b_sfname is the name as the user typed it (or NULL).
    // b_fname is the same as b_sfname, unless ":cd" has been done,
    // then it is the same as b_ffname (NULL for no name).
    uchar_kt *b_ffname; ///< full path file name
    uchar_kt *b_sfname; ///< short file name
    uchar_kt *b_fname;  ///< current file name

    bool file_id_valid;
    fileid_st file_id;

    /// 'modified': Set to true if something in the
    /// file has been changed and not written out.
    int b_changed;
    /// Change identifier incremented for each change, including undo
    #define b_changedtick changedtick_di.di_tv.vval.v_number
    /// b:changedtick dictionary item.
    ChangedtickDictItem changedtick_di;
    /// Set to true if we are in the middle of saving the buffer.
    bool b_saving;

    // Changes to a buffer require updating of the display.
    // To minimize the work, remember changes made and update everything at once.

    /// true when there are changes since the last time the display was updated
    bool b_mod_set;

    linenum_kt b_mod_top;       ///< topmost lnum that was changed
    linenum_kt b_mod_bot;       ///< lnum below last changed line, AFTER the change
    long b_mod_xlines;        ///< number of extra buffer lines inserted
                              ///< negative when lines were deleted
    wininfo_st *b_wininfo;     ///< list of last used info for each window
    long b_mtime;             ///< last change time of original file
    long b_mtime_read;        ///< last change time when reading
    uint64_t b_orig_size;     ///< size of original file in bytes
    int b_orig_mode;          ///< mode of original file
    filemark_st b_namedm[NMARKS]; ///< current named marks (mark.c)
    visualinfo_st b_visual;    ///< These variables are set when VIsual_active becomes FALSE
    int b_visual_mode_eval;   ///< b_visual.vi_mode for visualmode()
    filemark_st b_last_cursor;    ///< cursor position when last unloading this
    filemark_st b_last_insert;    ///< where Insert mode was left
    filemark_st b_last_change;    ///< position of last change: '. mark'


    /// the changelist contains old change positions
    filemark_st b_changelist[JUMPLISTSIZE];
    int b_changelistlen;  ///< number of active entries
    bool b_new_change;    ///< set by u_savecommon()

    /// Character table, only used in charset.c for 'iskeyword'
    /// bitset with 4*64=256 bits: 1 bit per character 0-255.
    uint64_t b_chartab[4];

    /// Table used for mappings local to a buffer.
    mapblock_T *(b_maphash[MAX_MAPHASH]);

    /// First abbreviation local to a buffer.
    mapblock_T *b_first_abbr;

    /// User commands local to the buffer.
    garray_st b_ucmds;

    /// start and end of an operator, also used for '[ and ']
    apos_st b_op_start;

    /// used for Insstart_orig
    apos_st b_op_start_orig;
    apos_st b_op_end;

    /// Have we read ShaDa marks yet ?
    bool b_marks_read;

    // The following only used in undo.c.
    u_header_T *b_u_oldhead; ///< pointer to oldest header
    u_header_T *b_u_newhead; ///< pointer to newest header, may not be valid
                             ///< if b_u_curhead is not NULL
    u_header_T *b_u_curhead; ///< pointer to current header
    int b_u_numhead;         ///< current number of headers
    bool b_u_synced;         ///< entry lists are synced
    long b_u_seq_last;       ///< last used undo sequence number
    long b_u_save_nr_last;   ///< counter for last file write
    long b_u_seq_cur;        ///< hu_seq of header below which we are now
    time_t b_u_time_cur;     ///< uh_time of header below which we are now
    long b_u_save_nr_cur;    ///< file write nr after which we are now

    // variables for "U" command in undo.c
    uchar_kt *b_u_line_ptr;     ///< saved line for "U" command
    linenum_kt b_u_line_lnum;   ///< line number of line in u_line
    columnum_kt b_u_line_colnr; ///< optional column number

    /// ^N/^P have scanned this buffer
    bool b_scanned;

    // flags for use of ":lmap" and IM control
    long b_p_iminsert;  ///< input mode for insert
    long b_p_imsearch;  ///< input mode for search

#define B_IMODE_USE_INSERT -1    ///< Use b_p_iminsert value for search
#define B_IMODE_NONE        0    ///< Input via none
#define B_IMODE_LMAP        1    ///< Input via langmap
#define B_IMODE_LAST        1

    short b_kmap_state;          ///< using "lmap" mappings
#define KEYMAP_INIT         1    ///< 'keymap' was set, call keymap_init()
#define KEYMAP_LOADED       2    ///< 'keymap' mappings have been loaded
    garray_st b_kmap_ga;         ///< the keymap table

    //////////////////////////////////////////////////////////////
    // Options local to a buffer.
    // They are here because their value depends on the type of file
    // or contents of the file being edited.
    bool b_p_initialized;         ///< set when options initialized
    int b_p_scriptID[BV_COUNT];   ///< SIDs for buffer-local options

    int b_p_ai;                   ///< 'autoindent'
    int b_p_ai_nopaste;           ///< b_p_ai saved for paste mode
    uchar_kt *b_p_bkc;            ///< 'backupco
    unsigned int b_bkc_flags;     ///< flags for 'backupco
    int b_p_ci;                   ///< 'copyindent'
    int b_p_bin;                  ///< 'binary'
    int b_p_bomb;                 ///< 'bomb'
    uchar_kt *b_p_bh;             ///< 'bufhidden'
    uchar_kt *b_p_bt;             ///< 'buftype'
    int b_has_qf_entry;           ///< quickfix exists for buffer
    int b_p_bl;                   ///< 'buflisted'
    int b_p_cin;                  ///< 'cindent'
    uchar_kt *b_p_cino;           ///< 'cinoptions'
    uchar_kt *b_p_cink;           ///< 'cinkeys'
    uchar_kt *b_p_cinw;           ///< 'cinwords'
    uchar_kt *b_p_com;            ///< 'comments'
    uchar_kt *b_p_cms;            ///< 'commentstring'
    uchar_kt *b_p_cpt;            ///< 'complete'
    uchar_kt *b_p_cfu;            ///< 'completefunc'
    uchar_kt *b_p_ofu;            ///< 'omnifunc'
    int b_p_eol;                  ///< 'endofline'
    int b_p_fixeol;               ///< 'fixendofline'
    int b_p_et;                   ///< 'expandtab'
    int b_p_et_nobin;             ///< 'b_p_et' saved for binary mode
    int b_p_et_nopaste;           ///< 'b_p_et' saved for paste mode
    uchar_kt *b_p_fenc;           ///< 'fileencoding'
    uchar_kt *b_p_ff;             ///< 'fileformat'
    uchar_kt *b_p_ft;             ///< 'filetype'
    uchar_kt *b_p_fo;             ///< 'formatoptions'
    uchar_kt *b_p_flp;            ///< 'formatlistpat'
    int b_p_inf;                  ///< 'infercase'
    uchar_kt *b_p_isk;            ///< 'iskeyword'
    uchar_kt *b_p_def;            ///< 'define' local value
    uchar_kt *b_p_inc;            ///< 'include'
    uchar_kt *b_p_inex;           ///< 'includeexpr'
    uint32_t b_p_inex_flags;      ///< flags for 'includeexpr'
    uchar_kt *b_p_inde;           ///< 'indentexpr'
    uint32_t b_p_inde_flags;      ///< flags for 'indentexpr'
    uchar_kt *b_p_indk;           ///< 'indentkeys'
    uchar_kt *b_p_fp;             ///< 'formatprg'
    uchar_kt *b_p_fex;            ///< 'formatexpr'
    uint32_t b_p_fex_flags;       ///< flags for 'formatexpr'
    uchar_kt *b_p_kp;             ///< 'keywordprg'
    int b_p_lisp;                 ///< 'lisp'
    uchar_kt *b_p_mps;            ///< 'matchpairs'
    int b_p_ml;                   ///< 'modeline'
    int b_p_ml_nobin;             ///< 'b_p_ml' saved for binary mode
    int b_p_ma;                   ///< 'modifiable'
    uchar_kt *b_p_nf;             ///< 'nrformats'
    int b_p_pi;                   ///< 'preserveindent'
    uchar_kt *b_p_qe;             ///< 'quoteescape'
    int b_p_ro;                   ///< 'readonly'
    long b_p_sw;                  ///< 'shiftwidth'
    long b_p_scbk;                ///< 'scrollback'
    int b_p_si;                   ///< 'smartindent'
    long b_p_sts;                 ///< 'softtabstop'
    long b_p_sts_nopaste;         ///< 'b_p_sts' saved for paste mode
    uchar_kt *b_p_sua;            ///< 'suffixesadd'
    int b_p_swf;                  ///< 'swapfile'
    long b_p_smc;                 ///< 'synmaxcol'
    uchar_kt *b_p_syn;            ///< 'syntax'
    long b_p_ts;                  ///< 'tabstop'
    long b_p_tw;                  ///< 'textwidth'
    long b_p_tw_nobin;            ///< 'b_p_tw' saved for binary mode
    long b_p_tw_nopaste;          ///< 'b_p_tw saved for paste mode
    long b_p_wm;                  ///< 'wrapma'rgin'
    long b_p_wm_nobin;            ///< 'b_p_wm' saved for binary mode
    long b_p_wm_nopaste;          ///< 'b_p_wm' saved for paste mode
    uchar_kt *b_p_keymap;         ///< 'keymap'

    // local values for options which are normally global
    uchar_kt *b_p_gp;       ///< 'grepprg' local value
    uchar_kt *b_p_mp;       ///< 'makeprg' local value
    uchar_kt *b_p_efm;      ///< 'errorformat' local value
    uchar_kt *b_p_ep;       ///< 'equalprg' local value
    uchar_kt *b_p_path;     ///< 'path' local value
    int b_p_ar;             ///< 'autoread' local value
    uchar_kt *b_p_tags;     ///< 'tags' local value
    uchar_kt *b_p_tc;       ///< 'tagcase' local value
    unsigned b_tc_flags;    ///< flags for 'tagcase'
    uchar_kt *b_p_dict;     ///< 'dictionary' local value
    uchar_kt *b_p_tsr;      ///< 'thesaurus' local value
    long b_p_ul;            ///< 'undolevels' local value
    int b_p_udf;            ///< 'undofile'
    uchar_kt *b_p_lw;       ///< 'lispwords' local value
    // end of buffer options
    //////////////////////////////////////////////////////////////

    // values set from b_p_cino
    int b_ind_level;
    int b_ind_open_imag;
    int b_ind_no_brace;
    int b_ind_first_open;
    int b_ind_open_extra;
    int b_ind_close_extra;
    int b_ind_open_left_imag;
    int b_ind_jump_label;
    int b_ind_case;
    int b_ind_case_code;
    int b_ind_case_break;
    int b_ind_param;
    int b_ind_func_type;
    int b_ind_comment;
    int b_ind_in_comment;
    int b_ind_in_comment2;
    int b_ind_cpp_baseclass;
    int b_ind_continuation;
    int b_ind_unclosed;
    int b_ind_unclosed2;
    int b_ind_unclosed_noignore;
    int b_ind_unclosed_wrapped;
    int b_ind_unclosed_whiteok;
    int b_ind_matching_paren;
    int b_ind_paren_prev;
    int b_ind_maxparen;
    int b_ind_maxcomment;
    int b_ind_scopedecl;
    int b_ind_scopedecl_code;
    int b_ind_java;
    int b_ind_js;
    int b_ind_keep_case_label;
    int b_ind_hash_comment;
    int b_ind_cpp_namespace;
    int b_ind_if_for_while;

    linenum_kt b_no_eol_lnum; ///< non-zero lnum when last line of next binary
                              ///< write should not have an end-of-line
    int b_start_eol;          ///< last line had eol when it was read
    int b_start_ffc;          ///< first char of 'ff' when edit started
    uchar_kt *b_start_fenc;   ///< 'fileencoding' when edit started or NULL
    int b_bad_char;           ///< "++bad=" argument when edit started or 0
    int b_start_bomb;         ///< 'bomb' when it was read */

    scope_dict_st b_bufvar;    ///< Variable for "b:" Dictionary.
    dict_st *b_vars;           ///< b: scope dictionary.

    // When a buffer is created, it starts without a swap file. b_may_swap is
    // then set to indicate that a swap file may be opened later. It is reset
    // if a swap file could not be opened.
    bool b_may_swap;
    /// Set to true if user has been warned on first change of a read-only file
    bool b_did_warn;

    // Two special kinds of buffers:
    // help buffer:
    //     used for help files, won't use a swap file.
    // spell buffer:
    //    used for spell info, never displayed and doesn't have a file name.
    bool b_help;  ///< TRUE for help file buffer (when set b_p_bt is "help")

    bool b_spell; ///< True for a spell file buffer, most fields
                  ///< are not used!  Use the B_SPELL macro to
                  ///< access b_spell without #ifdef.

    /// Info related to syntax highlighting. w_s normally points
    /// to this, but some windows may use a different synblock_T.
    synblock_T b_s;

    signlist_T *b_signlist;     ///< list of signs to draw
    Terminal *terminal;         ///< Terminal instance associated with the buffer
    dict_st *additional_data;   ///< Additional data from shada file if any.
    int b_mapped_ctrl_c;        ///< modes where CTRL-C is mapped
    bufhl_info_T *b_bufhl_info; ///< buffer stored highlights
};

/// Stuff for diff mode.
#define DB_COUNT   8  ///< up to four buffers can be diff'ed

/// Each diffblock defines where a block of lines starts in each of the buffers
/// and how many lines it occupies in that buffer.  When the lines are missing
/// in the buffer the df_count[] is zero.  This is all counted in
/// buffer lines.
/// There is always at least one unchanged line in between the diffs.
/// Otherwise it would have been included in the diff above or below it.
/// df_lnum[] + df_count[] is the lnum below the change.  When in one buffer
/// lines have been inserted, in the other buffer df_lnum[] is the line below
/// the insertion and df_count[] is zero.  When appending lines at the end of
/// the buffer, df_lnum[] is one beyond the end!
/// This is using a linked list, because the number of differences is expected
/// to be reasonable small.  The list is sorted on lnum.
typedef struct diffblock_S diff_T;
struct diffblock_S
{
    diff_T *df_next;
    linenum_kt df_lnum[DB_COUNT];  ///< line number in buffer
    linenum_kt df_count[DB_COUNT]; ///< nr of inserted/changed lines
};

#define SNAP_HELP_IDX     0
#define SNAP_AUCMD_IDX    1
#define SNAP_COUNT        2

/// Tab pages point to the top frame of each tab page.
/// Note: Most values are NOT valid for the current tab page! Use "curwin",
/// "firstwin", etc. for that. "tp_topframe" is always valid and can be
/// compared against "topframe" to find the current tab page.
typedef struct tabpage_s tabpage_st;
struct tabpage_s
{
    handle_kt handle;
    tabpage_st *tp_next;    ///< next tabpage or NULL
    frame_st *tp_topframe;  ///< topframe for the windows
    win_st *tp_curwin;      ///< current window in this Tab page
    win_st *tp_prevwin;     ///< previous window in this Tab page
    win_st *tp_firstwin;    ///< first window in this Tab page
    win_st *tp_lastwin;     ///< last window in this Tab page
    long tp_old_Rows;       ///< Rows when Tab page was left
    long tp_old_Columns;    ///< Columns when Tab page was left
    long tp_ch_used;        ///< value of 'cmdheight' when frame size was set

    diff_T *tp_first_diff;
    filebuf_st *(tp_diffbuf[DB_COUNT]);
    int tp_diff_invalid;                ///< list of diffs is outdated
    frame_st *(tp_snapshot[SNAP_COUNT]);///< window layout snapshots
    scope_dict_st tp_winvar;            ///< Variable for "t:" Dictionary.
    dict_st *tp_vars;                   ///< Internal variables, local to tab page.
    uchar_kt *tp_localdir;              ///< Absolute path of local cwd or NULL.
};

/// Structure to cache info for displayed lines in w_lines[].
/// Each logical line has one entry.
/// The entry tells how the logical line is currently displayed in the window.
/// This is updated when displaying the window.
/// When the display is changed (e.g., when clearing the screen) w_lines_valid
/// is changed to exclude invalid entries.
/// When making changes to the buffer, wl_valid is reset to indicate wl_size
/// may not reflect what is actually in the buffer.  When wl_valid is FALSE,
/// the entries can only be used to count the number of displayed lines used.
/// wl_lnum and wl_lastlnum are invalid too.
typedef struct w_line
{
    linenum_kt wl_lnum;     ///< buffer line number for logical line
    uint16_t wl_size;       ///< height in screen lines
    char wl_valid;          ///< TRUE values are valid for text in buffer
    char wl_folded;         ///< TRUE when this is a range of folded lines
    linenum_kt wl_lastlnum; ///< last buffer line number for logical line
} wline_T;

/// Windows are kept in a tree of frames.  Each frame has a column (FR_COL)
/// or row (FR_ROW) layout or is a leaf, which has a window.
struct frame_s
{
    char fr_layout;      ///< FR_LEAF, FR_COL or FR_ROW
    int fr_width;
    int fr_newwidth;     ///< new width used in win_equal_rec()
    int fr_height;
    int fr_newheight;    ///< new height used in win_equal_rec()
    frame_st *fr_parent; ///< containing frame or NULL
    frame_st *fr_next;   ///< frame right or below in same parent, NULL for first
    frame_st *fr_prev;   ///< frame left or above in same parent, NULL  for last
                         ///< fr_child and fr_win are mutually exclusive
    frame_st *fr_child;  ///< first contained frame
    win_st *fr_win;      ///< window that fills this frame
};

#define FR_LEAF   0   ///< frame is a leaf
#define FR_ROW    1   ///< frame with a row of windows
#define FR_COL    2   ///< frame with a column of windows

/// Struct used for highlighting 'hlsearch' matches, matches defined by
/// ":match" and matches defined by match functions.
/// For 'hlsearch' there is one pattern for all windows.  For ":match" and the
/// match functions there is a different pattern for each window.
typedef struct
{
    regmmatch_st rm;       ///< points to the regexp program; contains last found
                          ///< match (may continue in next line)
    filebuf_st *buf;           ///< the buffer to search for a match
    linenum_kt lnum;        ///< the line to search for a match
    int attr;             ///< attributes to be used for a match
    int attr_cur;         ///< attributes currently active in win_line()
    linenum_kt first_lnum;  ///< first lnum to search for multi-line pat
    columnum_kt startcol;     ///< in win_line() points to char where HL starts
    columnum_kt endcol;       ///< in win_line() points to char where HL ends
    bool is_addpos;       ///< position specified directly by matchaddpos()
    proftime_kt tm;        ///< for a time limit
} match_T;

/// number of positions supported by matchaddpos()
#define MAXPOSMATCH 8

/// Same as bpos_st, but with additional field len.
typedef struct
{
    linenum_kt  lnum; ///< line number
    columnum_kt col;  ///< column number
    int len;          ///< length: 0 - to the end of line
} cpos_st;

/// posmatch_T provides an array for storing match items
/// for matchaddpos() function.
typedef struct posmatch posmatch_T;
struct posmatch
{
    cpos_st pos[MAXPOSMATCH]; ///< array of positions
    int cur;                  ///< internal position counter
    linenum_kt toplnum;         ///< top buffer line
    linenum_kt botlnum;         ///< bottom buffer line
};

/// matchitem_T provides a linked list for storing
/// match items for ":match" and the match functions.
typedef struct matchitem matchitem_T;
struct matchitem
{
    matchitem_T *next;
    int id;             ///< match ID
    int priority;       ///< match priority
    uchar_kt *pattern;  ///< pattern to highlight
    int hlg_id;         ///< highlight group ID
    regmmatch_st match;  ///< regexp program for pattern
    posmatch_T pos;     ///< position matches
    match_T hl;         ///< struct for doing the actual highlighting
    int conceal_char;   ///< cchar for Conceal highlighting
};

/// Structure which contains all information that belongs to a window
///
/// All row numbers are relative to the start of the window, except w_winrow.
struct window_s
{
    handle_kt handle; ///< unique identifier for the window

    /// buffer we are a window into (used often, keep it the first item!)
    filebuf_st *w_buffer;

    synblock_T  *w_s;     ///< for :ownsyntax
    int w_hl_id;          ///< 'winhighlight' id
    int w_hl_id_inactive; ///< 'winhighlight' id for inactive window
    int w_hl_attr;        ///< 'winhighlight' final attrs

    win_st *w_prev; ///< link to previous window
    win_st *w_next; ///< link to next window

    /// window is being closed, don't let autocommands close it too
    bool w_closing;

    frame_st *w_frame; ///< frame containing this window
    apos_st w_cursor;   ///< cursor position in buffer

    /// The column we'd like to be at. This is used to try to
    /// stay in the same column for up/down cursor motions.
    columnum_kt w_curswant;

    /// If set, then update w_curswant the next time through
    /// cursupdate() to the current virtual column
    int w_set_curswant;

    // the next seven are used to update the visual part
    char w_old_visual_mode;      ///< last known VIsual_mode
    linenum_kt w_old_cursor_lnum;  ///< last known end of visual part
    columnum_kt w_old_cursor_fcol;   ///< first column for block visual part
    columnum_kt w_old_cursor_lcol;   ///< last column for block visual part
    linenum_kt w_old_visual_lnum;  ///< last known start of visual part
    columnum_kt w_old_visual_col;    ///< last known start of visual part
    columnum_kt w_old_curswant;      ///< last known value of Curswant

    // "w_topline", "w_leftcol" and "w_skipcol" specify the offsets for displaying the buffer.
    linenum_kt w_topline;     ///< buffer line number of the line at the top of the window
    char w_topline_was_set; ///< flag set to TRUE when topline is set, e.g. by winrestview()
    int w_topfill;          ///< number of filler lines above w_topline
    int w_old_topfill;      ///< w_topfill at last redraw
    bool w_botfill;         ///< true when filler lines are actually below w_topline (at end of file)
    bool w_old_botfill;     ///< w_botfill at last redraw

    /// window column number of the left most character
    /// in the window; used when 'wrap' is off
    columnum_kt w_leftcol;

    /// starting column when a single line doesn't fit in the window
    columnum_kt w_skipcol;

    // Layout of the window in the screen.
    // May need to add "msg_scrolled" to "w_winrow" in rare situations.
    int w_winrow; ///< first row of window in screen
    int w_height; ///< number of rows in window, excluding status/command line(s)

    /// number of status lines (0 or 1)
    int w_status_height;

    int w_wincol;     ///< Leftmost column of window in screen.
    int w_width;      ///< Width of window, excluding separation.
    int w_vsep_width; ///< Number of separator columns (0 or 1).

    // -----------------  start of cached values -----------------
    //
    // Recomputing is minimized by storing the result of computations.
    // Use functions in screen.c to check if they are valid and to update.
    // w_valid is a bitfield of flags, which indicate if specific values are
    // valid or need to be recomputed.
    int w_valid;
    apos_st w_valid_cursor;    ///< last known position of w_cursor, used  to adjust w_valid
    columnum_kt w_valid_leftcol; ///< last known w_leftcol

    // w_cline_height is the number of physical lines taken by the buffer line
    // that the cursor is on. We use this to avoid extra calls to plines().
    int w_cline_height;  ///< current size of cursor line
    bool w_cline_folded; ///< cursor line is folded
    int w_cline_row;     ///< starting row of the cursor line

    /// column number of the cursor in the buffer line, as opposed to the column
    /// number we're at on the screen. This makes a difference on lines which span
    /// more than one screen line or when w_leftcol is non-zero
    columnum_kt w_virtcol;

    // w_wrow and w_wcol specify the cursor position in the window.
    // This is related to positions in the window, not in the display or
    // buffer, thus w_wrow is relative to w_winrow.
    int w_wrow, w_wcol; ///< cursor position in window
    linenum_kt w_botline; ///< number of the line below the bottom of the window
    int w_empty_rows;   ///< number of ~ rows in window
    int w_filler_rows;  ///< number of filler rows at the end of the window

    // Info about the lines currently in the window is remembered to avoid
    // recomputing it every time.  The allocated size of w_lines[] is Rows.
    // Only the w_lines_valid entries are actually valid.
    // When the display is up-to-date w_lines[0].wl_lnum is equal to w_topline
    // and w_lines[w_lines_valid - 1].wl_lnum is equal to w_botline.
    // Between changing text and updating the display w_lines[] represents
    // what is currently displayed.  wl_valid is reset to indicated this.
    // This is used for efficient redrawing.
    int w_lines_valid;  ///< number of valid entries
    wline_T *w_lines;
    garray_st w_folds;   ///< array of nested folds
    bool w_fold_manual; ///< when true: some folds are opened/closed  manually
    bool w_foldinvalid; ///< when true: folding needs to be recomputed
    int w_nrwidth;      ///< width of 'number' and 'relativenumber' column being used

    // -----------------  end of cached values -----------------

    int w_redr_type;       ///< type of redraw to be performed on win
    int w_upd_rows;        ///< number of window lines to update when w_redr_type is REDRAW_TOP
    linenum_kt w_redraw_top; ///< when != 0: first line needing redraw
    linenum_kt w_redraw_bot; ///< when != 0: last line needing redraw
    int w_redr_status;     ///< if TRUE status line must be redrawn

    // remember what is shown in the ruler for this window (if 'ruler' set)
    apos_st w_ru_cursor;        ///< cursor position shown in ruler
    columnum_kt w_ru_virtcol;     ///< virtcol shown in ruler
    linenum_kt w_ru_topline;    ///< topline shown in ruler
    linenum_kt w_ru_line_count; ///< line count used for ruler
    int w_ru_topfill;         ///< topfill shown in ruler
    char w_ru_empty;          ///< TRUE if ruler shows 0-1 (empty line)
    int w_alt_fnum;           ///< alternate file (for # and CTRL-^)

    arglist_st *w_alist;      ///< pointer to arglist_st for this window
    int w_arg_idx;         ///< current index in argument list (can be out of range!)
    int w_arg_idx_invalid; ///< editing another file than w_arg_idx
    uchar_kt *w_localdir;  ///< absolute path of local directory or NULL

    // Options local to a window.
    // They are local because they influence the layout of the window or
    // depend on the window layout.
    // There are two values: w_onebuf_opt is local to the buffer currently in
    // this window, w_allbuf_opt is for all buffers in this window.
    winopt_T w_onebuf_opt;
    winopt_T w_allbuf_opt;

    // A few options have local flags for P_INSECURE.
    uint32_t w_p_stl_flags;  ///< flags for 'statusline'
    uint32_t w_p_fde_flags;  ///< flags for 'foldexpr'
    uint32_t w_p_fdt_flags;  ///< flags for 'foldtext'

    int *w_p_cc_cols;  ///< array of columns to highlight or NULL
    int w_p_brimin;    ///< minimum width for breakindent
    int w_p_brishift;  ///< additional shift for breakindent
    bool w_p_brisbr;   ///< sbr in 'briopt'

    /// transform a pointer to a "onebuf" option into a "allbuf" option
    #define GLOBAL_WO(p)    ((char *)p + sizeof(winopt_T))

    long w_scbind_pos;

    /// Variable for "w:" dictionary.
    scope_dict_st w_winvar;

    /// Dictionary with w: variables.
    dict_st *w_vars;

    /// for the window dependent Farsi functions
    int w_farsi;

    // The w_prev_pcmark field is used to check whether we really did jump to
    // a new line after setting the w_pcmark. If not, then we revert to
    // using the previous w_pcmark.
    apos_st w_pcmark;      ///< previous context mark
    apos_st w_prev_pcmark; ///< previous w_pcmark

    /// contains old cursor positions
    xfilemark_st w_jumplist[JUMPLISTSIZE];
    int w_jumplistlen;         ///< number of active entries
    int w_jumplistidx;         ///< current position
    int w_changelistidx;       ///< current position in b_changelist
    matchitem_T *w_match_head; ///< head of match list
    int w_next_match_id;       ///< next match ID

    // the tagstack grows from 0 upwards:
    // entry 0: older
    // entry 1: newer
    // entry 2: newest
    taggy_st w_tagstack[TAGSTACKSIZE]; ///< the tag stack
    int w_tagstackidx; ///< idx just below active entry
    int w_tagstacklen; ///< number of tags on stack

    // w_fraction is the fractional row of the cursor within the window, from
    // 0 at the top row to FRACTION_MULT at the last row.
    // w_prev_fraction_row was the actual cursor row when w_fraction was last
    // calculated.
    int w_fraction;
    int w_prev_fraction_row;

    /// line count when ml_nrwidth_width was computed.
    linenum_kt w_nrwidth_line_count;
    int w_nrwidth_width;  ///< nr of chars to print line count.
    qf_info_T *w_llist;   ///< Location list for this window

    /// Location list reference used in the location list window.
    /// In a non-location list window, w_llist_ref is NULL.
    qf_info_T *w_llist_ref;
};
#endif // NVIM_BUFFER_DEFS_H
