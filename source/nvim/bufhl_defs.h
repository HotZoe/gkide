/// @file nvim/bufhl_defs.h

#ifndef NVIM_BUFHL_DEFS_H
#define NVIM_BUFHL_DEFS_H

#include "nvim/pos.h"
#include "nvim/lib/kvec.h"

typedef struct bufhl_s bufhl_item_st;
typedef kvec_t(struct bufhl_s) bufhl_vec_st;

/// buffer specific highlighting
struct bufhl_s
{
    int src_id;
    int hl_id;         ///< highlight group
    columnum_kt start; ///< first column to highlight
    columnum_kt stop;  ///< last column to highlight
};

typedef struct bufhl_lineinfo_s
{
    bufhl_vec_st entries;
    int current;
    columnum_kt valid_to;
} bufhl_lineinfo_st;

#endif // NVIM_BUFHL_DEFS_H
