/// @file nvim/hardcopy.h

#ifndef NVIM_HARDCOPY_H
#define NVIM_HARDCOPY_H

#include <stdint.h>
#include <stdlib.h>

#include "nvim/types.h"
#include "nvim/ex_cmds_defs.h"

/// Structure to hold printing color and font attributes.
typedef struct prt_clrfnt_s
{
    uint32_t fg_color;
    uint32_t bg_color;
    int bold;
    int italic;
    int underline;
    int undercurl;
} prt_clrfnt_st;

/// Structure passed back to the generic printer code.
typedef struct prt_geninfo_s
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
typedef struct prt_opttable_s
{
    const char *name;
    int hasnum;
    int number;
    uchar_kt *string;  ///< points into option string
    int strlen;
    int present;
} prt_opttable_st;

/// printer option index
enum prt_flgoption_e
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

/// print unit names, also see: prt_flgunit_e
#define PRT_UNIT_NAMES  { "pc", "in", "mm", "pt" }

/// For prt_get_unit().
enum prt_flgunit_e
{
    kPrtUnitNone    = -1,
    kPrtUnitPerc    = 0,  ///< print unit name: pc
    kPrtUnitInch    = 1,  ///< print unit name: in
    kPrtUnitMM      = 2,  ///< print unit name: mm
    kPrtUnitPoint   = 3,  ///< print unit name: pt
};

#define PRINT_NUMBER_WIDTH  8

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "hardcopy.h.generated.h"
#endif

#endif // NVIM_HARDCOPY_H
