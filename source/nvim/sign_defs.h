/// @file nvim/sign_defs.h

#ifndef NVIM_SIGN_DEFS_H
#define NVIM_SIGN_DEFS_H

#include "nvim/pos.h"

/// signs: line annotations
typedef struct signlist_s signlist_st;
struct signlist_s
{
    int id;            ///< unique identifier for each placed sign
    linenum_kt lnum;   ///< line number which has this sign
    int typenr;        ///< typenr of sign
    signlist_st *next; ///< next signlist entry
};

// type argument for buf_getsigntype()
#define SIGN_ANY     0
#define SIGN_LINEHL  1
#define SIGN_ICON    2
#define SIGN_TEXT    3

#endif // NVIM_SIGN_DEFS_H
