/// @file nvim/fold.h

#ifndef NVIM_FOLD_H
#define NVIM_FOLD_H

#include <stdio.h>

#include "nvim/pos.h"
#include "nvim/garray.h"
#include "nvim/types.h"
#include "nvim/buffer_defs.h"

/// Info used to pass info about a fold from the fold-detection
/// code to the code that displays the foldcolumn.
typedef struct foldinfo_s
{
    /// line number where fold starts
    linenum_kt fi_lnum;
    /// level of the fold; when this is zero the other fields are invalid
    int fi_level;
    /// lowest fold level that starts in the same line
    int fi_low_level;
} foldinfo_st;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "fold.h.generated.h"
#endif

#endif // NVIM_FOLD_H
