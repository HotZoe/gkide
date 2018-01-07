/// @file nvim/bufhl_defs.h

#ifndef NVIM_BUFHL_DEFS_H
#define NVIM_BUFHL_DEFS_H

#include "nvim/pos.h"
#include "nvim/lib/kvec.h"

/// buffer specific highlighting
struct bufhl_hl_item
{
    int src_id;
    int hl_id;      ///< highlight group
    columnum_kt start;  ///< first column to highlight
    columnum_kt stop;   ///< last column to highlight
};
typedef struct bufhl_hl_item bufhl_hl_item_T;

typedef kvec_t(struct bufhl_hl_item) bufhl_vec_T;

typedef struct
{
    bufhl_vec_T entries;
    int current;
    columnum_kt valid_to;
} bufhl_lineinfo_T;

#endif  // NVIM_BUFHL_DEFS_H
