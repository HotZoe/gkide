/// @file nvim/undo_defs.h

#ifndef NVIM_UNDO_DEFS_H
#define NVIM_UNDO_DEFS_H

#include <time.h>

#include "nvim/pos.h"
#include "nvim/buffer_defs.h"
#include "nvim/mark_defs.h"

/// Structure to store info about the Visual area.
typedef struct
{
    apos_st vi_start;        ///< start pos of last VIsual
    apos_st vi_end;          ///< end position of last VIsual
    int vi_mode;             ///< VIsual_mode of last VIsual
    columnum_kt vi_curswant; ///< MAXCOL from w_curswant
} visualinfo_st;

typedef struct undo_blk_s   undo_blk_st; ///< undo entry block
typedef struct undo_hdr_s   undo_hdr_st; ///< undo header block

struct undo_blk_s
{
    undo_blk_st *ue_next;   ///< pointer to next entry in list
    linenum_kt ue_top;    ///< number of line above undo block
    linenum_kt ue_bot;    ///< number of line below undo block
    linenum_kt ue_lcount; ///< linecount when u_save called
    uchar_kt **ue_array;  ///< array of lines in undo block
    long ue_size;         ///< number of lines in ue_array

#ifdef U_DEBUG
    int ue_magic;         ///< magic number to check allocation
#endif
};

/// The following have a pointer and a number.
/// The number is used when reading the undo file in u_read_undo()
struct undo_hdr_s
{
    union
    {
        undo_hdr_st *ptr; ///< pointer to next undo header in list
        long seq;
    } uh_next;
    union
    {
        undo_hdr_st *ptr; ///< pointer to previous header in list
        long seq;
    } uh_prev;
    union
    {
        undo_hdr_st *ptr; ///< pointer to next header for alt. redo
        long seq;
    } uh_alt_next;
    union
    {
        undo_hdr_st *ptr; ///< pointer to previous header for alt. redo
        long seq;
    } uh_alt_prev;

    long uh_seq;                  ///< sequence number, higher == newer undo
    int uh_walk;                  ///< used by undo_time()
    undo_blk_st *uh_entry;        ///< pointer to first entry
    undo_blk_st *uh_getbot_entry; ///< pointer to where ue_bot must be set
    apos_st uh_cursor;            ///< cursor position before saving
    long uh_cursor_vcol;
    int uh_flags;                 ///< see below
    mark_st uh_namedm[NMARKS];///< marks before undo/after redo
    visualinfo_st uh_visual;      ///< Visual areas before undo/after redo
    time_t uh_time;               ///< timestamp when the change was made
    long uh_save_nr;              ///< set when the file was saved after
                                  ///< the changes in this block
#ifdef U_DEBUG
    int uh_magic;                 ///< magic number to check allocation
#endif
};

// values for uh_flags
#define UH_CHANGED    0x01  ///< b_changed flag before undo/after redo
#define UH_EMPTYBUF   0x02  ///< buffer was empty

/// Structure passed around between undofile functions.
typedef struct
{
    filebuf_st *bi_buf;
    FILE *bi_fp;
} undobuf_st;

#endif // NVIM_UNDO_DEFS_H
