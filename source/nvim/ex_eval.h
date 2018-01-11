/// @file nvim/ex_eval.h

#ifndef NVIM_EX_EVAL_H
#define NVIM_EX_EVAL_H

#include "nvim/pos.h"
#include "nvim/ex_cmds_defs.h"

/// A list used for saving values of emsg_silent.
/// Used by ex_try() to save the value of emsg_silent if it
/// was non-zero. When this is done, the kCSNflgSilent flag below is set.
typedef struct errmsg_elem_s  errmsg_elem_st;
struct errmsg_elem_s
{
    int saved_emsg_silent; ///< saved value of "emsg_silent"
    errmsg_elem_st *next;  ///< next element on the list
};

/// For conditional commands a stack is kept of nested conditionals.
/// When cs_idx < 0, there is no conditional command.
#define CSTACK_LEN      50

/// There is no CSF_IF, the lack of kCSNflgWhile, kCSNflgFor and kCSNflgTry
/// means ":if" was used. Note that kCSNflgElse is only used when kCSNflgTry
/// and kCSNflgWhile are unset (an ":if"), and kCSNflgSilent is only used
/// when kCSNflgTry is set.
///
/// conditional stack flags for condstack_s::cs_flags
enum cs_normalflags_e
{
    kCSNflgTrue     = 0x0001,  ///< condition was TRUE
    kCSNflgActive   = 0x0002,  ///< current state is active
    kCSNflgElse     = 0x0004,  ///< ":else" has been passed
    kCSNflgWhile    = 0x0008,  ///< is a ":while"
    kCSNflgFor      = 0x0010,  ///< is a ":for"
    kCSNflgTry      = 0x0100,  ///< is a ":try"
    kCSNflgFinally  = 0x0200,  ///< ":finally" has been passed
    kCSNflgThrown   = 0x0400,  ///< exception thrown to this try conditional
    kCSNflgCaught   = 0x0800,  ///< exception caught by this try conditional
    kCSNflgSilent   = 0x1000,  ///< "emsg_silent" reset by ":try"
};

/// What's pending for being reactivated at the
/// ":endtry" of this try conditional.
///
/// conditional stack flags for condstack_s::cs_pending
enum cs_tryflags_e
{
    kCSTflgNone      = 0,   ///< nothing pending in ":finally" clause
    kCSTflgError     = 1,   ///< an error is pending
    kCSTflgInterrupt = 2,   ///< an interrupt is pending
    kCSTflgThrow     = 4,   ///< a throw is pending
    kCSTflgBreak     = 8,   ///< ":break" is pending
    kCSTflgContinue  = 16,  ///< ":continue" is pending
    kCSTflgReturn    = 24,  ///< ":return" is pending
    kCSTflgFinish    = 32,  ///< ":finish" is pending
};

/// conditional stack flags for condstack_s::cs_lflags
enum cs_loopflags_e
{
    kCSLflgLoop     = 1,  ///< just found ":while" or ":for"
    kCSLflgEndloop  = 2,  ///< just found ":endwhile" or ":endfor"
    kCSLflgContinue = 4,  ///< just found ":continue"
    kCSLflgFinally  = 8,  ///< just found ":finally"
};

struct condstack_s
{
    /// normal flags: cs_normalflags_e
    int cs_flags[CSTACK_LEN];
    /// what's pending in ":finally": cs_tryflags_e
    char cs_pending[CSTACK_LEN];

    union
    {
        void *csp_rv[CSTACK_LEN]; ///< return typeval for pending return
        void *csp_ex[CSTACK_LEN]; ///< exception for pending throw
    } cs_pend;

    void *cs_forinfo[CSTACK_LEN]; ///< info used by ":for"
    int cs_line[CSTACK_LEN];      ///< line nr of ":while"/":for" line
    int cs_idx;                   ///< current entry, or -1 if none
    int cs_looplevel;             ///< nr of nested ":while"s and ":for"s
    int cs_trylevel;              ///< nr of nested ":try"s
    errmsg_elem_st *cs_emsg_list; ///< saved values of emsg_silent
    int cs_lflags;                ///< loop flags: cs_loopflags_e
};

#define cs_rettv       cs_pend.csp_rv
#define cs_exception   cs_pend.csp_ex

/// A list of error messages that can be converted to an exception.
/// "throw_msg" is only set in the first element of the list. Usually,
/// it points to the original message stored in that element, but sometimes
/// it points to a later message in the list. See cause_errthrow() below.
typedef struct errmsg_list_s   errmsg_list_st;
struct errmsg_list_s
{
    uchar_kt *msg;        ///< original message
    uchar_kt *throw_msg;  ///< msg to throw: usually original one
    errmsg_list_st *next; ///< next of several messages in a row
};

/// Structure describing an exception.
typedef struct excmd_exception_s excmd_exception_st;
struct excmd_exception_s
{
    int type;                   ///< exception type
    uchar_kt *value;            ///< exception value
    errmsg_list_st *messages;   ///< message(s) causing error exception
    uchar_kt *throw_name;       ///< name of the throw point
    linenum_kt throw_lnum;      ///< line number of the throw point
    excmd_exception_st *caught; ///< next exception on the caught stack
};

// The exception types.
#define ET_USER         0      ///< exception caused by ":throw" command
#define ET_ERROR        1      ///< error exception
#define ET_INTERRUPT    2      ///< interrupt exception triggered by Ctrl-C

/// Structure to save the error/interrupt/exception state between calls to
/// enter_cleanup() and leave_cleanup(). Must be allocated as an automatic
/// variable by the (common) caller of these functions.
typedef struct excmd_cleanup_s excmd_cleanup_st;
struct excmd_cleanup_s
{
    int pending; ///< error/interrupt/exception state
    excmd_exception_st *exception; ///< exception value
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "ex_eval.h.generated.h"
#endif

#endif // NVIM_EX_EVAL_H
