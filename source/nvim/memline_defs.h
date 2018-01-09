/// @file nvim/memline_defs.h

#ifndef NVIM_MEMLINE_DEFS_H
#define NVIM_MEMLINE_DEFS_H

#include "nvim/memfile_defs.h"

/// When searching for a specific line, we remember what blocks in the tree
/// are the branches leading to that block. This is stored in ml_stack. Each
/// entry is a pointer to info in a block (may be data block or pointer block)
typedef struct info_pointer
{
    blocknr_T ip_bnum;  ///< block number
    linenum_kt ip_low;    ///< lowest lnum in this block
    linenum_kt ip_high;   ///< highest lnum in this block
    int ip_index;       ///< index for block with current lnum block/index pair
} infoptr_T;

typedef struct ml_chunksize
{
    int mlcs_numlines;
    long mlcs_totalsize;
} chunksize_T;

// Flags when calling ml_updatechunk()
#define ML_CHNK_ADDLINE    1
#define ML_CHNK_DELLINE    2
#define ML_CHNK_UPDLINE    3

/// the memline structure holds all the information about a memline
typedef struct memline
{
    linenum_kt ml_line_count;     ///< number of lines in the buffer

    memfile_T *ml_mfp;          ///< pointer to associated memfile

    #define ML_EMPTY        1   ///< empty buffer
    #define ML_LINE_DIRTY   2   ///< cached line was changed and allocated
    #define ML_LOCKED_DIRTY 4   ///< ml_locked was changed
    #define ML_LOCKED_POS   8   ///< ml_locked needs positive block number
    int ml_flags;

    infoptr_T *ml_stack;        ///< stack of pointer blocks (array of IPTRs)
    int ml_stack_top;           ///< current top of ml_stack
    int ml_stack_size;          ///< total number of entries in ml_stack

    linenum_kt ml_line_lnum;      ///< line number of cached line, 0 if not valid
    uchar_kt *ml_line_ptr;        ///< pointer to cached line

    blk_hdr_st *ml_locked;          ///< block used by last ml_get
    linenum_kt ml_locked_low;     ///< first line in ml_locked
    linenum_kt ml_locked_high;    ///< last line in ml_locked
    int ml_locked_lineadd;      ///< number of lines inserted in ml_locked
    chunksize_T *ml_chunksize;

    int ml_numchunks;
    int ml_usedchunks;
} memline_T;

#endif // NVIM_MEMLINE_DEFS_H
