/// @file nvim/hardcopy.h

#ifndef NVIM_HARDCOPY_H
#define NVIM_HARDCOPY_H

#include <stdint.h>
#include <stdlib.h>

#include "nvim/types.h"
#include "nvim/ex_cmds_defs.h"

/// Structure to hold printing color and font attributes.
typedef struct
{
    uint32_t fg_color;
    uint32_t bg_color;
    int bold;
    int italic;
    int underline;
    int undercurl;
} prt_clrfnt_st;

/// Structure passed back to the generic printer code.
typedef struct
{
    int n_collated_copies;
    int n_uncollated_copies;
    int duplex;
    int chars_per_line;
    int lines_per_page;
    int has_color;
    prt_clrfnt_st number;
    int modec;
    int do_syntax;
    int user_abort;
    uchar_kt *jobname;
    uchar_kt *outfile;
    uchar_kt *arguments;
} prt_geninfo_st;

/// Generic option table item, only used for printer at the moment.
typedef struct
{
    const char *name;
    int hasnum;
    int number;
    uchar_kt *string;  ///< points into option string
    int strlen;
    int present;
} prt_opttable_st;

/// printer option index
enum prt_optidx_e
{
    kPrtOptTop           = 0,
    kPrtOptBottom        = 1,
    kPrtOptLeft          = 2,
    kPrtOptRight         = 3,
    kPrtOptHeaderHight   = 4,
    kPrtOptSyntax        = 5,
    kPrtOptNumber        = 6,
    kPrtOptWrap          = 7,
    kPrtOptDuplex        = 8,
    kPrtOptPortrait      = 9,
    kPrtOptPaper         = 10,
    kPrtOptCollate       = 11,
    kPrtOptJobSplit      = 12,
    kPrtOptFormfeed      = 13,
    kPrtOptNumOptions    = 14,
};

// For prt_get_unit().
#define PRT_UNIT_NONE       -1
#define PRT_UNIT_PERC       0
#define PRT_UNIT_INCH       1
#define PRT_UNIT_MM         2
#define PRT_UNIT_POINT      3
#define PRINT_NUMBER_WIDTH  8

#define PRT_UNIT_NAMES      {"pc", "in", "mm", "pt"}

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "hardcopy.h.generated.h"
#endif

#endif // NVIM_HARDCOPY_H
