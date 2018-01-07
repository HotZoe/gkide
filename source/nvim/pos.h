/// @file nvim/pos.h

#ifndef NVIM_POS_H
#define NVIM_POS_H

/// maximum (invalid) line number
#define MAXLNUM   0x7fffffff

/// maximum column number, 31 bits
#define MAXCOL    0x7fffffff

#define INIT_POS_T(l, c, ca) {l, c, ca}

typedef long linenum_kt; /// line number
typedef int colnr_T;  /// Column number

/// Format used to print values which have linenum_kt type
#define PRIdLINENR  "ld"
/// Format used to print values which have colnr_T type
#define PRIdCOLNR   "d"

/// position in file or buffer
typedef struct
{
    linenum_kt lnum;   ///< line number
    colnr_T col;     ///< column number
    colnr_T coladd;
} pos_T;


/// Same, but without coladd.
typedef struct
{
    linenum_kt lnum; ///< line number
    colnr_T col;   ///< column number
} lpos_T;

#endif // NVIM_POS_H
