/// @file nvim/memline_defs.h

#ifndef NVIM_MEMLINE_DEFS_H
#define NVIM_MEMLINE_DEFS_H

#include "nvim/memfile_defs.h"

/// When searching for a specific line, we remember what blocks in the tree
/// are the branches leading to that block. This is stored in ml_stack. Each
/// entry is a pointer to info in a block (may be data block or pointer block)
typedef struct infoptr_s
{
    blknum_kt ip_bnum;  ///< block number
    linenum_kt ip_low;  ///< lowest lnum in this block
    linenum_kt ip_high; ///< highest lnum in this block
    int ip_index;       ///< index for block with current lnum block/index pair
} infoptr_st;

/// memory line chunk size
typedef struct mlchksize_s
{
    int mlcs_numlines;
    long mlcs_totalsize;
} mlchksize_st;

/// Flags when calling ml_updatechunk()
enum
{
    kMLCLineAdd = 1, ///< memory line chunk line: add
    kMLCLineDel = 2, ///< memory line chunk line: delete
    kMLCLineUpd = 3, ///< memory line chunk line: update
};

/// flags used for memline_s::ml_flags
enum memline_flg_e
{
    kMLflgBufEmpty    = 1, ///< empty buffer
    kMLflgLineDirty   = 2, ///< cached line was changed and allocated
    kMLflgLockedDirty = 4, ///< memline_s::ml_locked was changed
    kMLflgLockedPos   = 8  ///< memline_s::ml_locked needs positive block number
};

/// the memline_s structure holds all the information about a memline
typedef struct memline_s
{
    linenum_kt ml_line_count;   ///< number of lines in the buffer
    memfile_st *ml_mfp;         ///< pointer to associated memfile
    int ml_flags;               ///< memline_flg_e
    infoptr_st *ml_stack;       ///< stack of pointer blocks (array of IPTRs)
    int ml_stack_top;           ///< current top of ml_stack
    int ml_stack_size;          ///< total number of entries in ml_stack

    linenum_kt ml_line_lnum;    ///< line number of cached line, 0 if not valid
    uchar_kt *ml_line_ptr;      ///< pointer to cached line

    blk_hdr_st *ml_locked;      ///< block used by last ml_get
    linenum_kt ml_locked_low;   ///< first line in ml_locked
    linenum_kt ml_locked_high;  ///< last line in ml_locked
    int ml_locked_lineadd;      ///< number of lines inserted in ml_locked
    mlchksize_st *ml_chunksize;

    int ml_numchunks;
    int ml_usedchunks;
} memline_st;

#endif // NVIM_MEMLINE_DEFS_H
