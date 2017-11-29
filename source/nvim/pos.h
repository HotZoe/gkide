/// @file nvim/pos.h

#ifndef NVIM_POS_H
#define NVIM_POS_H

/// Format used to print values which have linenr_T type
#define PRIdLINENR "ld"

/// Format used to print values which have colnr_T type
#define PRIdCOLNR "d"

/// maximum (invalid) line number
#define MAXLNUM   0x7fffffff

/// maximum column number, 31 bits
#define MAXCOL    0x7fffffff

#define INIT_POS_T(l, c, ca) {l, c, ca}

/// line number type
typedef long linenr_T;

/// Column number type
typedef int colnr_T;

/// position in file or buffer
typedef struct
{
    linenr_T lnum;   ///< line number
    colnr_T col;     ///< column number
    colnr_T coladd;
} pos_T;


/// Same, but without coladd.
typedef struct
{
    linenr_T lnum; ///< line number
    colnr_T col;   ///< column number
} lpos_T;

#endif // NVIM_POS_H
