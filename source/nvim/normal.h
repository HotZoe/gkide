/// @file nvim/normal.h

#ifndef NVIM_NORMAL_H
#define NVIM_NORMAL_H

#include <stdbool.h>
#include "nvim/pos.h"
#include "nvim/buffer_defs.h"

/// flag values for find_ident_under_cursor()
enum find_flg_e
{
    kFindFlgIdent  = 1, ///< find identifier (word)
    kFindFlgString = 2, ///< find any string (word)
    kFindFlgEval   = 4  ///< include "->", "[]" and "."
};

/// Motion types, used for operators and for yank/delete registers.
///
/// The three valid numerical values must not be changed, as they
/// are used in external communication and serialization.
typedef enum motion_type_e
{
    kMTUnknown   = -1, ///< Unknown or invalid motion type
    kMTCharWise  = 0,  ///< character-wise movement/register
    kMTLineWise  = 1,  ///< line-wise movement/register
    kMTBlockWise = 2   ///< block-wise movement/register
} motion_type_et;

/// Arguments for operators.
typedef struct oparg_s
{
    int op_type;                ///< current pending operator type
    int regname;                ///< register to use for the operator
    motion_type_et motion_type; ///< type of the current cursor motion
    int motion_force;    ///< force motion type: 'v', 'V' or CTRL-V
    bool use_reg_one;    ///< true if delete uses reg 1 even when not linewise
    bool inclusive;      ///< true if char motion is inclusive (only
                         ///< valid when motion_type is kMTCharWise)
    bool end_adjusted;   ///< backuped b_op_end one char
                         ///< (only used by do_format())
    apos_st start;       ///< start of the operator
    apos_st end;         ///< end of the operator
    apos_st cursor_start;///< cursor position before motion for "gw"

    long line_count; ///< number of lines from op_start to op_end (inclusive)
    bool empty;      ///< op_start and op_end the same (only used by op_change())
    bool is_VIsual;  ///< operator on Visual area
    columnum_kt start_vcol;///< start col for block mode operator
    columnum_kt end_vcol;  ///< end col for block mode operator
    long prev_opcount;     ///< ca.opcount saved for K_EVENT
    long prev_count0;      ///< ca.count0 saved for K_EVENT
} oparg_st;

/// Arguments for Normal mode commands.
typedef struct cmdarg_s
{
    oparg_st *oap;       ///< operator arguments
    int prechar;         ///< prefix character (optional, always 'g')
    int cmdchar;         ///< command character
    int nchar;           ///< next command character (optional)
    int ncharC1;         ///< first composing character (optional)
    int ncharC2;         ///< second composing character (optional)
    int extra_char;      ///< yet another character (optional)
    long opcount;        ///< count before an operator
    long count0;         ///< count before command, default 0
    long count1;         ///< count before command, default 1
    int arg;             ///< extra argument from nv_cmds[]
    int retval;          ///< return: CA_* values
    uchar_kt *searchbuf; ///< return: pointer to search pattern or NULL
} cmdarg_st;

// values for retval:
#define CA_COMMAND_BUSY   1 ///< skip restarting edit() once
#define CA_NO_ADJ_OP_END  2 ///< don't adjust operator end

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "normal.h.generated.h"
#endif

#endif // NVIM_NORMAL_H
