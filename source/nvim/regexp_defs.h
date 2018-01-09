/// @file nvim/regexp_defs.h

#ifndef NVIM_REGEXP_DEFS_H
#define NVIM_REGEXP_DEFS_H

#include <stdbool.h>

#include "nvim/pos.h"

/// The number of sub-matches is limited to 10.
/// The first one (index 0) is the whole match, referenced with "\0".
/// The second one (index 1) is the first sub-match, referenced with "\1".
/// This goes up to the tenth (index 9), referenced with "\9".
#define NSUBEXP    10

/// In the NFA engine: how many braces are allowed.
/// @todo Use dynamic memory allocation instead of static, like here
#define NFA_MAX_BRACES  20

// In the NFA engine: how many states are allowed.
#define NFA_MAX_STATES     100000
#define NFA_TOO_EXPENSIVE  -1

// Which regexp engine to use? Needed for vim_regcomp().
// Must match with 'regexpengine'.
#define AUTOMATIC_ENGINE     0
#define BACKTRACKING_ENGINE  1
#define NFA_ENGINE           2

typedef struct regengine regengine_st;

/// Structure returned by vim_regcomp() to pass on to vim_regexec().
/// This is the general structure. For the actual matcher, two specific
/// structures are used. See code below.
typedef struct regprog_s
{
    regengine_st *engine;
    unsigned regflags;
    unsigned re_engine;  ///< Automatic, backtracking or NFA engine.
    unsigned re_flags;   ///< Second argument for vim_regcomp().
} regprog_st;

/// Structure used by the back track matcher.
/// These fields are only to be used in regexp.c!
/// @see regexp.c for an explanation.
typedef struct
{
    // These four members implement regprog_st.
    regengine_st *engine;
    unsigned regflags;
    unsigned re_engine;
    unsigned re_flags;   ///< Second argument for vim_regcomp().

    int regstart;
    uchar_kt reganch;
    uchar_kt *regmust;
    int regmlen;
    uchar_kt reghasz;
    uchar_kt program[1]; ///< actually longer ...
} bt_regprog_st;

/// Structure representing a NFA state.
/// A NFA state may have no outgoing edge, when it is a NFA_MATCH state.
typedef struct nfa_state_s nfa_state_st;
struct nfa_state_s
{
    int c;
    nfa_state_st *out;
    nfa_state_st *out1;
    int id;
    int lastlist[2];   ///< 0: normal, 1: recursive
    int val;
};

/// Structure used by the NFA matcher.
typedef struct
{
    // These four members implement regprog_st.
    regengine_st *engine;
    unsigned regflags;
    unsigned re_engine;
    unsigned re_flags;    ///< Second argument for vim_regcomp().

    nfa_state_st *start;   ///< points into state[]

    int reganch;          ///< pattern starts with ^
    int regstart;         ///< char at start of pattern
    uchar_kt *match_text;   ///< plain text to match with

    int has_zend;         ///< pattern contains \ze
    int has_backref;      ///< pattern contains \1 .. \9
    int reghasz;
    uchar_kt *pattern;
    int nsubexp;          ///< number of ()
    int nstate;
    nfa_state_st state[1]; ///< actually longer ...
} nfa_regprog_st;

/// Structure to be used for single-line matching.
/// Sub-match "no" starts at "startp[no]" and ends just before "endp[no]".
/// When there is no match, the pointer is NULL.
typedef struct
{
    regprog_st *regprog;
    uchar_kt *startp[NSUBEXP];
    uchar_kt *endp[NSUBEXP];
    bool rm_ic;
} regmatch_st;

/// Structure to be used for multi-line matching.
/// Sub-match "no" starts in line "startpos[no].lnum"
/// column "startpos[no].col" and ends in line "endpos[no].lnum"
/// just before column "endpos[no].col". The line numbers are relative
/// to the first line, thus startpos[0].lnum is always 0. When there is
/// no match, the line number is -1.
typedef struct
{
    regprog_st *regprog;
    bpos_st startpos[NSUBEXP];
    bpos_st endpos[NSUBEXP];
    int rmm_ic;
    columnum_kt rmm_maxcol;        ///< when not zero: maximum column
} regmmatch_st;

/// Structure used to store external references: "\z\(\)" to "\z\1".
/// Use a reference count to avoid the need to copy this around.
/// When it goes from 1 to zero the matches need to be freed.
typedef struct
{
    short refcnt;
    uchar_kt *matches[NSUBEXP];
} reg_extmatch_st;

struct regengine
{
    regprog_st *(*regcomp)(uchar_kt *, int);
    void (*regfree)(regprog_st *);
    int (*regexec_nl)(regmatch_st *, uchar_kt *, columnum_kt, bool);

    long (*regexec_multi)(regmmatch_st *,
                          win_st *,
                          fbuf_st *,
                          linenum_kt,
                          columnum_kt,
                          proftime_kt *);
    uchar_kt *expr;
};

#endif // NVIM_REGEXP_DEFS_H
