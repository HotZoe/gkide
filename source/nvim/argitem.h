/// @file nvim/argitem.h
///
/// common argument item used else where.

#ifndef NVIM_ARGITEM_H
#define NVIM_ARGITEM_H

#include "nvim/garray.h"

/// Argument list: Array of file names.
/// Used for the global argument list and
/// the argument lists local to a window.
typedef struct arglist_s
{
    garray_st al_ga;  ///< growarray with the array of file names
    int al_refcount;  ///< number of windows using this arglist_st
    int id;           ///< id of this arglist_st
} arglist_st;

/// For each argument remember the file name as it was given, and the buffer
/// number that contains the expanded file name (required for when ":cd" is
/// used.
typedef struct argentry_s
{
    uchar_kt *ae_fname; ///< file name as specified
    int ae_fnum;        ///< buffer number with expanded file name
} aentry_st;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "argitem.h.generated.h"
#endif

#endif // NVIM_ARGITEM_H
