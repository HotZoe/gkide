/// @file nvim/pos.h

#ifndef NVIM_POS_H
#define NVIM_POS_H

/// maximum (invalid) line number
#define MAXLNUM   0x7fffffff

/// maximum column number, 31 bits
#define MAXCOL    0x7fffffff

#define INIT_POS_T(l, c, ca) { l, c, ca }

typedef long linenum_kt; /// line number
typedef int columnum_kt; /// Column number

/// Format used to print values which have linenum_kt type
#define LineNumKtPrtFmt  "ld"
/// Format used to print values which have columnum_kt type
#define ColumNumKtPrtFmt  "d"

/// position in file or buffer
typedef struct
{
    linenum_kt lnum;    ///< line number
    columnum_kt col;    ///< column number
    columnum_kt coladd; ///<
} apos_st;

/// Same, but without coladd.
typedef struct
{
    linenum_kt lnum; ///< line number
    columnum_kt col; ///< column number
} bpos_st;

#endif // NVIM_POS_H
