/// @file nvim/eval.c
///
/// Expression evaluation.

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <msgpack.h>

#include "nvim/assert.h"
#include "nvim/nvim.h"
#include "nvim/ascii.h"
#ifdef HAVE_HDR_LOCALE_H
    #include <locale.h>
#endif
#include "nvim/eval.h"
#include "nvim/buffer.h"
#include "nvim/charset.h"
#include "nvim/cursor.h"
#include "nvim/diff.h"
#include "nvim/edit.h"
#include "nvim/ex_cmds.h"
#include "nvim/ex_cmds2.h"
#include "nvim/ex_docmd.h"
#include "nvim/ex_eval.h"
#include "nvim/ex_getln.h"
#include "nvim/fileio.h"
#include "nvim/os/fileio.h"
#include "nvim/func_attr.h"
#include "nvim/fold.h"
#include "nvim/getchar.h"
#include "nvim/hashtab.h"
#include "nvim/iconv.h"
#include "nvim/if_cscope.h"
#include "nvim/indent_c.h"
#include "nvim/indent.h"
#include "nvim/mark.h"
#include "nvim/mbyte.h"
#include "nvim/memline.h"
#include "nvim/memory.h"
#include "nvim/message.h"
#include "nvim/misc1.h"
#include "nvim/keymap.h"
#include "nvim/map.h"
#include "nvim/file_search.h"
#include "nvim/garray.h"
#include "nvim/move.h"
#include "nvim/normal.h"
#include "nvim/ops.h"
#include "nvim/option.h"
#include "nvim/os_unix.h"
#include "nvim/path.h"
#include "nvim/popupmnu.h"
#include "nvim/profile.h"
#include "nvim/quickfix.h"
#include "nvim/regexp.h"
#include "nvim/screen.h"
#include "nvim/search.h"
#include "nvim/sha256.h"
#include "nvim/spell.h"
#include "nvim/state.h"
#include "nvim/strings.h"
#include "nvim/syntax.h"
#include "nvim/tag.h"
#include "nvim/ui.h"
#include "nvim/main.h"
#include "nvim/mouse.h"
#include "nvim/terminal.h"
#include "nvim/undo.h"
#include "nvim/version.h"
#include "nvim/window.h"
#include "nvim/eval/encode.h"
#include "nvim/eval/decode.h"
#include "nvim/os/os.h"
#include "nvim/event/libuv_process.h"
#include "nvim/os/pty_process.h"
#include "nvim/event/rstream.h"
#include "nvim/event/wstream.h"
#include "nvim/event/time.h"
#include "nvim/os/time.h"
#include "nvim/msgpack/channel.h"
#include "nvim/msgpack/server.h"
#include "nvim/msgpack/helpers.h"
#include "nvim/api/private/helpers.h"
#include "nvim/api/nvim.h"
#include "nvim/os/dl.h"
#include "nvim/os/input.h"
#include "nvim/event/loop.h"
#include "nvim/lib/kvec.h"
#include "nvim/lib/khash.h"
#include "nvim/lib/queue.h"
#include "nvim/lua/executor.h"
#include "nvim/eval/typval.h"
#include "nvim/eval/executor.h"
#include "nvim/eval/gc.h"
#include "nvim/macros.h"
#include "nvim/utils.h"

#include "generated/config/config.h"

#if (defined(HOST_OS_WINDOWS) && !defined(FOUND_WORKING_LIBINTL))
// for separate library: libintl.a
extern char *ngettext(const char *__msgid1,
                      const char *__msgid2,
                      unsigned long int __n);
#endif

/// @todo: Remove DICT_MAXNEST, make users be non-recursive instead

/// maximum nesting of lists and dicts
#define DICT_MAXNEST    100

/// refcount for dict or list that should not be freed.
#define DO_NOT_FREE_CNT 99999

/// Character used as separator in autoload function/variable names.
#define AUTOLOAD_CHAR   '#'

/// Structure returned by get_lval() and used by set_var_lval().
///
/// - For a plain name:
///   - @b name         points to the variable name.
///   - @b exp_name     is NULL
///   - @b tv           is NULL
/// - For a magic braces name:
///   - @b name         points to the expanded variable name.
///   - @b exp_name     is non-NULL, to be freed later.
///   - @b tv           is NULL
/// - For an index in a list:
///   - @b name         points to the (expanded) variable name.
///   - @b exp_name     NULL or non-NULL, to be freed later.
///   - @b tv           points to the (first) list item value
///   - @b li           points to the (first) list item
///   - @b range, @b n1, @b n2 and @b empty2 indicate what items are used.
/// - For an existing Dict item:
///   - @b name         points to the (expanded) variable name.
///   - @b exp_name     NULL or non-NULL, to be freed later.
///   - @b tv           points to the dict item value
///   - @b newkey      is NULL
/// - For a non-existing Dict item:
///   - @b name         points to the (expanded) variable name.
///   - @b exp_name     NULL or non-NULL, to be freed later.
///   - @b tv           points to the Dictionary typval_st
///   - @b newkey       is the key for the new item.
typedef struct leftval_s
{
    const char *ll_name; ///< Start of variable name (can be NULL).
    size_t ll_name_len;  ///< Length of the .ll_name.
    char *ll_exp_name;   ///< NULL or expanded name in allocated memory.
    typval_st *ll_tv;    ///< Typeval of item being used. If "newkey" isn't
                         ///< NULL it's the Dict to which to add the item.
    listitem_st *ll_li;  ///< The list item or NULL.
    list_st *ll_list;    ///< The list or NULL.
    int ll_range;        ///< TRUE when a [i:j] range was used.
    long ll_n1;          ///< First index for list.
    long ll_n2;          ///< Second index for list range.
    int ll_empty2;       ///< Second index is empty: [i:].
    dict_st *ll_dict;    ///< The Dictionary or NULL.
    dictitem_st *ll_di;  ///< The dictitem or NULL.
    uchar_kt *ll_newkey; ///< New key for Dict in allocated memory or NULL.
} leftval_st;

static char *e_letunexp = N_("E18: Unexpected characters in :let");
static char *e_missbrac = N_("E111: Missing ']'");
static char *e_listarg = N_("E686: Argument of %s must be a List");
static char *e_listdictarg = N_("E712: Argument of %s must be a List or Dictionary");
static char *e_listreq = N_("E714: List required");
static char *e_dictreq = N_("E715: Dictionary required");
static char *e_stringreq = N_("E928: String required");
static char *e_toomanyarg = N_("E118: Too many arguments for function: %s");
static char *e_dictkey = N_("E716: Key not present in Dictionary: %s");
static char *e_funcexts = N_("E122: Function %s already exists, add ! to replace it");
static char *e_funcdict = N_("E717: Dictionary entry already exists");
static char *e_funcref = N_("E718: Funcref required");
static char *e_dictrange = N_("E719: Cannot use [:] with a Dictionary");
static char *e_nofunc = N_("E130: Unknown function: %s");
static char *e_illvar = N_("E461: Illegal variable name: %s");
static const char *e_readonlyvar = N_("E46: Cannot change read-only variable \"%.*s\"");

/// @todo: move to eval/executor
static char *e_letwrong = N_("E734: Wrong variable type for %s=");

static uchar_kt *const namespace_char = (uchar_kt *)"abglstvw";

/// Variable used for g:
static scope_dict_st globvars_var;

/// g: value
#define globvarht globvardict.dv_hashtab

/// Old Vim variables such as @b v:version are also available without
/// the @b v: also in functions. We need a special hashtable for them.
static hashtable_st compat_hashtab;

hashtable_st func_hashtab;

/// Used for checking if local variables or arguments used in a lambda.
static int *eval_lavars_used = NULL;

/// Array to hold the hashtab with variables local to each sourced script.
/// Each item holds a variable (nameless) that points to the dict_st.
typedef struct scriptvar_s
{
    scope_dict_st sv_var;
    dict_st sv_dict;
} scriptvar_st;

static garray_st ga_scripts = {0, 0, sizeof(scriptvar_st *), 4, NULL};

#define SCRIPT_SV(id)   (((scriptvar_st **)ga_scripts.ga_data)[(id) - 1])
#define SCRIPT_VARS(id) (SCRIPT_SV(id)->sv_dict.dv_hashtab)

/// attributes used for @b echo
static int echo_attr = 0;

/// trans_function_name() flags
typedef enum
{
    TFN_INT = 1,          ///< May use internal function name
    TFN_QUIET = 2,        ///< Do not emit error messages.
    TFN_NO_AUTOLOAD = 4,  ///< Do not use script autoloading.
    TFN_NO_DEREF = 8,     ///< Do not dereference a Funcref.
    TFN_READ_ONLY = 16,   ///< Will not change the variable.
} TransFunctionNameFlags;

/// get_lval() flags
typedef enum
{
    /// Do not emit error messages.
    GLV_QUIET = TFN_QUIET,

    /// Do not use script autoloading.
    GLV_NO_AUTOLOAD = TFN_NO_AUTOLOAD,

    /// Indicates that caller will not change
    /// the value (prevents error message).
    GLV_READ_ONLY = TFN_READ_ONLY,
} GetLvalFlags;

// function flags
#define FC_ABORT    0x01    ///< abort function on error
#define FC_RANGE    0x02    ///< function accepts range
#define FC_DICT     0x04    ///< Dict function, uses "self"
#define FC_CLOSURE  0x08    ///< closure, uses outer scope variables
#define FC_DELETED  0x10    ///< :delfunction used while uf_refcount > 0
#define FC_REMOVED  0x20    ///< function redefined while uf_refcount > 0

/// The names of packages that once were loaded are remembered.
static garray_st ga_loaded = { 0, 0, sizeof(uchar_kt *), 4, NULL };

#define FUNCARG(fp, j)  ((uchar_kt **)(fp->uf_args.ga_data))[j]
#define FUNCLINE(fp, j) ((uchar_kt **)(fp->uf_lines.ga_data))[j]

/// Short variable name length
#define VAR_SHORT_LEN  20

/// Number of fixed variables used for arguments
#define FIXVAR_CNT     12

/// Structure to hold info for a function that is
/// currently being executed, also @see func_call_st
struct func_call_s
{
    ufunc_st *func;  ///< Function being called.
    int linenr;      ///< Next line to be executed.
    int returned;    ///< @b :return used.

    /// Fixed variables for arguments.
    TV_DICTITEM_STRUCT(VAR_SHORT_LEN + 1) fixvar[FIXVAR_CNT];

    dict_st l_vars;                        ///< @b l: local function variables.
    scope_dict_st l_vars_var;              ///< Variable for @b l: scope.
    dict_st l_avars;                       ///< @b a: argument variables.
    scope_dict_st l_avars_var;             ///< Variable for @b a: scope.
    list_st l_varlist;                     ///< List for @b a:000
    listitem_st l_listitems[MAX_FUNC_ARGS];///< List items for a:000.

    typval_st *rettv;      ///< Return value.
    linenum_kt breakpoint; ///< Next line with breakpoint or zero.
    int dbg_tick;          ///< Debug_tick when breakpoint was set.
    int level;             ///< Top nesting level of executed function.
    proftime_kt prof_child;///< Time spent in a child.
    func_call_st *caller;  ///< Calling function or NULL.
    int fc_refcount;       ///< Number of user functions that reference this funccall.
    int fc_copyID;         ///< CopyID used for garbage collection.
    garray_st fc_funcs;    ///< List of ufunc_st* which keep a reference to @b func
};

/// Structure used by trans_function_name()
typedef struct funcdict_s
{
    dict_st *fd_dict;    ///< Dictionary used.
    uchar_kt *fd_newkey; ///< New key in "dict" in allocated memory.
    dictitem_st *fd_di;  ///< Dictionary item used.
} funcdict_st;

/// Info used by a @b :for loop.
typedef struct forinfo_s
{
    int fi_semicolon;      ///< TRUE if ending in '; var]'
    int fi_varcount;       ///< nr of variables in the list
    list_watcher_st fi_lw; ///< keep an eye on the item used.
    list_st *fi_list;      ///< list being used
} forinfo_st;

/// enum used by var_flavour()
typedef enum var_flavour_e
{
    VAR_FLAVOUR_DEFAULT,   ///< doesn't start with uppercase
    VAR_FLAVOUR_SESSION,   ///< starts with uppercase, some lower
    VAR_FLAVOUR_SHADA      ///< all uppercase
} var_flavour_et;

/// @addtogroup NVIM_VAR_FLAGS v:XXX Variables Flags(vimvar_st)
/// @{

/// for vimvar_st::vv_flags, compatible, also used without 'v:'
#define VV_COMPAT       0x01

/// for vimvar_st::vv_flags, read-only
#define VV_RO           0x02

/// for vimvar_st::vv_flags, read-only in the sandbox
#define VV_RO_SBX       0x04

/// for init ::vimvars only
#define VV(idx, name, type, flags)       \
    [idx] = {                            \
        .vv_name = name,                 \
        .vv_di = {                       \
            .di_tv = { .v_type = type }, \
            .di_flags = 0,               \
            .di_key = { 0 },             \
        },                               \
        .vv_flags = flags,               \
    }

typedef struct vimvar_s
{
    /// Name of the variable, without @b v:
    /// the length is <= 16
    char *vv_name;
    struct
    {
        typval_st di_tv;     ///< scope dictionary
        uint8_t di_flags;    ///< Flags.
        uchar_kt di_key[17]; ///< Key value.
    } vv_di;                 ///< Value and name for key (max 16 chars)
    char vv_flags;           ///< Flags: #VV_COMPAT, #VV_RO, #VV_RO_SBX.
} vimvar_st;

/// Array to hold the value of @b v: variables.
///
/// The value is in a dictitem, so that it can
/// also be used in the @b v: scope. The reason
/// to use this table anyway is for very quick
/// access to the variables with the @b VV_* defines.
static vimvar_st vimvars[] =
{
    // VV_ tails differing from upcased string literals:
    // VV_REG           "register"
    // VV_OP            "operator"
    // VV_SEND_SERVER   "servername"
    // VV_CC_TO         "charconvert_to"
    // VV_CC_FROM       "charconvert_from"
    //
    // index              name                type        flags
    //
    VV(VV_COUNT,          "count",            kNvarNumber,  VV_COMPAT+VV_RO),
    VV(VV_COUNT1,         "count1",           kNvarNumber,  VV_RO),
    VV(VV_PREVCOUNT,      "prevcount",        kNvarNumber,  VV_RO),
    VV(VV_ERRMSG,         "errmsg",           kNvarString,  VV_COMPAT),
    VV(VV_WARNINGMSG,     "warningmsg",       kNvarString,  0),
    VV(VV_STATUSMSG,      "statusmsg",        kNvarString,  0),
    VV(VV_SHELL_ERROR,    "shell_error",      kNvarNumber,  VV_COMPAT+VV_RO),
    VV(VV_THIS_SESSION,   "this_session",     kNvarString,  VV_COMPAT),
    VV(VV_VERSION,        "version",          kNvarNumber,  VV_COMPAT+VV_RO),
    VV(VV_LNUM,           "lnum",             kNvarNumber,  VV_RO_SBX),
    VV(VV_TERMRESPONSE,   "termresponse",     kNvarString,  VV_RO),
    VV(VV_FNAME,          "fname",            kNvarString,  VV_RO),
    VV(VV_LANG,           "lang",             kNvarString,  VV_RO),
    VV(VV_LC_TIME,        "lc_time",          kNvarString,  VV_RO),
    VV(VV_CTYPE,          "ctype",            kNvarString,  VV_RO),
    VV(VV_CC_FROM,        "charconvert_from", kNvarString,  VV_RO),
    VV(VV_CC_TO,          "charconvert_to",   kNvarString,  VV_RO),
    VV(VV_FNAME_IN,       "fname_in",         kNvarString,  VV_RO),
    VV(VV_FNAME_OUT,      "fname_out",        kNvarString,  VV_RO),
    VV(VV_FNAME_NEW,      "fname_new",        kNvarString,  VV_RO),
    VV(VV_FNAME_DIFF,     "fname_diff",       kNvarString,  VV_RO),
    VV(VV_CMDARG,         "cmdarg",           kNvarString,  VV_RO),
    VV(VV_FOLDSTART,      "foldstart",        kNvarNumber,  VV_RO_SBX),
    VV(VV_FOLDEND,        "foldend",          kNvarNumber,  VV_RO_SBX),
    VV(VV_FOLDDASHES,     "folddashes",       kNvarString,  VV_RO_SBX),
    VV(VV_FOLDLEVEL,      "foldlevel",        kNvarNumber,  VV_RO_SBX),
    VV(VV_PROGNAME,       "progname",         kNvarString,  VV_RO),
    VV(VV_SEND_SERVER,    "servername",       kNvarString,  VV_RO),
    VV(VV_DYING,          "dying",            kNvarNumber,  VV_RO),
    VV(VV_EXCEPTION,      "exception",        kNvarString,  VV_RO),
    VV(VV_THROWPOINT,     "throwpoint",       kNvarString,  VV_RO),
    VV(VV_REG,            "register",         kNvarString,  VV_RO),
    VV(VV_CMDBANG,        "cmdbang",          kNvarNumber,  VV_RO),
    VV(VV_INSERTMODE,     "insertmode",       kNvarString,  VV_RO),
    VV(VV_VAL,            "val",              kNvarUnknown, VV_RO),
    VV(VV_KEY,            "key",              kNvarUnknown, VV_RO),
    VV(VV_PROFILING,      "profiling",        kNvarNumber,  VV_RO),
    VV(VV_FCS_REASON,     "fcs_reason",       kNvarString,  VV_RO),
    VV(VV_FCS_CHOICE,     "fcs_choice",       kNvarString,  0),
    VV(VV_BEVAL_BUFNR,    "beval_bufnr",      kNvarNumber,  VV_RO),
    VV(VV_BEVAL_WINNR,    "beval_winnr",      kNvarNumber,  VV_RO),
    VV(VV_BEVAL_WINID,    "beval_winid",      kNvarNumber,  VV_RO),
    VV(VV_BEVAL_LNUM,     "beval_lnum",       kNvarNumber,  VV_RO),
    VV(VV_BEVAL_COL,      "beval_col",        kNvarNumber,  VV_RO),
    VV(VV_BEVAL_TEXT,     "beval_text",       kNvarString,  VV_RO),
    VV(VV_SCROLLSTART,    "scrollstart",      kNvarString,  0),
    VV(VV_SWAPNAME,       "swapname",         kNvarString,  VV_RO),
    VV(VV_SWAPCHOICE,     "swapchoice",       kNvarString,  0),
    VV(VV_SWAPCOMMAND,    "swapcommand",      kNvarString,  VV_RO),
    VV(VV_CHAR,           "char",             kNvarString,  0),
    VV(VV_MOUSE_WIN,      "mouse_win",        kNvarNumber,  0),
    VV(VV_MOUSE_WINID,    "mouse_winid",      kNvarNumber,  0),
    VV(VV_MOUSE_LNUM,     "mouse_lnum",       kNvarNumber,  0),
    VV(VV_MOUSE_COL,      "mouse_col",        kNvarNumber,  0),
    VV(VV_OP,             "operator",         kNvarString,  VV_RO),
    VV(VV_SEARCHFORWARD,  "searchforward",    kNvarNumber,  0),
    VV(VV_HLSEARCH,       "hlsearch",         kNvarNumber,  0),
    VV(VV_OLDFILES,       "oldfiles",         kNvarList,    0),
    VV(VV_WINDOWID,       "windowid",         kNvarNumber,  VV_RO_SBX),
    VV(VV_PROGPATH,       "progpath",         kNvarString,  VV_RO),
    VV(VV_COMMAND_OUTPUT, "command_output",   kNvarString,  0),
    VV(VV_COMPLETED_ITEM, "completed_item",   kNvarDict,    VV_RO),
    VV(VV_OPTION_NEW,     "option_new",       kNvarString,  VV_RO),
    VV(VV_OPTION_OLD,     "option_old",       kNvarString,  VV_RO),
    VV(VV_OPTION_TYPE,    "option_type",      kNvarString,  VV_RO),
    VV(VV_ERRORS,         "errors",           kNvarList,    0),
    VV(VV_MSGPACK_TYPES,  "msgpack_types",    kNvarDict,    VV_RO),
    VV(VV_EVENT,          "event",            kNvarDict,    VV_RO),
    VV(VV_FALSE,          "false",            kNvarSpecial, VV_RO),
    VV(VV_TRUE,           "true",             kNvarSpecial, VV_RO),
    VV(VV_NULL,           "null",             kNvarSpecial, VV_RO),
    VV(VV__NULL_LIST,     "_null_list",       kNvarList,    VV_RO),
    VV(VV__NULL_DICT,     "_null_dict",       kNvarDict,    VV_RO),
    VV(VV_VIM_DID_ENTER,  "vim_did_enter",    kNvarNumber,  VV_RO),
    VV(VV_TESTING,        "testing",          kNvarNumber,  0),
    VV(VV_TYPE_NUMBER,    "t_number",         kNvarNumber,  VV_RO),
    VV(VV_TYPE_STRING,    "t_string",         kNvarNumber,  VV_RO),
    VV(VV_TYPE_FUNC,      "t_func",           kNvarNumber,  VV_RO),
    VV(VV_TYPE_LIST,      "t_list",           kNvarNumber,  VV_RO),
    VV(VV_TYPE_DICT,      "t_dict",           kNvarNumber,  VV_RO),
    VV(VV_TYPE_FLOAT,     "t_float",          kNvarNumber,  VV_RO),
    VV(VV_TYPE_BOOL,      "t_bool",           kNvarNumber,  VV_RO),
    VV(VV_EXITING,        "exiting",          kNvarNumber,  VV_RO),
};
#undef VV
/// @}

// for shorthand
#define vv_type         vv_di.di_tv.v_type
#define vv_nr           vv_di.di_tv.vval.v_number
#define vv_special      vv_di.di_tv.vval.v_special
#define vv_float        vv_di.di_tv.vval.v_float
#define vv_str          vv_di.di_tv.vval.v_string
#define vv_list         vv_di.di_tv.vval.v_list
#define vv_dict         vv_di.di_tv.vval.v_dict
#define vv_tv           vv_di.di_tv
#define vimvarht        vimvardict.dv_hashtab

/// Variable used for @b v:
static scope_dict_st vimvars_var;

typedef struct terminal_jobdata_s
{
    union
    {
        libuv_process_st uv;
        pty_process_st pty;
    } proc;
    stream_st in;
    stream_st out;
    stream_st err;            ///< Initialized in common_job_start().
    terminal_st *term;
    bool stopped;
    bool exited;
    bool rpc;
    int refcount;
    callback_st on_stdout;
    callback_st on_stderr;
    callback_st on_exit;
    int *status_ptr;
    uint64_t id;
    multiqueue_st *events;
} terminal_jobdata_st;

typedef struct job_event_s
{
    terminal_jobdata_st *data;
    callback_st *callback;
    const char *type;
    list_st *received;
    int status;
} job_event_st;

typedef struct timer_s
{
    time_watcher_st tw;
    int timer_id;
    int repeat_count;
    int refcount;
    long timeout;
    bool stopped;
    bool paused;
    callback_st callback;
} timer_st;

typedef void (*func_ptr_ft)(void);

/// Prototype of C function that implements NvimL function
typedef void (*nvl_func_ft)(typval_st *args, typval_st *rvar, func_ptr_ft data);

/// Structure holding NvimL function definition
typedef struct nviml_func_s
{
    char *name;        ///< Name of the function.
    uint8_t min_argc;  ///< Minimal number of arguments.
    uint8_t max_argc;  ///< Maximal number of arguments.
    nvl_func_ft func;  ///< Function implementation.
    func_ptr_ft data;  ///< Userdata for function implementation.
} nviml_func_st;

KHASH_MAP_INIT_STR(functions, nviml_func_st)

/// Type of assert_* check being performed
typedef enum assert_type_e
{
    ASSERT_EQUAL,
    ASSERT_NOTEQUAL,
    ASSERT_MATCH,
    ASSERT_NOTMATCH,
    ASSERT_INRANGE,
    ASSERT_OTHER,
} assert_type_et;

/// Type for dict_list function
typedef enum dictlist_type_e
{
    kDictListKeys,   ///< List dictionary keys.
    kDictListValues, ///< List dictionary values.
    kDictListItems,  ///< List dictionary contents: [keys, values].
} dictlist_type_et;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "eval.c.generated.h"
#endif

/// find_name_end(): include [] in name
#define FNE_INCL_BR      1

/// find_name_end(): check name starts with valid character
#define FNE_CHECK_START  2

static PMap(uint64_t) *jobs = NULL;
static PMap(uint64_t) *timers = NULL;
static uint64_t last_timer_id = 0;

/// Dummy va_list for passing to xsnprintf
///
/// Used because:
/// - passing a NULL pointer doesn't work when va_list isn't a pointer
/// - locally in the function results in a "used before set" warning
/// - using va_start() to initialize it gives "function with fixed args" error
static va_list dummy_ap;

/// @see MessagePackType
static const char *const msgpack_type_names[] =
{
    [kMPNil] = "nil",
    [kMPBoolean] = "boolean",
    [kMPInteger] = "integer",
    [kMPFloat] = "float",
    [kMPString] = "string",
    [kMPBinary] = "binary",
    [kMPArray] = "array",
    [kMPMap] = "map",
    [kMPNone] = "none",
};

const list_st *eval_msgpack_type_lists[] =
{
    [kMPNil] = NULL,
    [kMPBoolean] = NULL,
    [kMPInteger] = NULL,
    [kMPFloat] = NULL,
    [kMPString] = NULL,
    [kMPBinary] = NULL,
    [kMPArray] = NULL,
    [kMPMap] = NULL,
    [kMPNone] = NULL,
};

/// Initialize the global and @b v: variables.
void eval_init(void)
{
    vimvars[VV_VERSION].vv_nr = NVIM_VERSION_INT32;

    jobs = pmap_new(uint64_t)();
    timers = pmap_new(uint64_t)();

    vimvar_st *p;
    init_var_dict(&globvardict, &globvars_var, VAR_DEF_SCOPE);
    init_var_dict(&vimvardict, &vimvars_var, VAR_SCOPE);
    vimvardict.dv_lock = kNvlVarFixed;
    hash_init(&compat_hashtab);
    hash_init(&func_hashtab);

    for(size_t i = 0; i < ARRAY_SIZE(vimvars); i++)
    {
        p = &vimvars[i];

        assert(ustrlen(p->vv_name) <= 16);
        ustrcpy(p->vv_di.di_key, p->vv_name);

        if(p->vv_flags & VV_RO)
        {
            p->vv_di.di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
        }
        else if(p->vv_flags & VV_RO_SBX)
        {
            p->vv_di.di_flags = DI_FLAGS_RO_SBX | DI_FLAGS_FIX;
        }
        else
        {
            p->vv_di.di_flags = DI_FLAGS_FIX;
        }

        // add to v: scope dict, unless the value is not always available
        if(p->vv_type != kNvarUnknown)
        {
            hash_add(&vimvarht, p->vv_di.di_key);
        }

        // add to compat scope dict
        if(p->vv_flags & VV_COMPAT)
        {
            hash_add(&compat_hashtab, p->vv_di.di_key);
        }
    }

    dict_st *const msgpack_types_dict = tv_dict_alloc();

    for(size_t i = 0; i < ARRAY_SIZE(msgpack_type_names); i++)
    {
        list_st *const type_list = tv_list_alloc();
        type_list->lv_lock = kNvlVarFixed;
        type_list->lv_refcount = 1;

        dictitem_st *const di = tv_dict_item_alloc(msgpack_type_names[i]);
        di->di_flags |= DI_FLAGS_RO | DI_FLAGS_FIX;
        di->di_tv.v_type = kNvarList;
        di->di_tv.vval.v_list = type_list;

        eval_msgpack_type_lists[i] = type_list;

        // There must not be duplicate items in this dictionary by definition.
        if(tv_dict_add(msgpack_types_dict, di) == FAIL)
        {
            assert(false);
        }
    }

    msgpack_types_dict->dv_lock = kNvlVarFixed;
    set_vim_var_dict(VV_MSGPACK_TYPES, msgpack_types_dict);
    set_vim_var_dict(VV_COMPLETED_ITEM, tv_dict_alloc());

    dict_st *v_event = tv_dict_alloc();
    v_event->dv_lock = kNvlVarFixed;

    set_vim_var_dict(VV_EVENT, v_event);
    set_vim_var_list(VV_ERRORS, tv_list_alloc());

    set_vim_var_nr(VV_SEARCHFORWARD, 1L);
    set_vim_var_nr(VV_HLSEARCH, 1L);
    set_vim_var_nr(VV_COUNT1, 1);
    set_vim_var_nr(VV_TYPE_FUNC,    VAR_TYPE_FUNC);
    set_vim_var_nr(VV_TYPE_LIST,    VAR_TYPE_LIST);
    set_vim_var_nr(VV_TYPE_DICT,    VAR_TYPE_DICT);
    set_vim_var_nr(VV_TYPE_BOOL,    VAR_TYPE_BOOL);
    set_vim_var_nr(VV_TYPE_FLOAT,   VAR_TYPE_FLOAT);
    set_vim_var_nr(VV_TYPE_NUMBER,  VAR_TYPE_NUMBER);
    set_vim_var_nr(VV_TYPE_STRING,  VAR_TYPE_STRING);

    set_vim_var_special(VV_FALSE,   kSpecialVarFalse);
    set_vim_var_special(VV_TRUE,    kSpecialVarTrue);
    set_vim_var_special(VV_NULL,    kSpecialVarNull);
    set_vim_var_special(VV_EXITING, kSpecialVarNull);
    set_reg_var(0); // default for v:register is not 0 but '"'
}

#if defined(EXITFREE)
void eval_clear(void)
{
    vimvar_st *p;
    for(size_t i = 0; i < ARRAY_SIZE(vimvars); i++)
    {
        p = &vimvars[i];
        if(p->vv_di.di_tv.v_type == kNvarString)
        {
            xfree(p->vv_str);
            p->vv_str = NULL;
        }
        else if(p->vv_di.di_tv.v_type == kNvarList)
        {
            tv_list_unref(p->vv_list);
            p->vv_list = NULL;
        }
    }

    hash_clear(&vimvarht);
    hash_init(&vimvarht); // garbage_collect() will access it
    hash_clear(&compat_hashtab);
    free_scriptnames();
    free_locales();
    vars_clear(&globvarht); // global variables
    ga_clear_strings(&ga_loaded); // autoloaded script names

    // Script-local variables. First clear all the variables and in a second
    // loop free the scriptvar_st, because a variable in one script might hold
    // a reference to the whole scope of another script.
    for(int i = 1; i <= ga_scripts.ga_len; ++i)
    {
        vars_clear(&SCRIPT_VARS(i));
    }

    for(int i = 1; i <= ga_scripts.ga_len; ++i)
    {
        xfree(SCRIPT_SV(i));
    }

    ga_clear(&ga_scripts);
    (void)garbage_collect(false); // unreferenced lists and dicts
    free_all_functions();
}
#endif

/// Return the name of the executed function.
uchar_kt *func_name(void *cookie)
{
    return ((func_call_st *)cookie)->func->uf_name;
}

/// Return the address holding the next breakpoint line for a funccall cookie.
linenum_kt *func_breakpoint(void *cookie)
{
    return &((func_call_st *)cookie)->breakpoint;
}

/// Return the address holding the debug tick for a funccall cookie.
int *func_dbg_tick(void *cookie)
{
    return &((func_call_st *)cookie)->dbg_tick;
}

/// Return the nesting level for a funccall cookie.
int func_level(void *cookie)
{
    return ((func_call_st *)cookie)->level;
}

/// pointer to funccal for currently active function
func_call_st *current_funccal = NULL;

/// pointer to list of previously used funccal, still around because some
/// item in it is still being used.
func_call_st *previous_funccal = NULL;

/// Return TRUE when a function was ended by a ":return" command.
int current_func_returned(void)
{
    return current_funccal->returned;
}

/// Set an internal variable to a string value.
/// Creates the variable if it does not already exist.
void set_internal_string_var(uchar_kt *name, uchar_kt *value)
{
    const typval_st tv = {
        .v_type = kNvarString,
        .vval.v_string = value,
    };

    set_var((const char *)name, ustrlen(name), (typval_st *)&tv, true);
}

static leftval_st *redir_lval = NULL;
static garray_st redir_ga; // Only valid when redir_lval is not NULL.
static uchar_kt *redir_endp = NULL;
static uchar_kt *redir_varname = NULL;

/// Start recording command output to a variable
///
/// @param name
/// @param append   append to an existing variabl
///
/// Returns OK if successfully completed the setup. FAIL otherwise.
int var_redir_start(uchar_kt *name, int append)
{
    int save_emsg;
    int err;
    typval_st tv;

    // Catch a bad name early.
    if(!eval_isnamec1(*name))
    {
        EMSG(_(e_invarg));
        return FAIL;
    }

    // Make a copy of the name, it is used in redir_lval until redir ends.
    redir_varname = ustrdup(name);
    redir_lval = xcalloc(1, sizeof(leftval_st));

    // The output is stored in growarray "redir_ga" until redirection ends.
    ga_init(&redir_ga, (int)sizeof(char), 500);

    // Parse the variable name (can be a dict or list entry).
    redir_endp = (uchar_kt *)get_lval(redir_varname,
                                    NULL,
                                    redir_lval,
                                    false,
                                    false,
                                    0,
                                    FNE_CHECK_START);

    if(redir_endp == NULL || redir_lval->ll_name == NULL || *redir_endp != NUL)
    {
        clear_lval(redir_lval);

        if(redir_endp != NULL && *redir_endp != NUL)
        {
            // Trailing characters are present after the variable name
            EMSG(_(e_trailing));
        }
        else
        {
            EMSG(_(e_invarg));
        }

        redir_endp = NULL; // don't store a value, only cleanup
        var_redir_stop();
        return FAIL;
    }

    // check if we can write to the variable:
    // set it to or append an empty string
    save_emsg = did_emsg;
    did_emsg = FALSE;
    tv.v_type = kNvarString;
    tv.vval.v_string = (uchar_kt *)"";

    if(append)
    {
        set_var_lval(redir_lval, redir_endp, &tv, TRUE, (uchar_kt *)".");
    }
    else
    {
        set_var_lval(redir_lval, redir_endp, &tv, TRUE, (uchar_kt *)"=");
    }

    clear_lval(redir_lval);
    err = did_emsg;
    did_emsg |= save_emsg;

    if(err)
    {
        redir_endp = NULL; // don't store a value, only cleanup
        var_redir_stop();
        return FAIL;
    }

    return OK;
}

/// Append "value[value_len]" to the variable set by var_redir_start().
/// The actual appending is postponed until redirection ends, because the value
/// appended may in fact be the string we write to, changing it may cause freed
/// memory to be used:
/// - :redir => foo
/// - :let foo
/// - :redir END
void var_redir_str(uchar_kt *value, int value_len)
{
    int len;

    if(redir_lval == NULL)
    {
        return;
    }

    if(value_len == -1)
    {
        len = (int)ustrlen(value); // Append the entire string
    }
    else
    {
        len = value_len; // Append only "value_len" characters
    }

    ga_grow(&redir_ga, len);
    memmove((char *)redir_ga.ga_data + redir_ga.ga_len, value, len);
    redir_ga.ga_len += len;
}

/// Stop redirecting command output to a variable.
/// Frees the allocated memory.
void var_redir_stop(void)
{
    typval_st tv;

    if(redir_lval != NULL)
    {
        // If there was no error: assign the text to the variable.
        if(redir_endp != NULL)
        {
            ga_append(&redir_ga, NUL); // Append the trailing NUL.
            tv.v_type = kNvarString;
            tv.vval.v_string = redir_ga.ga_data;

            // Call get_lval() again, if it's inside a Dict or
            // List it may have changed.
            redir_endp = (uchar_kt *)get_lval(redir_varname,
                                            NULL,
                                            redir_lval,
                                            false,
                                            false,
                                            0,
                                            FNE_CHECK_START);

            if(redir_endp != NULL && redir_lval->ll_name != NULL)
            {
                set_var_lval(redir_lval, redir_endp,
                             &tv, false, (uchar_kt *)".");
            }

            clear_lval(redir_lval);
        }

        // free the collected output
        xfree(redir_ga.ga_data);
        redir_ga.ga_data = NULL;
        xfree(redir_lval);
        redir_lval = NULL;
    }

    xfree(redir_varname);
    redir_varname = NULL;
}

int eval_charconvert(const char *const enc_from,
                     const char *const enc_to,
                     const char *const fname_from,
                     const char *const fname_to)
{
    bool err = false;

    set_vim_var_string(VV_CC_FROM, enc_from, -1);
    set_vim_var_string(VV_CC_TO, enc_to, -1);
    set_vim_var_string(VV_FNAME_IN, fname_from, -1);
    set_vim_var_string(VV_FNAME_OUT, fname_to, -1);

    if(eval_to_bool(p_ccv, &err, NULL, false))
    {
        err = true;
    }

    set_vim_var_string(VV_CC_FROM, NULL, -1);
    set_vim_var_string(VV_CC_TO, NULL, -1);
    set_vim_var_string(VV_FNAME_IN, NULL, -1);
    set_vim_var_string(VV_FNAME_OUT, NULL, -1);

    if(err)
    {
        return FAIL;
    }

    return OK;
}

int eval_printexpr(const char *const fname, const char *const args)
{
    bool err = false;
    set_vim_var_string(VV_FNAME_IN, fname, -1);
    set_vim_var_string(VV_CMDARG, args, -1);

    if(eval_to_bool(p_pexpr, &err, NULL, false))
    {
        err = true;
    }

    set_vim_var_string(VV_FNAME_IN, NULL, -1);
    set_vim_var_string(VV_CMDARG, NULL, -1);

    if(err)
    {
        os_remove(fname);
        return FAIL;
    }

    return OK;
}

void eval_diff(const char *const origfile,
               const char *const newfile,
               const char *const outfile)
{
    bool err = false;

    set_vim_var_string(VV_FNAME_IN, origfile, -1);
    set_vim_var_string(VV_FNAME_NEW, newfile, -1);
    set_vim_var_string(VV_FNAME_OUT, outfile, -1);

    (void)eval_to_bool(p_dex, &err, NULL, FALSE);

    set_vim_var_string(VV_FNAME_IN, NULL, -1);
    set_vim_var_string(VV_FNAME_NEW, NULL, -1);
    set_vim_var_string(VV_FNAME_OUT, NULL, -1);
}

void eval_patch(const char *const origfile,
                const char *const difffile,
                const char *const outfile)
{
    bool err = false;

    set_vim_var_string(VV_FNAME_IN, origfile, -1);
    set_vim_var_string(VV_FNAME_DIFF, difffile, -1);
    set_vim_var_string(VV_FNAME_OUT, outfile, -1);

    (void)eval_to_bool(p_pex, &err, NULL, FALSE);

    set_vim_var_string(VV_FNAME_IN, NULL, -1);
    set_vim_var_string(VV_FNAME_DIFF, NULL, -1);
    set_vim_var_string(VV_FNAME_OUT, NULL, -1);
}

/// Top level evaluation function, returning a boolean.
/// Sets @b error to TRUE if there was an error.
///
/// @param arg
/// @param error
/// @param nextcmd
/// @param skid     only parse, don't execute
///
/// Return TRUE or FALSE.
int eval_to_bool(uchar_kt *arg, bool *error, uchar_kt **nextcmd, int skip)
{
    typval_st tv;
    bool retval = false;

    if(skip)
    {
        emsg_skip++;
    }

    if(eval_lev_0(arg, &tv, nextcmd, !skip) == FAIL)
    {
        *error = true;
    }
    else
    {
        *error = false;

        if(!skip)
        {
            retval = (tv_get_number_chk(&tv, error) != 0);
            tv_clear(&tv);
        }
    }

    if(skip)
    {
        emsg_skip--;
    }

    return retval;
}

/// Top level evaluation function, returning a string
///
/// @param[in]  arg
/// String to evaluate.
///
/// @param  nextcmd
/// Pointer to the start of the next Ex command.
///
/// @param[in]  skip
/// If true, only do parsing to nextcmd without
/// reporting errors or actually evaluating anything.
///
/// @return [allocated]
/// string result of evaluation or NULL in case of error or when skipping.
char *eval_to_string_skip(const char *arg,
                          const char **nextcmd,
                          const bool skip)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ARG(1)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    typval_st tv;
    char *retval;

    if(skip)
    {
        emsg_skip++;
    }

    if(eval_lev_0((uchar_kt *)arg, &tv, (uchar_kt **)nextcmd, !skip) == FAIL
       || skip)
    {
        retval = NULL;
    }
    else
    {
        retval = xstrdup(tv_get_string(&tv));
        tv_clear(&tv);
    }

    if(skip)
    {
        emsg_skip--;
    }

    return retval;
}

/// Skip over an expression at @b *pp
///
/// Return FAIL for an error, OK otherwise.
int skip_expr(uchar_kt **pp)
{
    typval_st rettv;
    *pp = skipwhite(*pp); // skip the leading white-space
    return eval_lev_1(pp, &rettv, FALSE);
}

/// Top level evaluation function, returning a string.
/// When @b convert is TRUE convert a List into a sequence
/// of lines and convert a Float to a String.
///
/// Return pointer to allocated memory, or NULL for failure.
uchar_kt *eval_to_string(uchar_kt *arg, uchar_kt **nextcmd, int convert)
{
    typval_st tv;
    char *retval;
    garray_st ga;

    if(eval_lev_0(arg, &tv, nextcmd, true) == FAIL)
    {
        retval = NULL;
    }
    else
    {
        if(convert && tv.v_type == kNvarList)
        {
            ga_init(&ga, (int)sizeof(char), 80);

            if(tv.vval.v_list != NULL)
            {
                tv_list_join(&ga, tv.vval.v_list, "\n");

                if(tv.vval.v_list->lv_len > 0)
                {
                    ga_append(&ga, NL);
                }
            }

            ga_append(&ga, NUL);
            retval = (char *)ga.ga_data;
        }
        else if(convert && tv.v_type == kNvarFloat)
        {
            char numbuf[NUMBUFLEN];
            xsnprintf(numbuf, NUMBUFLEN, "%g", tv.vval.v_float);
            retval = xstrdup(numbuf);
        }
        else
        {
            retval = xstrdup(tv_get_string(&tv));
        }

        tv_clear(&tv);
    }

    return (uchar_kt *)retval;
}

/// Call eval_to_string() without using current local variables and using
/// textlock. When @b use_sandbox is TRUE use the sandbox.
uchar_kt *eval_to_string_safe(uchar_kt *arg, uchar_kt **nextcmd, int use_sandbox)
{
    uchar_kt *retval;
    void *save_funccalp;
    save_funccalp = save_funccal();

    if(use_sandbox)
    {
        ++sandbox;
    }

    ++textlock;
    retval = eval_to_string(arg, nextcmd, FALSE);

    if(use_sandbox)
    {
        --sandbox;
    }

    --textlock;
    restore_funccal(save_funccalp);
    return retval;
}

/// Top level evaluation function, returning a number.
/// Evaluates "expr" silently.
/// Returns -1 for an error.
int eval_to_number(uchar_kt *expr)
{
    typval_st rettv;
    int retval;
    uchar_kt *p = skipwhite(expr);
    ++emsg_off;

    if(eval_lev_1(&p, &rettv, true) == FAIL)
    {
        retval = -1;
    }
    else
    {
        retval = tv_get_number_chk(&rettv, NULL);
        tv_clear(&rettv);
    }

    --emsg_off;
    return retval;
}

/// Prepare v: variable "idx" to be used.
/// Save the current typeval in "save_tv".
/// When not used yet add the variable to the v: hashtable.
static void prepare_vimvar(int idx, typval_st *save_tv)
{
    *save_tv = vimvars[idx].vv_tv;

    if(vimvars[idx].vv_type == kNvarUnknown)
    {
        hash_add(&vimvarht, vimvars[idx].vv_di.di_key);
    }
}

/// Restore v: variable "idx" to typeval "save_tv".
/// When no longer defined, remove the variable from the v: hashtable.
static void restore_vimvar(int idx, typval_st *save_tv)
{
    hashitem_st  *hi;
    vimvars[idx].vv_tv = *save_tv;

    if(vimvars[idx].vv_type == kNvarUnknown)
    {
        hi = hash_find(&vimvarht, vimvars[idx].vv_di.di_key);

        if(HASHITEM_EMPTY(hi))
        {
            EMSG2(_(e_intern2), "restore_vimvar()");
        }
        else
        {
            hash_remove(&vimvarht, hi);
        }
    }
}

/// Evaluate an expression to a list with suggestions.
/// For the "expr:" part of 'spellsuggest'.
/// Returns NULL when there is an error.
list_st *eval_spell_expr(uchar_kt *badword, uchar_kt *expr)
{
    typval_st save_val;
    typval_st rettv;
    list_st *list = NULL;
    uchar_kt *p = skipwhite(expr);

    // Set "v:val" to the bad word.
    prepare_vimvar(VV_VAL, &save_val);
    vimvars[VV_VAL].vv_type = kNvarString;
    vimvars[VV_VAL].vv_str = badword;

    if(p_verbose == 0)
    {
        ++emsg_off;
    }

    if(eval_lev_1(&p, &rettv, true) == OK)
    {
        if(rettv.v_type != kNvarList)
        {
            tv_clear(&rettv);
        }
        else
        {
            list = rettv.vval.v_list;
        }
    }

    if(p_verbose == 0)
    {
        --emsg_off;
    }

    restore_vimvar(VV_VAL, &save_val);
    return list;
}

/// "list" is supposed to contain two items: a word and a number. Return the
/// word in "pp" and the number as the return value.
/// Return -1 if anything isn't right.
/// Used to get the good word and score from the eval_spell_expr() result.
int get_spellword(list_st *list, const char **pp)
{
    listitem_st *li;
    li = list->lv_first;

    if(li == NULL)
    {
        return -1;
    }

    *pp = tv_get_string(&li->li_tv);
    li = li->li_next;

    if(li == NULL)
    {
        return -1;
    }

    return tv_get_number(&li->li_tv);
}


/// Call some vimL function and return the result in "*rettv".
/// Uses argv[argc] for the function arguments. Only Number and String
/// arguments are currently supported.
///
/// @param func
/// @param argc
/// @param argv
/// @param safe          use the sandbox
/// @param str_arg_only  all arguments are strings
/// @param rettv
///
/// Return OK or FAIL.
int call_vim_function(const uchar_kt *func,
                      int argc,
                      const uchar_kt *const *const argv,
                      int safe,
                      int str_arg_only,
                      typval_st *rettv)
{
    long n;
    int len;
    int doesrange;
    void *save_funccalp = NULL;
    int ret;
    typval_st *argvars = xmalloc((argc + 1) * sizeof(typval_st));

    for(int i = 0; i < argc; i++)
    {
        // Pass a NULL or empty argument as an empty string
        if(argv[i] == NULL || *argv[i] == NUL)
        {
            argvars[i].v_type = kNvarString;
            argvars[i].vval.v_string = (uchar_kt *)"";
            continue;
        }

        if(str_arg_only)
        {
            len = 0;
        }
        else
        {
            // Recognize a number argument, the others must be strings.
            str_to_num(argv[i], NULL, &len, kStrToNumAll, &n, NULL, 0);
        }

        if(len != 0 && len == (int)ustrlen(argv[i]))
        {
            argvars[i].v_type = kNvarNumber;
            argvars[i].vval.v_number = n;
        }
        else
        {
            argvars[i].v_type = kNvarString;
            argvars[i].vval.v_string = (uchar_kt *)argv[i];
        }
    }

    if(safe)
    {
        save_funccalp = save_funccal();
        ++sandbox;
    }

    rettv->v_type = kNvarUnknown; // tv_clear() uses this.

    ret = call_func(func,
                    (int)ustrlen(func),
                    rettv,
                    argc,
                    argvars,
                    NULL,
                    curwin->w_cursor.lnum,
                    curwin->w_cursor.lnum,
                    &doesrange,
                    true,
                    NULL,
                    NULL);

    if(safe)
    {
        --sandbox;
        restore_funccal(save_funccalp);
    }

    xfree(argvars);

    if(ret == FAIL)
    {
        tv_clear(rettv);
    }

    return ret;
}

/// Call vimL function "func" and return the result as a number.
///
/// @param func
/// @param argc
/// @param argv
/// @param safe   use the sandbox
///
/// Returns -1 when calling the function fails.
/// Uses argv[argc] for the function arguments.
long call_func_retnr(uchar_kt *func,
                     int argc,
                     const uchar_kt *const *const argv,
                     int safe)
{
    typval_st rettv;
    long retval;

    // All arguments are passed as strings, no conversion to number.
    if(call_vim_function(func, argc, argv, safe, TRUE, &rettv) == FAIL)
    {
        return -1;
    }

    retval = tv_get_number_chk(&rettv, NULL);
    tv_clear(&rettv);
    return retval;
}

/// Call VimL function and return the result as a string
///
/// @param[in]  func  Function name.
/// @param[in]  argc  Number of arguments.
/// @param[in]  argv  Array with string arguments.
/// @param[in]  safe  Use the sandbox.
///
/// @return [allocated]
/// NULL when calling function failes, allocated string otherwise.
char *call_func_retstr(const char *const func,
                       const int argc,
                       const uchar_kt *const *const argv,
                       const bool safe)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    typval_st rettv;

    // All arguments are passed as strings, no conversion to number.
    if(call_vim_function((const uchar_kt *)func, argc,
                         argv, safe, true, &rettv) == FAIL)
    {
        return NULL;
    }

    char *const retval = xstrdup(tv_get_string(&rettv));
    tv_clear(&rettv);

    return retval;
}

/// Call vimL function "func" and return the result as a List.
///
/// @param[in]  func  Function name.
/// @param[in]  argc  Number of arguments.
/// @param[in]  argv  Array with string arguments.
/// @param[in]  safe  Use the sandbox.
///
/// Uses argv[argc] for the function arguments.
/// Returns NULL when there is something wrong.
///
void *call_func_retlist(uchar_kt *func,
                        int argc,
                        const uchar_kt *const *const argv,
                        int safe)
{
    typval_st rettv;

    // All arguments are passed as strings, no conversion to number.
    if(call_vim_function(func, argc, argv, safe, TRUE, &rettv) == FAIL)
    {
        return NULL;
    }

    if(rettv.v_type != kNvarList)
    {
        tv_clear(&rettv);
        return NULL;
    }

    return rettv.vval.v_list;
}

/// Save the current function call pointer, and set it to NULL.
/// Used when executing autocommands and for ":source".
void *save_funccal(void)
{
    func_call_st *fc = current_funccal;
    current_funccal = NULL;
    return (void *)fc;
}

void restore_funccal(void *vfc)
{
    func_call_st *fc = (func_call_st *)vfc;
    current_funccal = fc;
}

/// Prepare profiling for entering a child or something else
/// that is not counted for the script/function itself.
/// Should always be called in pair with prof_child_exit().
///
/// @param tm  place to store waittime
void prof_child_enter(proftime_kt *tm)
{
    func_call_st *fc = current_funccal;
    if(fc != NULL && fc->func->uf_profiling)
    {
        fc->prof_child = profile_start();
    }
    script_prof_save(tm);
}

/// Take care of time spent in a child.
/// Should always be called after prof_child_enter().
///
/// @param tm  where waittime was stored
void prof_child_exit(proftime_kt *tm)
{
    func_call_st *fc = current_funccal;
    if(fc != NULL && fc->func->uf_profiling)
    {
        fc->prof_child = profile_end(fc->prof_child);

        fc->prof_child =
            profile_sub_wait(*tm, fc->prof_child); // don't count waiting time

        fc->func->uf_tm_children =
            profile_add(fc->func->uf_tm_children, fc->prof_child);

        fc->func->uf_tml_children =
            profile_add(fc->func->uf_tml_children, fc->prof_child);
    }
    script_prof_restore(tm);
}


/// Evaluate 'foldexpr'.
/// Returns the foldlevel, and any character preceding
/// it in "*cp". Doesn't give error messages.
int eval_foldexpr(uchar_kt *arg, int *cp)
{
    typval_st tv;
    int retval;
    uchar_kt *s;
    int use_sandbox = was_set_insecurely((uchar_kt *)"foldexpr", kOptSetLocal);
    ++emsg_off;

    if(use_sandbox)
    {
        ++sandbox;
    }

    ++textlock;
    *cp = NUL;

    if(eval_lev_0(arg, &tv, NULL, TRUE) == FAIL)
    {
        retval = 0;
    }
    else
    {
        // If the result is a number, just return the number.
        if(tv.v_type == kNvarNumber)
        {
            retval = tv.vval.v_number;
        }
        else if(tv.v_type != kNvarString || tv.vval.v_string == NULL)
        {
            retval = 0;
        }
        else
        {
            // If the result is a string, check if there
            // is a non-digit before the number.
            s = tv.vval.v_string;

            if(!ascii_isdigit(*s) && *s != '-')
            {
                *cp = *s++;
            }

            retval = atol((char *)s);
        }

        tv_clear(&tv);
    }

    --emsg_off;

    if(use_sandbox)
    {
        --sandbox;
    }

    --textlock;
    return retval;
}

/// - :let,                     list all variable values
/// - :let var1 var2,           list variable values
/// - :let var = expr,          assignment command.
/// - :let var += expr,         assignment command.
/// - :let var -= expr,         assignment command.
/// - :let var .= expr,         assignment command.
/// - :let [var1, var2] = expr, unpack list.
void ex_let(exargs_st *eap)
{
    uchar_kt *arg = eap->arg;
    uchar_kt *expr = NULL;
    typval_st rettv;
    int i;
    int var_count = 0;
    int semicolon = 0;
    uchar_kt op[2];
    uchar_kt *argend;
    int first = TRUE;
    argend = (uchar_kt *)skip_var_list(arg, &var_count, &semicolon);

    if(argend == NULL)
    {
        return;
    }

    if(argend > arg && argend[-1] == '.') // For var.='str'.
    {
        argend--;
    }

    expr = skipwhite(argend);

    if(*expr != '='
       && !(ustrchr((uchar_kt *)"+-.", *expr) != NULL && expr[1] == '='))
    {
        // ":let" without "=": list variables
        if(*arg == '[')
        {
            EMSG(_(e_invarg));
        }
        else if(!ends_excmd(*arg))
        {
            // ":let var1 var2"
            arg = (uchar_kt *)list_arg_vars(eap, (const char *)arg, &first);
        }
        else if(!eap->skip)
        {
            // ":let"
            list_glob_vars(&first);
            list_buf_vars(&first);
            list_win_vars(&first);
            list_tab_vars(&first);
            list_script_vars(&first);
            list_func_vars(&first);
            list_vim_vars(&first);
        }

        eap->nextcmd = check_nextcmd(arg);
    }
    else
    {
        op[0] = '=';
        op[1] = NUL;

        if(*expr != '=')
        {
            if(ustrchr((uchar_kt *)"+-.", *expr) != NULL)
            {
                op[0] = *expr; // +=, -=, .=
            }

            expr = skipwhite(expr + 2);
        }
        else
        {
            expr = skipwhite(expr + 1);
        }

        if(eap->skip)
        {
            ++emsg_skip;
        }

        i = eval_lev_0(expr, &rettv, &eap->nextcmd, !eap->skip);

        if(eap->skip)
        {
            if(i != FAIL)
            {
                tv_clear(&rettv);
            }

            emsg_skip--;
        }
        else if(i != FAIL)
        {
            (void)ex_let_vars(eap->arg, &rettv, false,
                              semicolon, var_count, op);

            tv_clear(&rettv);
        }
    }
}

/// Assign the typevalue "tv" to the variable or variables at "arg_start".
/// Handles both "var" with any type and "[var, var; var]" with a list type.
/// When "nextchars" is not NULL it points to a string with characters that
/// must appear after the variable(s).  Use "+", "-" or "." for add, subtract
/// or concatenate.
///
/// @param arg_start
/// @param tv
/// @param copy         copy values from "tv", don't move
/// @param semicolon    from skip_var_list()
/// @param var_count    from skip_var_list()
/// @param nextchars
///
/// Returns OK or FAIL
static int ex_let_vars(uchar_kt *arg_start,
                       typval_st *tv,
                       int copy,
                       int semicolon,
                       int var_count,
                       uchar_kt *nextchars)
{
    int i;
    list_st *l;
    typval_st ltv;
    listitem_st *item;
    uchar_kt *arg = arg_start;

    if(*arg != '[')
    {
        // ":let var = expr" or ":for var in list"
        if(ex_let_one(arg, tv, copy, nextchars, nextchars) == NULL)
        {
            return FAIL;
        }

        return OK;
    }

    // ":let [v1, v2] = list" or ":for [v1, v2] in listlist"
    if(tv->v_type != kNvarList || (l = tv->vval.v_list) == NULL)
    {
        EMSG(_(e_listreq));
        return FAIL;
    }

    i = tv_list_len(l);

    if(semicolon == 0 && var_count < i)
    {
        EMSG(_("E687: Less targets than List items"));
        return FAIL;
    }

    if(var_count - semicolon > i)
    {
        EMSG(_("E688: More targets than List items"));
        return FAIL;
    }

    item = l->lv_first;

    while(*arg != ']')
    {
        arg = skipwhite(arg + 1);
        arg = ex_let_one(arg, &item->li_tv, TRUE, (uchar_kt *)",;]", nextchars);
        item = item->li_next;

        if(arg == NULL)
        {
            return FAIL;
        }

        arg = skipwhite(arg);

        if(*arg == ';')
        {
            // Put the rest of the list (may be empty) in the var after ';'.
            // Create a new list for this.
            l = tv_list_alloc();

            while(item != NULL)
            {
                tv_list_append_tv(l, &item->li_tv);
                item = item->li_next;
            }

            ltv.v_type = kNvarList;
            ltv.v_lock = 0;
            ltv.vval.v_list = l;
            l->lv_refcount = 1;

            arg = ex_let_one(skipwhite(arg + 1), &ltv,
                             false, (uchar_kt *)"]", nextchars);

            tv_clear(&ltv);

            if(arg == NULL)
            {
                return FAIL;
            }

            break;
        }
        else if(*arg != ',' && *arg != ']')
        {
            EMSG2(_(e_intern2), "ex_let_vars()");
            return FAIL;
        }
    }

    return OK;
}

/// Skip over assignable variable "var" or list of variables "[var, var]".
/// Used for ":let varvar = expr" and ":for varvar in expr".
/// For "[var, var]" increment "*var_count" for each variable.
/// for "[var, var; var]" set "semicolon".
/// Return NULL for an error.
static const uchar_kt *skip_var_list(const uchar_kt *arg,
                                   int *var_count,
                                   int *semicolon)
{
    const uchar_kt *p;
    const uchar_kt *s;

    if(*arg == '[')
    {
        // "[var, var]": find the matching ']'.
        p = arg;

        for(;;)
        {
            p = skipwhite(p + 1);// skip whites after '[', ';' or ','
            s = skip_var_one(p);

            if(s == p)
            {
                EMSG2(_(e_invarg2), p);
                return NULL;
            }

            ++*var_count;
            p = skipwhite(s);

            if(*p == ']')
            {
                break;
            }
            else if(*p == ';')
            {
                if(*semicolon == 1)
                {
                    EMSG(_("Double ; in list of variables"));
                    return NULL;
                }

                *semicolon = 1;
            }
            else if(*p != ',')
            {
                EMSG2(_(e_invarg2), p);
                return NULL;
            }
        }

        return p + 1;
    }
    else
    {
        return skip_var_one(arg);
    }
}

/// Skip one (assignable) variable name,
/// including @r, $VAR, &option, d.key, l[idx].
static const uchar_kt *skip_var_one(const uchar_kt *arg)
{
    if(*arg == '@' && arg[1] != NUL)
    {
        return arg + 2;
    }

    return find_name_end(*arg == '$' || *arg == '&' ? arg + 1 : arg,
                         NULL,
                         NULL,
                         FNE_INCL_BR | FNE_CHECK_START);
}

/// List variables for hashtab "ht" with prefix "prefix".
/// If "empty" is TRUE also list NULL strings as empty strings.
static void list_hashtable_vars(hashtable_st *ht,
                                const char *prefix,
                                int empty,
                                int *first)
{
    hashitem_st *hi;
    dictitem_st *di;
    int todo = (int)ht->ht_used;

    for(hi = ht->ht_array; todo > 0 && !got_int; ++hi)
    {
        if(!HASHITEM_EMPTY(hi))
        {
            todo--;
            di = TV_DICT_HI2DI(hi);

            if(empty
               || di->di_tv.v_type != kNvarString
               || di->di_tv.vval.v_string != NULL)
            {
                list_one_var(di, prefix, first);
            }
        }
    }
}

/// List global variables.
static void list_glob_vars(int *first)
{
    list_hashtable_vars(&globvarht, "", true, first);
}

/// List buffer variables.
static void list_buf_vars(int *first)
{
    list_hashtable_vars(&curbuf->b_vars->dv_hashtab, "b:", true, first);
}

/// List window variables.
static void list_win_vars(int *first)
{
    list_hashtable_vars(&curwin->w_vars->dv_hashtab, "w:", true, first);
}

/// List tab page variables.
static void list_tab_vars(int *first)
{
    list_hashtable_vars(&curtab->tp_vars->dv_hashtab, "t:", true, first);
}

/// List Vim variables.
static void list_vim_vars(int *first)
{
    list_hashtable_vars(&vimvarht, "v:", false, first);
}

/// List script-local variables, if there is a script.
static void list_script_vars(int *first)
{
    if(current_SID > 0 && current_SID <= ga_scripts.ga_len)
    {
        list_hashtable_vars(&SCRIPT_VARS(current_SID), "s:", false, first);
    }
}

/// List function variables, if there is a function.
static void list_func_vars(int *first)
{
    if(current_funccal != NULL)
    {
        list_hashtable_vars(&current_funccal->l_vars.dv_hashtab,
                            "l:", false, first);
    }
}

/// List variables in "arg".
static const char *list_arg_vars(exargs_st *eap, const char *arg, int *first)
{
    int len;
    typval_st tv;
    const char *name;
    const char *name_start;
    int error = FALSE;

    while(!ends_excmd(*arg) && !got_int)
    {
        if(error || eap->skip)
        {
            arg = (const char *)find_name_end((uchar_kt *)arg,
                                              NULL,
                                              NULL,
                                              FNE_INCL_BR | FNE_CHECK_START);

            if(!ascii_iswhite(*arg) && !ends_excmd(*arg))
            {
                emsg_severe = TRUE;
                EMSG(_(e_trailing));
                break;
            }
        }
        else
        {
            // get_name_len() takes care of expanding curly braces
            name_start = name = arg;
            char *tofree;
            len = get_name_len(&arg, &tofree, true, true);

            if(len <= 0)
            {
                // This is mainly to keep test 49 working: when expanding
                // curly braces fails overrule the exception error message.
                if(len < 0 && !aborting())
                {
                    emsg_severe = TRUE;
                    EMSG2(_(e_invarg2), arg);
                    break;
                }

                error = TRUE;
            }
            else
            {
                if(tofree != NULL)
                {
                    name = tofree;
                }

                if(get_var_tv((const char *)name, len,
                              &tv, NULL, true, false) == FAIL)
                {
                    error = true;
                }
                else
                {
                    // handle d.key, l[idx], f(expr)
                    const char *const arg_subsc = arg;

                    if(handle_subscript(&arg, &tv, true, true) == FAIL)
                    {
                        error = true;
                    }
                    else
                    {
                        if(arg == arg_subsc && len == 2 && name[1] == ':')
                        {
                            switch(*name)
                            {
                                case 'g':
                                    list_glob_vars(first);
                                    break;

                                case 'b':
                                    list_buf_vars(first);
                                    break;

                                case 'w':
                                    list_win_vars(first);
                                    break;

                                case 't':
                                    list_tab_vars(first);
                                    break;

                                case 'v':
                                    list_vim_vars(first);
                                    break;

                                case 's':
                                    list_script_vars(first);
                                    break;

                                case 'l':
                                    list_func_vars(first);
                                    break;

                                default:
                                    EMSG2(_("E738: Can't list variables for %s"), name);
                            }
                        }
                        else
                        {
                            char *const s = encode_tv2echo(&tv, NULL);

                            const char *const used_name = (arg == arg_subsc
                                                           ? name : name_start);

                            const ptrdiff_t name_size
                                = (used_name == tofree
                                   ? (ptrdiff_t)strlen(used_name)
                                   : (arg - used_name));

                            list_one_var_a("",
                                           used_name,
                                           name_size,
                                           tv.v_type,
                                           s == NULL ? "" : s,
                                           first);

                            xfree(s);
                        }

                        tv_clear(&tv);
                    }
                }
            }

            xfree(tofree);
        }

        arg = (const char *)skipwhite((const uchar_kt *)arg);
    }

    return arg;
}

/// Set one item of :let var = expr or :let [v1, v2] = list to its value
///
/// @param[in]  arg       Start of the variable name.
/// @param[in]  tv        Value to assign to the variable.
/// @param[in]  copy      If true, copy value from `tv`.
/// @param[in]  endchars  Valid characters after variable name or NULL.
/// @param[in]  op        Operation performed: *op is +, -, . for +=, etc.
///                       NULL for =.
///
/// @return
/// a pointer to the char just after the var name or NULL in case of error.
///
/// @todo move to eval/ex_cmds
static uchar_kt *ex_let_one(uchar_kt *arg,
                          typval_st *const tv,
                          const bool copy,
                          const uchar_kt *const endchars,
                          const uchar_kt *const op)
FUNC_ATTR_NONNULL_ARG(1, 2)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    uchar_kt *arg_end = NULL;
    int len;
    int opt_flags;
    uchar_kt *tofree = NULL;

    // ":let $VAR = expr": Set environment variable.
    if(*arg == '$')
    {

        arg++; // Find the end of the name.
        char *name = (char *)arg;
        len = get_env_len((const uchar_kt **)&arg);

        if(len == 0)
        {
            EMSG2(_(e_invarg2), name - 1);
        }
        else
        {
            if(op != NULL && (*op == '+' || *op == '-'))
            {
                EMSG2(_(e_letwrong), op);
            }
            else if(endchars != NULL
                    && ustrchr(endchars, *skipwhite(arg)) == NULL)
            {
                EMSG(_(e_letunexp));
            }
            else if(!check_secure())
            {
                const char c1 = name[len];
                name[len] = NUL;
                const char *p = tv_get_string_chk(tv);

                if(p != NULL && op != NULL && *op == '.')
                {
                    char *s = vim_getenv(name);

                    if(s != NULL)
                    {
                        tofree = (uchar_kt *)xstrdup_concat(s, p);

                        p = (const char *)tofree;
                        xfree(s);
                    }
                }

                if(p != NULL)
                {
                    vim_setenv(name, p);

                    if(ustricmp(name, ENV_GKIDE_USR_HOME) == 0)
                    {
                        (void)init_gkide_usr_home();
                    }
                    else if(didset_vim && ustricmp(name, "VIM") == 0)
                    {
                        didset_vim = false;
                    }
                    else if(didset_vimruntime
                            && ustricmp(name, "VIMRUNTIME") == 0)
                    {
                        didset_vimruntime = false;
                    }

                    arg_end = arg;
                }

                name[len] = c1;
                xfree(tofree);
            }
        }

        // ":let &option = expr": Set option value.
        // ":let &l:option = expr": Set local option value.
        // ":let &g:option = expr": Set global option value.
    }
    else if(*arg == '&')
    {
        // Find the end of the name.
        char *const p = (char *)find_option_end((const char **)&arg, &opt_flags);

        if(p == NULL
           || (endchars != NULL
               && ustrchr(endchars, *skipwhite((const uchar_kt *)p)) == NULL))
        {
            EMSG(_(e_letunexp));
        }
        else
        {
            int opt_type;
            long numval;
            char *stringval = NULL;
            const char c1 = *p;
            *p = NUL;
            number_kt n = tv_get_number(tv);

            // != NULL if number or string.
            const char *s = tv_get_string_chk(tv);

            if(s != NULL && op != NULL && *op != '=')
            {
                opt_type = get_option_value(arg,
                                            &numval,
                                            (uchar_kt **)&stringval,
                                            opt_flags);

                if((opt_type == 1 && *op == '.')
                   || (opt_type == 0 && *op != '.'))
                {
                    EMSG2(_(e_letwrong), op);
                }
                else
                {
                    if(opt_type == 1) // number
                    {
                        if(*op == '+')
                        {
                            n = numval + n;
                        }
                        else
                        {
                            n = numval - n;
                        }
                    }
                    else if(opt_type == 0 && stringval != NULL) // string
                    {
                        char *const oldstringval = stringval;

                        stringval = xstrdup_concat(stringval, s);

                        xfree(oldstringval);
                        s = stringval;
                    }
                }
            }

            if(s != NULL)
            {
                set_option_value((const char *)arg, n, s, opt_flags);
                arg_end = (uchar_kt *)p;
            }

            *p = c1;
            xfree(stringval);
        }
        // ":let @r = expr": Set register contents.
    }
    else if(*arg == '@')
    {
        arg++;

        if(op != NULL && (*op == '+' || *op == '-'))
        {
            emsgf(_(e_letwrong), op);
        }
        else if(endchars != NULL
                && ustrchr(endchars, *skipwhite(arg + 1)) == NULL)
        {
            emsgf(_(e_letunexp));
        }
        else
        {
            uchar_kt *s;
            uchar_kt *ptofree = NULL;
            const char *p = tv_get_string_chk(tv);

            if(p != NULL && op != NULL && *op == '.')
            {
                s = get_reg_contents(*arg == '@' ? '"' : *arg, kGRegExprSrc);

                if(s != NULL)
                {
                    ptofree = ustrdup_concat(s, (const uchar_kt *)p);
                    p = (const char *)ptofree;
                    xfree(s);
                }
            }

            if(p != NULL)
            {
                write_reg_contents(*arg == '@' ? '"' : *arg,
                                   (const uchar_kt *)p, ustrlen(p), false);
                arg_end = arg + 1;
            }

            xfree(ptofree);
        }
    }

    // ":let var = expr": Set internal variable.
    // ":let {expr} = expr": Idem, name made with curly braces
    else if(eval_isnamec1(*arg) || *arg == '{')
    {
        leftval_st lv;
        uchar_kt *const p = get_lval(arg, tv, &lv, false,
                                   false, 0, FNE_CHECK_START);

        if(p != NULL && lv.ll_name != NULL)
        {
            if(endchars != NULL
               && ustrchr(endchars, *skipwhite(p)) == NULL)
            {
                EMSG(_(e_letunexp));
            }
            else
            {
                set_var_lval(&lv, p, tv, copy, op);
                arg_end = p;
            }
        }

        clear_lval(&lv);
    }
    else
    {
        EMSG2(_(e_invarg2), arg);
    }

    return arg_end;
}

/// Get an lvalue
///
/// Lvalue may be
/// - variable: "name", "na{me}"
/// - dictionary item: "dict.key", "dict['key']"
/// - list item: "list[expr]"
/// - list slice: "list[expr:expr]"
///
/// Indexing only works if trying to use it with an existing List or Dictionary.
///
/// @param[in]  name
/// Name to parse.
///
/// @param  rettv
/// Pointer to the value to be assigned or NULL.
///
/// @param[out]  lp
/// Lvalue definition. When evaluation errors occur `->ll_name` is NULL.
///
/// @param[in]  unlet
/// True if using `:unlet`. This results in slightly different behaviour
/// when something is wrong; must end in space or cmd separator.
///
/// @param[in]  skip
/// True when skipping.
///
/// @param[in]  flags
/// @see GetLvalFlags.
///
/// @param[in]  fne_flags
/// Flags for find_name_end().
///
/// @return
/// A pointer to just after the name, including indexes.
/// Returns NULL for a parsing error, but it is still needed
/// to free items in lp.
///
/// @todo move to eval/executor
static uchar_kt *get_lval(uchar_kt *const name,
                        typval_st *const rettv,
                        leftval_st *const lp,
                        const bool unlet,
                        const bool skip,
                        const int flags,
                        const int fne_flags)
FUNC_ATTR_NONNULL_ARG(1, 3)
{
    dictitem_st *v;
    typval_st var1;
    typval_st var2;
    int empty1 = FALSE;
    listitem_st *ni;
    hashtable_st *ht;
    int quiet = flags & GLV_QUIET;

    // Clear everything in "lp".
    memset(lp, 0, sizeof(leftval_st));

    if(skip)
    {
        // When skipping just find the end of the name.
        lp->ll_name = (const char *)name;

        return (uchar_kt *)find_name_end((const uchar_kt *)name,
                                       NULL,
                                       NULL,
                                       FNE_INCL_BR | fne_flags);
    }

    // Find the end of the name.
    uchar_kt *expr_start;
    uchar_kt *expr_end;
    uchar_kt *p = (uchar_kt *)find_name_end(name,
                                        (const uchar_kt **)&expr_start,
                                        (const uchar_kt **)&expr_end,
                                        fne_flags);

    if(expr_start != NULL)
    {
        // Don't expand the name when we
        // already know there is an error.
        if(unlet
           && !ascii_iswhite(*p)
           && !ends_excmd(*p)
           && *p != '[' && *p != '.')
        {
            EMSG(_(e_trailing));
            return NULL;
        }

        lp->ll_exp_name = (char *)make_expanded_name(name,
                                                     expr_start,
                                                     expr_end,
                                                     (uchar_kt *)p);

        lp->ll_name = lp->ll_exp_name;

        if(lp->ll_exp_name == NULL)
        {
            // Report an invalid expression in braces, unless the
            // expression evaluation has been cancelled due to an
            // aborting error, an interrupt, or an exception.
            if(!aborting() && !quiet)
            {
                emsg_severe = TRUE;
                EMSG2(_(e_invarg2), name);
                return NULL;
            }

            lp->ll_name_len = 0;
        }
        else
        {
            lp->ll_name_len = strlen(lp->ll_name);
        }
    }
    else
    {
        lp->ll_name = (const char *)name;
        lp->ll_name_len = (size_t)((const char *)p - lp->ll_name);
    }

    // Without [idx] or .key we are done.
    if((*p != '[' && *p != '.') || lp->ll_name == NULL)
    {
        return p;
    }

    v = find_var(lp->ll_name, lp->ll_name_len, &ht, flags & GLV_NO_AUTOLOAD);

    if(v == NULL && !quiet)
    {
        emsgf(_("E121: Undefined variable: %.*s"),
              (int)lp->ll_name_len, lp->ll_name);
    }

    if(v == NULL)
    {
        return NULL;
    }

    // Loop until no more [idx] or .key is following.
    lp->ll_tv = &v->di_tv;

    while(*p == '[' || (*p == '.' && lp->ll_tv->v_type == kNvarDict))
    {
        if(!(lp->ll_tv->v_type == kNvarList && lp->ll_tv->vval.v_list != NULL)
           && !(lp->ll_tv->v_type == kNvarDict && lp->ll_tv->vval.v_dict != NULL))
        {
            if(!quiet)
            {
                EMSG(_("E689: Can only index a List or Dictionary"));
            }

            return NULL;
        }

        if(lp->ll_range)
        {
            if(!quiet)
            {
                EMSG(_("E708: [:] must come last"));
            }

            return NULL;
        }

        int len = -1;
        uchar_kt *key = NULL;

        if(*p == '.')
        {
            key = p + 1;

            for(len = 0; ASCII_ISALNUM(key[len]) || key[len] == '_'; len++)
            { /* empty body */ }

            if(len == 0)
            {
                if(!quiet)
                {
                    EMSG(_("E713: Cannot use empty key after ."));
                }

                return NULL;
            }

            p = key + len;
        }
        else
        {
            // Get the index [expr] or the first index [expr: ].
            p = skipwhite(p + 1);

            if(*p == ':')
            {
                empty1 = true;
            }
            else
            {
                empty1 = false;

                if(eval_lev_1(&p, &var1, true) == FAIL)
                {
                    // Recursive!
                    return NULL;
                }

                if(!tv_check_str(&var1))
                {
                    // Not a number or string.
                    tv_clear(&var1);
                    return NULL;
                }
            }

            // Optionally get the second index [ :expr].
            if(*p == ':')
            {
                if(lp->ll_tv->v_type == kNvarDict)
                {
                    if(!quiet)
                    {
                        EMSG(_(e_dictrange));
                    }

                    if(!empty1)
                    {
                        tv_clear(&var1);
                    }

                    return NULL;
                }

                if(rettv != NULL
                   && (rettv->v_type != kNvarList || rettv->vval.v_list == NULL))
                {
                    if(!quiet)
                    {
                        emsgf(_("E709: [:] requires a List value"));
                    }

                    if(!empty1)
                    {
                        tv_clear(&var1);
                    }

                    return NULL;
                }

                p = skipwhite(p + 1);

                if(*p == ']')
                {
                    lp->ll_empty2 = true;
                }
                else
                {
                    lp->ll_empty2 = false;

                    if(eval_lev_1(&p, &var2, true) == FAIL)
                    {
                        // Recursive!
                        if(!empty1)
                        {
                            tv_clear(&var1);
                        }

                        return NULL;
                    }

                    if(!tv_check_str(&var2))
                    {
                        // Not a number or string.
                        if(!empty1)
                        {
                            tv_clear(&var1);
                        }

                        tv_clear(&var2);
                        return NULL;
                    }
                }

                lp->ll_range = TRUE;
            }
            else
            {
                lp->ll_range = FALSE;
            }

            if(*p != ']')
            {
                if(!quiet)
                {
                    emsgf(_(e_missbrac));
                }

                if(!empty1)
                {
                    tv_clear(&var1);
                }

                if(lp->ll_range && !lp->ll_empty2)
                {
                    tv_clear(&var2);
                }

                return NULL;
            }

            ++p; // Skip to past ']'.
        }

        if(lp->ll_tv->v_type == kNvarDict)
        {
            if(len == -1)
            {
                // "[key]": get key from "var1"
                key = (uchar_kt *)tv_get_string(&var1); // is number or string

                if(key == NULL)
                {
                    tv_clear(&var1);
                    return NULL;
                }
            }

            lp->ll_list = NULL;
            lp->ll_dict = lp->ll_tv->vval.v_dict;
            lp->ll_di = tv_dict_find(lp->ll_dict, (const char *)key, len);

            // When assigning to a scope dictionary check that a function and
            // variable name is valid (only variable name unless it is l: or
            // g: dictionary). Disallow overwriting a builtin function.
            if(rettv != NULL && lp->ll_dict->dv_scope != 0)
            {
                int prevval;
                int wrong;

                if(len != -1)
                {
                    prevval = key[len];
                    key[len] = NUL;
                }
                else
                {
                    prevval = 0; // Avoid compiler warning.
                }

                wrong = ((lp->ll_dict->dv_scope == VAR_DEF_SCOPE
                          && tv_is_func(*rettv)
                          && !var_check_func_name((const char *)key, lp->ll_di == NULL))
                         || !valid_varname((const char *)key));

                if(len != -1)
                {
                    key[len] = prevval;
                }

                if(wrong)
                {
                    return NULL;
                }
            }

            if(lp->ll_di == NULL)
            {
                // Can't add "v:" variable.
                if(lp->ll_dict == &vimvardict)
                {
                    EMSG2(_(e_illvar), name);
                    return NULL;
                }

                // Key does not exist in dict: may need to add it.
                if(*p == '[' || *p == '.' || unlet)
                {
                    if(!quiet)
                    {
                        emsgf(_(e_dictkey), key);
                    }

                    if(len == -1)
                    {
                        tv_clear(&var1);
                    }

                    return NULL;
                }

                if(len == -1)
                {
                    lp->ll_newkey = ustrdup(key);
                }
                else
                {
                    lp->ll_newkey = ustrndup(key, len);
                }

                if(len == -1)
                {
                    tv_clear(&var1);
                }

                break;
                // existing variable, need to check if it can be changed
            }
            else if(!(flags & GLV_READ_ONLY)
                    && var_check_ro(lp->ll_di->di_flags,
                                    (const char *)name,
                                    (size_t)(p - name)))
            {
                if(len == -1)
                {
                    tv_clear(&var1);
                }

                return NULL;
            }

            if(len == -1)
            {
                tv_clear(&var1);
            }

            lp->ll_tv = &lp->ll_di->di_tv;
        }
        else
        {
            // Get the number and item for the only or first index of the List.
            if(empty1)
            {
                lp->ll_n1 = 0;
            }
            else
            {
                lp->ll_n1 = tv_get_number(&var1); // Is number or string.
                tv_clear(&var1);
            }

            lp->ll_dict = NULL;
            lp->ll_list = lp->ll_tv->vval.v_list;
            lp->ll_li = tv_list_find(lp->ll_list, lp->ll_n1);

            if(lp->ll_li == NULL)
            {
                if(lp->ll_n1 < 0)
                {
                    lp->ll_n1 = 0;
                    lp->ll_li = tv_list_find(lp->ll_list, lp->ll_n1);
                }
            }

            if(lp->ll_li == NULL)
            {
                if(lp->ll_range && !lp->ll_empty2)
                {
                    tv_clear(&var2);
                }

                if(!quiet)
                {
                    EMSGN(_(e_listidx), lp->ll_n1);
                }

                return NULL;
            }

            // May need to find the item or absolute index for the second
            // index of a range.
            // When no index given: "lp->ll_empty2" is TRUE.
            // Otherwise "lp->ll_n2" is set to the second index.
            if(lp->ll_range && !lp->ll_empty2)
            {
                lp->ll_n2 = tv_get_number(&var2); // Is number or string.
                tv_clear(&var2);

                if(lp->ll_n2 < 0)
                {
                    ni = tv_list_find(lp->ll_list, lp->ll_n2);

                    if(ni == NULL)
                    {
                        if(!quiet)
                        {
                            EMSGN(_(e_listidx), lp->ll_n2);
                        }

                        return NULL;
                    }

                    lp->ll_n2 = tv_list_idx_of_item(lp->ll_list, ni);
                }

                // Check that lp->ll_n2 isn't before lp->ll_n1.
                if(lp->ll_n1 < 0)
                {
                    lp->ll_n1 = tv_list_idx_of_item(lp->ll_list, lp->ll_li);
                }

                if(lp->ll_n2 < lp->ll_n1)
                {
                    if(!quiet)
                    {
                        EMSGN(_(e_listidx), lp->ll_n2);
                    }

                    return NULL;
                }
            }

            lp->ll_tv = &lp->ll_li->li_tv;
        }
    }

    return p;
}

/// Clear lval "lp" that was filled by get_lval().
///
/// @todo move to eval/executor
static void clear_lval(leftval_st *lp)
{
    xfree(lp->ll_exp_name);
    xfree(lp->ll_newkey);
}

// TODO(ZyX-I): move to eval/executor

/// Set a variable that was parsed by get_lval() to "rettv".
/// "endp" points to just after the parsed name.
/// "op" is NULL, "+" for "+=", "-" for "-=", "." for ".=" or "=" for "=".
///
/// @todo move to eval/executor
static void set_var_lval(leftval_st *lp,
                         uchar_kt *endp,
                         typval_st *rettv,
                         int copy,
                         const uchar_kt *op)
{
    int cc;
    listitem_st *ri;
    dictitem_st *di;

    if(lp->ll_tv == NULL)
    {
        cc = *endp;
        *endp = NUL;

        if(op != NULL && *op != '=')
        {
            typval_st tv;
            di = NULL; // handle +=, -= and .=

            if(get_var_tv((const char *)lp->ll_name,
                          (int)ustrlen(lp->ll_name),
                          &tv,
                          &di,
                          true,
                          false) == OK)
            {
                if((di == NULL
                    || (!var_check_ro(di->di_flags, (const char *)lp->ll_name, TV_CSTRING)
                        && !tv_check_lock(di->di_tv.v_lock, (const char *)lp->ll_name, TV_CSTRING)))
                   && eexe_mod_op(&tv, rettv, (const char *)op) == OK)
                {
                    set_var(lp->ll_name, lp->ll_name_len, &tv, false);
                }

                tv_clear(&tv);
            }
        }
        else
        {
            set_var(lp->ll_name, lp->ll_name_len, rettv, copy);
        }

        *endp = cc;
    }
    else if(tv_check_lock(lp->ll_newkey == NULL
                          ? lp->ll_tv->v_lock : lp->ll_tv->vval.v_dict->dv_lock,
                          (const char *)lp->ll_name,
                          TV_CSTRING))
    { /* empty body */ }
    else if(lp->ll_range)
    {
        listitem_st *ll_li = lp->ll_li;
        int ll_n1 = lp->ll_n1;

        // Check whether any of the list items is locked
        for(listitem_st *ri = rettv->vval.v_list->lv_first;
            ri != NULL && ll_li != NULL; /* none */)
        {
            if(tv_check_lock(ll_li->li_tv.v_lock,
                             (const char *)lp->ll_name, TV_CSTRING))
            {
                return;
            }

            ri = ri->li_next;

            if(ri == NULL || (!lp->ll_empty2 && lp->ll_n2 == ll_n1))
            {
                break;
            }

            ll_li = ll_li->li_next;
            ll_n1++;
        }

        // Assign the List values to the list items.
        for(ri = rettv->vval.v_list->lv_first; ri != NULL;)
        {
            if(op != NULL && *op != '=')
            {
                eexe_mod_op(&lp->ll_li->li_tv, &ri->li_tv, (const char *)op);
            }
            else
            {
                tv_clear(&lp->ll_li->li_tv);
                tv_copy(&ri->li_tv, &lp->ll_li->li_tv);
            }

            ri = ri->li_next;

            if(ri == NULL || (!lp->ll_empty2 && lp->ll_n2 == lp->ll_n1))
            {
                break;
            }

            if(lp->ll_li->li_next == NULL)
            {
                // Need to add an empty item.
                tv_list_append_number(lp->ll_list, 0);
                assert(lp->ll_li->li_next);
            }

            lp->ll_li = lp->ll_li->li_next;
            ++lp->ll_n1;
        }

        if(ri != NULL)
        {
            EMSG(_("E710: List value has more items than target"));
        }
        else if(lp->ll_empty2
                ? (lp->ll_li != NULL && lp->ll_li->li_next != NULL)
                : lp->ll_n1 != lp->ll_n2)
        {
            EMSG(_("E711: List value has not enough items"));
        }
    }
    else
    {
        typval_st oldtv = TV_INITIAL_VALUE;
        dict_st *dict = lp->ll_dict;
        bool watched = tv_dict_is_watched(dict);

        // Assign to a List or Dictionary item.
        if(lp->ll_newkey != NULL)
        {
            if(op != NULL && *op != '=')
            {
                EMSG2(_(e_letwrong), op);
                return;
            }

            // Need to add an item to the Dictionary.
            di = tv_dict_item_alloc((const char *)lp->ll_newkey);

            if(tv_dict_add(lp->ll_tv->vval.v_dict, di) == FAIL)
            {
                xfree(di);
                return;
            }

            lp->ll_tv = &di->di_tv;
        }
        else
        {
            if(watched)
            {
                tv_copy(lp->ll_tv, &oldtv);
            }

            if(op != NULL && *op != '=')
            {
                eexe_mod_op(lp->ll_tv, rettv, (const char *)op);
                goto notify;
            }
            else
            {
                tv_clear(lp->ll_tv);
            }
        }

        // Assign the value to the variable or list item.
        if(copy)
        {
            tv_copy(rettv, lp->ll_tv);
        }
        else
        {
            *lp->ll_tv = *rettv;
            lp->ll_tv->v_lock = 0;
            tv_init(rettv);
        }

notify:

        if(watched)
        {
            if(oldtv.v_type == kNvarUnknown)
            {
                tv_dict_watcher_notify(dict,
                                       (char *)lp->ll_newkey, lp->ll_tv, NULL);
            }
            else
            {
                dictitem_st *di = lp->ll_di;

                tv_dict_watcher_notify(dict,
                                       (char *)di->di_key, lp->ll_tv, &oldtv);

                tv_clear(&oldtv);
            }
        }
    }
}

/// Evaluate the expression used in a ":for var in expr" command.
/// "arg" points to "var".
/// Set "*errp" to TRUE for an error, FALSE otherwise;
/// Return a pointer that holds the info.  Null when there is an error.
///
/// @todo move to eval/ex_cmds
void *eval_for_line(const uchar_kt *arg, bool *errp, uchar_kt **nextcmdp, int skip)
{
    forinfo_st *fi = xcalloc(1, sizeof(forinfo_st));
    const uchar_kt *expr;
    typval_st tv;
    list_st *l;
    *errp = true; // Default: there is an error.
    expr = skip_var_list(arg, &fi->fi_varcount, &fi->fi_semicolon);

    if(expr == NULL)
    {
        return fi;
    }

    expr = skipwhite(expr);

    if(expr[0] != 'i' || expr[1] != 'n' || !ascii_iswhite(expr[2]))
    {
        EMSG(_("E690: Missing \"in\" after :for"));
        return fi;
    }

    if(skip)
    {
        ++emsg_skip;
    }

    if(eval_lev_0(skipwhite(expr + 2), &tv, nextcmdp, !skip) == OK)
    {
        *errp = false;

        if(!skip)
        {
            l = tv.vval.v_list;

            if(tv.v_type != kNvarList)
            {
                EMSG(_(e_listreq));
                tv_clear(&tv);
            }
            else if(l == NULL)
            {
                // a null list is like an empty list: do nothing
                tv_clear(&tv);
            }
            else
            {
                // No need to increment the refcount, it's already
                // set for the list being used in "tv".
                fi->fi_list = l;
                tv_list_watch_add(l, &fi->fi_lw);
                fi->fi_lw.lw_item = l->lv_first;
            }
        }
    }

    if(skip)
    {
        --emsg_skip;
    }

    return fi;
}

/// Use the first item in a ":for" list. Advance to the next.
/// Assign the values to the variable (list). "arg" points to the first one.
/// Return TRUE when a valid item was found, FALSE when at end of list or
/// something wrong.
///
/// @todo move to eval/ex_cmds
int next_for_item(void *fi_void, uchar_kt *arg)
{
    forinfo_st *fi = (forinfo_st *)fi_void;
    int result;
    listitem_st *item;
    item = fi->fi_lw.lw_item;

    if(item == NULL)
    {
        result = FALSE;
    }
    else
    {
        fi->fi_lw.lw_item = item->li_next;
        result = (ex_let_vars(arg,
                              &item->li_tv,
                              TRUE,
                              fi->fi_semicolon,
                              fi->fi_varcount,
                              NULL) == OK);
    }

    return result;
}

/// Free the structure used to store info used by ":for".
///
/// @todo move to eval/ex_cmds
void free_for_info(void *fi_void)
{
    forinfo_st *fi = (forinfo_st *)fi_void;

    if(fi != NULL && fi->fi_list != NULL)
    {
        tv_list_watch_remove(fi->fi_list, &fi->fi_lw);
        tv_list_unref(fi->fi_list);
    }

    xfree(fi);
}

void set_context_for_expression(expand_st *xp,
                                uchar_kt *arg,
                                excmd_idx_et cmdidx)
{
    int got_eq = FALSE;
    int c;
    uchar_kt *p;

    if(cmdidx == CMD_let)
    {
        xp->xp_context = EXPAND_USER_VARS;

        if(xstrpbrk(arg, (uchar_kt *)"\"'+-*/%.=!?~|&$([<>,#") == NULL)
        {
            // ":let var1 var2 ...": find last space.
            for(p = arg + ustrlen(arg); p >= arg;)
            {
                xp->xp_pattern = p;
                mb_ptr_back(arg, p);

                if(ascii_iswhite(*p))
                {
                    break;
                }
            }

            return;
        }
    }
    else
    {
        xp->xp_context = cmdidx == CMD_call
                                   ? EXPAND_FUNCTIONS
                                   : EXPAND_EXPRESSION;
    }

    xp->xp_pattern = xstrpbrk(arg, (uchar_kt *)"\"'+-*/%.=!?~|&$([<>,#");
    while(xp->xp_pattern != NULL)
    {
        c = *xp->xp_pattern;

        if(c == '&')
        {
            c = xp->xp_pattern[1];

            if(c == '&')
            {
                ++xp->xp_pattern;

                xp->xp_context = cmdidx != CMD_let
                                 || got_eq
                                    ? EXPAND_EXPRESSION : EXPAND_NOTHING;
            }
            else if(c != ' ')
            {
                xp->xp_context = EXPAND_SETTINGS;

                if((c == 'l' || c == 'g') && xp->xp_pattern[2] == ':')
                {
                    xp->xp_pattern += 2;
                }
            }
        }
        else if(c == '$')
        {
            // environment variable
            xp->xp_context = EXPAND_ENV_VARS;
        }
        else if(c == '=')
        {
            got_eq = TRUE;
            xp->xp_context = EXPAND_EXPRESSION;
        }
        else if(c == '#' && xp->xp_context == EXPAND_EXPRESSION)
        {
            // Autoload function/variable contains '#'
            break;
        }
        else if((c == '<' || c == '#')
                && xp->xp_context == EXPAND_FUNCTIONS
                && ustrchr(xp->xp_pattern, '(') == NULL)
        {
            // Function name can start with "<SNR>" and contain '#'.
            break;
        }
        else if(cmdidx != CMD_let || got_eq)
        {
            if(c == '"') // string
            {
                while((c = *++xp->xp_pattern) != NUL && c != '"')
                {
                    if(c == '\\' && xp->xp_pattern[1] != NUL)
                    {
                        ++xp->xp_pattern;
                    }
                }

                xp->xp_context = EXPAND_NOTHING;
            }
            else if(c == '\'') // literal string
            {
                // Trick: '' is like stopping and starting a literal string.
                while((c = *++xp->xp_pattern) != NUL && c != '\'')
                { /* empty body */ }

                xp->xp_context = EXPAND_NOTHING;
            }
            else if(c == '|')
            {
                if(xp->xp_pattern[1] == '|')
                {
                    ++xp->xp_pattern;
                    xp->xp_context = EXPAND_EXPRESSION;
                }
                else
                {
                    xp->xp_context = EXPAND_COMMANDS;
                }
            }
            else
            {
                xp->xp_context = EXPAND_EXPRESSION;
            }
        }
        else
        {
            // Doesn't look like something valid,
            // expand as an expression anyway.
            xp->xp_context = EXPAND_EXPRESSION;
        }

        arg = xp->xp_pattern;

        if(*arg != NUL)
        {
            while((c = *++arg) != NUL && (c == ' ' || c == '\t'))
            { /* empty body */ }
        }

        xp->xp_pattern = xstrpbrk(arg, (uchar_kt *)"\"'+-*/%.=!?~|&$([<>,#");
    }

    xp->xp_pattern = arg;
}

/// ":1,25call func(arg1, arg2)"  function call.
///
/// @todo move to eval/ex_cmds
void ex_call(exargs_st *eap)
{
    uchar_kt *arg = eap->arg;
    uchar_kt *startarg;
    uchar_kt *name;
    uchar_kt *tofree;
    int len;
    typval_st rettv;
    linenum_kt lnum;
    int doesrange;
    bool failed = false;
    funcdict_st fudi;
    partial_st *partial = NULL;

    if(eap->skip)
    {
        // trans_function_name() doesn't work well when skipping,
        // use eval_lev_0() instead to skip to any following command,
        // e.g. for:
        //   :if 0 | call dict.foo().bar() | endif.
        emsg_skip++;

        if(eval_lev_0(eap->arg, &rettv, &eap->nextcmd, false) != FAIL)
        {
            tv_clear(&rettv);
        }

        emsg_skip--;
        return;
    }

    tofree = trans_function_name(&arg, eap->skip, TFN_INT, &fudi, &partial);

    if(fudi.fd_newkey != NULL)
    {
        // Still need to give an error message for missing key.
        EMSG2(_(e_dictkey), fudi.fd_newkey);
        xfree(fudi.fd_newkey);
    }

    if(tofree == NULL)
    {
        return;
    }

    // Increase refcount on dictionary,
    // it could get deleted when evaluating the arguments.
    if(fudi.fd_dict != NULL)
    {
        ++fudi.fd_dict->dv_refcount;
    }

    // If it is the name of a variable of type kNvarUfunc or kNvarPartial use its
    // contents. For kNvarPartial get its partial, unless we already have one
    // from trans_function_name().
    len = (int)ustrlen(tofree);

    name = deref_func_name((const char *)tofree, &len,
                           partial != NULL ? NULL : &partial, false);

    // Skip white space to allow ":call func ()".
    // Not good, but required for backward compatibility.
    startarg = skipwhite(arg);
    rettv.v_type = kNvarUnknown; // tv_clear() uses this.

    if(*startarg != '(')
    {
        EMSG2(_("E107: Missing parentheses: %s"), eap->arg);
        goto end;
    }

    // When skipping, evaluate the function once,
    // to find the end of the arguments. When the
    // function takes a range, this is  discovered
    // after the first call, and the loop is broken.
    if(eap->skip)
    {
        emsg_skip++;
        lnum = eap->line2; // Do it once, also with an invalid range.
    }
    else
    {
        lnum = eap->line1;
    }

    for(; lnum <= eap->line2; lnum++)
    {
        if(!eap->skip && eap->addr_count > 0)
        {
            curwin->w_cursor.lnum = lnum;
            curwin->w_cursor.col = 0;
            curwin->w_cursor.coladd = 0;
        }

        arg = startarg;

        if(get_func_tv(name,
                       (int)ustrlen(name),
                       &rettv,
                       &arg,
                       eap->line1,
                       eap->line2,
                       &doesrange,
                       !eap->skip,
                       partial,
                       fudi.fd_dict) == FAIL)
        {
            failed = true;
            break;
        }

        // Handle a function returning a Funcref, Dictionary or List.
        if(handle_subscript((const char **)&arg, &rettv,
                            !eap->skip, true) == FAIL)
        {
            failed = true;
            break;
        }

        tv_clear(&rettv);

        if(doesrange || eap->skip)
        {
            break;
        }

        // Stop when immediately aborting on error, or when an interrupt
        // occurred or an exception was thrown but not caught.
        // get_func_tv() returned OK, so that the check for trailing
        // characters below is executed.
        if(aborting())
        {
            break;
        }
    }

    if(eap->skip)
    {
        --emsg_skip;
    }

    if(!failed)
    {
        // Check for trailing illegal characters and a following command.
        if(!ends_excmd(*arg))
        {
            emsg_severe = TRUE;
            EMSG(_(e_trailing));
        }
        else
        {
            eap->nextcmd = check_nextcmd(arg);
        }
    }

end:
    tv_dict_unref(fudi.fd_dict);
    xfree(tofree);
}

///":unlet[!] var1 ... " command.
///
/// @todo move to eval/ex_cmds
void ex_unlet(exargs_st *eap)
{
    ex_unletlock(eap, eap->arg, 0);
}

/// ":lockvar" and ":unlockvar" commands
///
/// @todo move to eval/ex_cmds
void ex_lockvar(exargs_st *eap)
{
    uchar_kt *arg = eap->arg;
    int deep = 2;

    if(eap->forceit)
    {
        deep = -1;
    }
    else if(ascii_isdigit(*arg))
    {
        deep = getdigits_int(&arg);
        arg = skipwhite(arg);
    }

    ex_unletlock(eap, arg, deep);
}

/// ":unlet", ":lockvar" and ":unlockvar" are quite similar.
///
/// @todo move to eval/ex_cmds
static void ex_unletlock(exargs_st *eap, uchar_kt *argstart, int deep)
{
    uchar_kt *arg = argstart;
    bool error = false;
    leftval_st lv;

    do
    {
        // Parse the name and find the end.
        uchar_kt *const name_end = (uchar_kt *)get_lval(arg,
                                                    NULL,
                                                    &lv,
                                                    true,
                                                    eap->skip || error,
                                                    0,
                                                    FNE_CHECK_START);

        if(lv.ll_name == NULL)
        {
            error = true; // error, but continue parsing.
        }

        if(name_end == NULL
           || (!ascii_iswhite(*name_end) && !ends_excmd(*name_end)))
        {
            if(name_end != NULL)
            {
                emsg_severe = TRUE;
                EMSG(_(e_trailing));
            }

            if(!(eap->skip || error))
            {
                clear_lval(&lv);
            }

            break;
        }

        if(!error && !eap->skip)
        {
            if(eap->cmdidx == CMD_unlet)
            {
                if(do_unlet_var(&lv, name_end, eap->forceit) == FAIL)
                {
                    error = TRUE;
                }
            }
            else
            {
                if(do_lock_var(&lv, name_end, deep,
                               eap->cmdidx == CMD_lockvar) == FAIL)
                {
                    error = true;
                }
            }
        }

        if(!eap->skip)
        {
            clear_lval(&lv);
        }

        arg = skipwhite(name_end);
    } while(!ends_excmd(*arg));

    eap->nextcmd = check_nextcmd(arg);
}

/// @todo move to eval/ex_cmds
static int do_unlet_var(leftval_st *const lp, uchar_kt *const name_end, int forceit)
{
    int ret = OK;
    int cc;

    if(lp->ll_tv == NULL)
    {
        cc = *name_end;
        *name_end = NUL;

        // Normal name or expanded name.
        if(do_unlet(lp->ll_name, lp->ll_name_len, forceit) == FAIL)
        {
            ret = FAIL;
        }

        *name_end = cc;
    }
    else if((lp->ll_list != NULL && tv_check_lock(lp->ll_list->lv_lock,
                                                  (const char *)lp->ll_name,
                                                  lp->ll_name_len))
            || (lp->ll_dict != NULL && tv_check_lock(lp->ll_dict->dv_lock,
                                                     (const char *)lp->ll_name,
                                                     lp->ll_name_len)))
    {
        return FAIL;
    }
    else if(lp->ll_range)
    {
        listitem_st *li;
        listitem_st *ll_li = lp->ll_li;
        int ll_n1 = lp->ll_n1;

        while(ll_li != NULL && (lp->ll_empty2 || lp->ll_n2 >= ll_n1))
        {
            li = ll_li->li_next;

            if(tv_check_lock(ll_li->li_tv.v_lock,
                             (const char *)lp->ll_name,
                             lp->ll_name_len))
            {
                return false;
            }

            ll_li = li;
            ll_n1++;
        }

        // Delete a range of List items.
        while(lp->ll_li != NULL && (lp->ll_empty2 || lp->ll_n2 >= lp->ll_n1))
        {
            li = lp->ll_li->li_next;
            tv_list_item_remove(lp->ll_list, lp->ll_li);
            lp->ll_li = li;
            ++lp->ll_n1;
        }
    }
    else
    {
        if(lp->ll_list != NULL)
        {
            // unlet a List item.
            tv_list_item_remove(lp->ll_list, lp->ll_li);
        }
        else
        {
            // unlet a Dictionary item.
            dict_st *d = lp->ll_dict;
            dictitem_st *di = lp->ll_di;
            bool watched = tv_dict_is_watched(d);
            char *key = NULL;
            typval_st oldtv;

            if(watched)
            {
                tv_copy(&di->di_tv, &oldtv);

                // need to save key because dictitem_remove will free it
                key = xstrdup((char *)di->di_key);
            }

            tv_dict_item_remove(d, di);

            if(watched)
            {
                tv_dict_watcher_notify(d, key, NULL, &oldtv);
                tv_clear(&oldtv);
                xfree(key);
            }
        }
    }

    return ret;
}

/// unlet a variable
///
/// @param[in]  name      Variable name to unlet.
/// @param[in]  name_len  Variable name length.
/// @param[in]  fonceit   If true, do not complain if variable doesn’t exist.
///
/// @return OK if it existed, FAIL otherwise.
///
/// @todo move to eval/ex_cmds
int do_unlet(const char *const name, const size_t name_len, const int forceit)
FUNC_ATTR_NONNULL_ALL
{
    const char *varname;
    dict_st *dict;
    hashtable_st *ht = find_var_ht_dict(name, name_len, &varname, &dict);

    if(ht != NULL && *varname != NUL)
    {
        dict_st *d;

        if(ht == &globvarht)
        {
            d = &globvardict;
        }
        else if(current_funccal != NULL
                && ht == &current_funccal->l_vars.dv_hashtab)
        {
            d = &current_funccal->l_vars;
        }
        else if(ht == &compat_hashtab)
        {
            d = &vimvardict;
        }
        else
        {
            dictitem_st *const di = find_var_in_ht(ht, *name, "", 0, false);
            d = di->di_tv.vval.v_dict;
        }

        if(d == NULL)
        {
            EMSG2(_(e_intern2), "do_unlet()");
            return FAIL;
        }

        hashitem_st *hi = hash_find(ht, (const uchar_kt *)varname);

        if(HASHITEM_EMPTY(hi))
        {
            hi = find_hi_in_scoped_ht((const char *)name, &ht);
        }

        if(hi != NULL && !HASHITEM_EMPTY(hi))
        {
            dictitem_st *const di = TV_DICT_HI2DI(hi);

            if(var_check_fixed(di->di_flags, (const char *)name, TV_CSTRING)
               || var_check_ro(di->di_flags, (const char *)name, TV_CSTRING)
               || tv_check_lock(d->dv_lock, (const char *)name, TV_CSTRING))
            {
                return FAIL;
            }

            if(tv_check_lock(d->dv_lock, (const char *)name, TV_CSTRING))
            {
                return FAIL;
            }

            typval_st oldtv;
            bool watched = tv_dict_is_watched(dict);

            if(watched)
            {
                tv_copy(&di->di_tv, &oldtv);
            }

            delete_var(ht, hi);

            if(watched)
            {
                tv_dict_watcher_notify(dict, varname, NULL, &oldtv);
                tv_clear(&oldtv);
            }

            return OK;
        }
    }

    if(forceit)
    {
        return OK;
    }

    EMSG2(_("E108: No such variable: \"%s\""), name);
    return FAIL;
}

/// Lock or unlock variable indicated by "lp".
/// - "deep" is the levels to go (-1 for unlimited);
/// - "lock" is TRUE for ":lockvar", FALSE for ":unlockvar".
///
/// @todo move to eval/ex_cmds
static int do_lock_var(leftval_st *lp,
                       uchar_kt *const FUNC_ARGS_UNUSED_MATCH(name_end),
                       const int deep,
                       const bool lock)
{
    int ret = OK;

    if(deep == 0) // Nothing to do.
    {
        return OK;
    }

    if(lp->ll_tv == NULL)
    {
        // Normal name or expanded name.
        dictitem_st *const di = find_var((const char *)lp->ll_name,
                                        lp->ll_name_len, NULL, true);

        if(di == NULL)
        {
            ret = FAIL;
        }
        else if((di->di_flags & DI_FLAGS_FIX)
                && di->di_tv.v_type != kNvarDict
                && di->di_tv.v_type != kNvarList)
        {
            // For historical reasons this error is not given for Lists and
            // Dictionaries. E.g. b: dictionary may be locked/unlocked.
            emsgf(_("E940: Cannot lock or unlock variable %s"), lp->ll_name);
        }
        else
        {
            if(lock)
            {
                di->di_flags |= DI_FLAGS_LOCK;
            }
            else
            {
                di->di_flags &= ~DI_FLAGS_LOCK;
            }

            tv_item_lock(&di->di_tv, deep, lock);
        }
    }
    else if(lp->ll_range)
    {
        listitem_st *li = lp->ll_li;

        // (un)lock a range of List items.
        while(li != NULL && (lp->ll_empty2 || lp->ll_n2 >= lp->ll_n1))
        {
            tv_item_lock(&li->li_tv, deep, lock);
            li = li->li_next;
            ++lp->ll_n1;
        }
    }
    else if(lp->ll_list != NULL)
    {
        // (un)lock a List item.
        tv_item_lock(&lp->ll_li->li_tv, deep, lock);
    }
    else
    {
        // (un)lock a Dictionary item.
        tv_item_lock(&lp->ll_di->di_tv, deep, lock);
    }

    return ret;
}

/// Delete all "menutrans_" variables.
void del_menutrans_vars(void)
{
    hash_lock(&globvarht);

    HASHTAB_ITER(&globvarht, hi, {
        if(ustrncmp(hi->hi_key, "menutrans_", 10) == 0)
        {
            delete_var(&globvarht, hi);
        }
    });

    hash_unlock(&globvarht);
}

// Local string buffer for the next two functions to store a variable name
// with its prefix. Allocated in cat_prefix_varname(), freed later in
// get_user_var_name().
static uchar_kt  *varnamebuf = NULL;
static size_t varnamebuflen = 0;

/// Function to concatenate a prefix and a variable name.
static uchar_kt *cat_prefix_varname(int prefix, uchar_kt *name)
{
    size_t len = ustrlen(name) + 3;

    if(len > varnamebuflen)
    {
        xfree(varnamebuf);
        len += 10; // some additional space
        varnamebuf = xmalloc(len);
        varnamebuflen = len;
    }

    *varnamebuf = prefix;
    varnamebuf[1] = ':';
    ustrcpy(varnamebuf + 2, name);
    return varnamebuf;
}

/// Function given to ExpandGeneric() to obtain the list of user defined
/// (global/buffer/window/built-in) variable names.
uchar_kt *get_user_var_name(expand_st *xp, int idx)
{
    static size_t gdone;
    static size_t bdone;
    static size_t wdone;
    static size_t tdone;
    static size_t vidx;
    static hashitem_st *hi;
    hashtable_st *ht;

    if(idx == 0)
    {
        gdone = bdone = wdone = vidx = 0;
        tdone = 0;
    }

    // Global variables
    if(gdone < globvarht.ht_used)
    {
        if(gdone++ == 0)
        {
            hi = globvarht.ht_array;
        }
        else
        {
            ++hi;
        }

        while(HASHITEM_EMPTY(hi))
        {
            ++hi;
        }

        if(ustrncmp("g:", xp->xp_pattern, 2) == 0)
        {
            return cat_prefix_varname('g', hi->hi_key);
        }

        return hi->hi_key;
    }

    // b: variables
    ht = &curbuf->b_vars->dv_hashtab;

    if(bdone < ht->ht_used)
    {
        if(bdone++ == 0)
        {
            hi = ht->ht_array;
        }
        else
        {
            ++hi;
        }

        while(HASHITEM_EMPTY(hi))
        {
            ++hi;
        }

        return cat_prefix_varname('b', hi->hi_key);
    }

    // w: variables
    ht = &curwin->w_vars->dv_hashtab;

    if(wdone < ht->ht_used)
    {
        if(wdone++ == 0)
        {
            hi = ht->ht_array;
        }
        else
        {
            ++hi;
        }

        while(HASHITEM_EMPTY(hi))
        {
            ++hi;
        }

        return cat_prefix_varname('w', hi->hi_key);
    }

    // t: variables
    ht = &curtab->tp_vars->dv_hashtab;

    if(tdone < ht->ht_used)
    {
        if(tdone++ == 0)
        {
            hi = ht->ht_array;
        }
        else
        {
            ++hi;
        }

        while(HASHITEM_EMPTY(hi))
        {
            ++hi;
        }

        return cat_prefix_varname('t', hi->hi_key);
    }

    // v: variables
    if(vidx < ARRAY_SIZE(vimvars))
    {
        return cat_prefix_varname('v', (uchar_kt *)vimvars[vidx++].vv_name);
    }

    xfree(varnamebuf);
    varnamebuf = NULL;
    varnamebuflen = 0;
    return NULL;
}

/// Return TRUE if "pat" matches "text".
/// Does not use 'cpo' and always uses 'magic'.
///
/// @todo move to eval/expressions
static int pattern_match(uchar_kt *pat, uchar_kt *text, int ic)
{
    int matches = 0;
    regmatch_st regmatch;

    // avoid 'l' flag in 'cpoptions'
    uchar_kt *save_cpo = p_cpo;
    p_cpo = (uchar_kt *)"";
    regmatch.regprog = regexp_compile(pat, RE_MAGIC + RE_STRING);

    if(regmatch.regprog != NULL)
    {
        regmatch.rm_ic = ic;
        matches = vim_regexec_nl(&regmatch, text, (columnum_kt)0);
        vim_regfree(regmatch.regprog);
    }

    p_cpo = save_cpo;
    return matches;
}

/// types for expressions.
typedef enum
{
    TYPE_UNKNOWN = 0, ///<  unknown
    TYPE_EQUAL,       ///<  ==
    TYPE_NEQUAL,      ///<  !=
    TYPE_GREATER,     ///<  >
    TYPE_GEQUAL,      ///<  >=
    TYPE_SMALLER,     ///<  <
    TYPE_SEQUAL,      ///<  <=
    TYPE_MATCH,       ///<  =~
    TYPE_NOMATCH,     ///<  !~
} exptype_T;

// The @b evaluate argument: When FALSE, the argument is only
// parsed but not  executed. The function may return OK, but the
// rettv will be of type kNvarUnknown.
// The function still returns FAIL for a syntax error.

/// Handle zero level expression.
/// This calls eval_lev_1() and handles error message and nextcmd.
/// Put the result in "rettv" when returning OK and @b evaluate is TRUE.
///
/// @note
/// @b rettv.v_lock is not set
///
/// Return OK or FAIL.
/// @todo move to eval/expressions
int eval_lev_0(uchar_kt *arg, typval_st *rettv, uchar_kt **nextcmd, int evaluate)
{
    int ret;
    uchar_kt *p;
    p = skipwhite(arg);
    ret = eval_lev_1(&p, rettv, evaluate);

    if(ret == FAIL || !ends_excmd(*p))
    {
        if(ret != FAIL)
        {
            tv_clear(rettv);
        }

        // Report the invalid expression unless the expression
        // evaluation has been cancelled due to an aborting error,
        // an interrupt, or an exception.
        if(!aborting())
        {
            emsgf(_(e_invexpr2), arg);
        }

        ret = FAIL;
    }

    if(nextcmd != NULL)
    {
        *nextcmd = check_nextcmd(p);
    }

    return ret;
}

/// Handle top level expression: "expr1 ? expr2 : expr3"
///
/// @note
/// - @b arg must point to the first non-white of the expression.
/// - @b arg is advanced to the next non-white after the recognized expression.
/// - @b rettv.v_lock is not set
///
/// @return OK or FAIL
///
/// @todo move to eval/expressions
static int eval_lev_1(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    int result;
    typval_st var2;

    // Get the first variable.
    if(eval_lev_2(arg, rettv, evaluate) == FAIL)
    {
        return FAIL;
    }

    if((*arg)[0] == '?')
    {
        result = FALSE;

        if(evaluate)
        {
            bool error = false;

            if(tv_get_number_chk(rettv, &error) != 0)
            {
                result = true;
            }

            tv_clear(rettv);

            if(error)
            {
                return FAIL;
            }
        }

        // Get the second variable.
        *arg = skipwhite(*arg + 1);

        if(eval_lev_1(arg, rettv, evaluate && result) == FAIL)
        {
            return FAIL;  // recursive!
        }

        // Check for the ":".
        if((*arg)[0] != ':')
        {
            emsgf(_("E109: Missing ':' after '?'"));

            if(evaluate && result)
            {
                tv_clear(rettv);
            }

            return FAIL;
        }

        // Get the third variable.
        *arg = skipwhite(*arg + 1);

        if(eval_lev_1(arg, &var2, evaluate && !result) == FAIL) // Recursive!
        {
            if(evaluate && result)
            {
                tv_clear(rettv);
            }

            return FAIL;
        }

        if(evaluate && !result)
        {
            *rettv = var2;
        }
    }

    return OK;
}

/// Handle first level expression: "expr2 || expr2 || expr2", the logical "OR"
///
/// - @b arg must point to the first non-white of the expression.
/// - @b arg is advanced to the next non-white after the recognized expression.
///
/// @return OK or FAIL
///
/// @todo move to eval/expressions
static int eval_lev_2(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    typval_st var2;
    long result;
    int first;
    bool error = false;

    // Get the first variable.
    if(eval_lev_3(arg, rettv, evaluate) == FAIL)
    {
        return FAIL;
    }

    // Repeat until there is no following "||".
    first = TRUE;
    result = FALSE;

    while((*arg)[0] == '|' && (*arg)[1] == '|')
    {
        if(evaluate && first)
        {
            if(tv_get_number_chk(rettv, &error) != 0)
            {
                result = true;
            }

            tv_clear(rettv);

            if(error)
            {
                return FAIL;
            }

            first = false;
        }

        // Get the second variable.
        *arg = skipwhite(*arg + 2);

        if(eval_lev_3(arg, &var2, evaluate && !result) == FAIL)
        {
            return FAIL;
        }

        // Compute the result.
        if(evaluate && !result)
        {
            if(tv_get_number_chk(&var2, &error) != 0)
            {
                result = true;
            }

            tv_clear(&var2);

            if(error)
            {
                return FAIL;
            }
        }

        if(evaluate)
        {
            rettv->v_type = kNvarNumber;
            rettv->vval.v_number = result;
        }
    }

    return OK;
}

/// Handle second level expression:
/// "expr3 && expr3 && expr3", the logical "AND"
///
/// - @b arg must point to the first non-white of the expression.
/// - @b arg is advanced to the next non-white after the recognized expression.
///
/// @return OK or FAIL
///
/// @todo move to eval/expressions
static int eval_lev_3(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    typval_st var2;
    long result;
    int first;
    bool error = false;

    // Get the first variable.
    if(eval_lev_4(arg, rettv, evaluate) == FAIL)
    {
        return FAIL;
    }

    // Repeat until there is no following "&&".
    first = TRUE;
    result = TRUE;

    while((*arg)[0] == '&' && (*arg)[1] == '&')
    {
        if(evaluate && first)
        {
            if(tv_get_number_chk(rettv, &error) == 0)
            {
                result = false;
            }

            tv_clear(rettv);

            if(error)
            {
                return FAIL;
            }

            first = false;
        }

        // Get the second variable.
        *arg = skipwhite(*arg + 2);

        if(eval_lev_4(arg, &var2, evaluate && result) == FAIL)
        {
            return FAIL;
        }

        // Compute the result.
        if(evaluate && result)
        {
            if(tv_get_number_chk(&var2, &error) == 0)
            {
                result = false;
            }

            tv_clear(&var2);

            if(error)
            {
                return FAIL;
            }
        }

        if(evaluate)
        {
            rettv->v_type = kNvarNumber;
            rettv->vval.v_number = result;
        }
    }

    return OK;
}

/// Handle third level expression:
/// - var1 == var2
/// - var1 =~ var2
/// - var1 != var2
/// - var1 !~ var2
/// - var1 > var2
/// - var1 >= var2
/// - var1 < var2
/// - var1 <= var2
/// - var1 is var2
/// - var1 isnot var2
///
/// @note
/// - @b arg must point to the first non-white of the expression.
/// - @b arg is advanced to the next non-white after the recognized expression.
///
/// @return OK or FAIL
///
/// @todo move to eval/expressions
static int eval_lev_4(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    typval_st var2;
    uchar_kt *p;
    int i;
    exptype_T type = TYPE_UNKNOWN;
    int type_is = FALSE; // TRUE for "is" and "isnot"
    int len = 2;
    long n1, n2;
    int ic;

    // Get the first variable.
    if(eval_lev_5(arg, rettv, evaluate) == FAIL)
    {
        return FAIL;
    }

    p = *arg;

    switch(p[0])
    {
        case '=':
            if(p[1] == '=')
            {
                type = TYPE_EQUAL;
            }
            else if(p[1] == '~')
            {
                type = TYPE_MATCH;
            }

            break;

        case '!':
            if(p[1] == '=')
            {
                type = TYPE_NEQUAL;
            }
            else if(p[1] == '~')
            {
                type = TYPE_NOMATCH;
            }

            break;

        case '>':
            if(p[1] != '=')
            {
                type = TYPE_GREATER;
                len = 1;
            }
            else
            {
                type = TYPE_GEQUAL;
            }

            break;

        case '<':
            if(p[1] != '=')
            {
                type = TYPE_SMALLER;
                len = 1;
            }
            else
            {
                type = TYPE_SEQUAL;
            }

            break;

        case 'i':
            if(p[1] == 's')
            {
                if(p[2] == 'n' && p[3] == 'o' && p[4] == 't')
                {
                    len = 5;
                }

                if(!isalnum(p[len]) && p[len] != '_')
                {
                    type = len == 2 ? TYPE_EQUAL : TYPE_NEQUAL;
                    type_is = TRUE;
                }
            }

            break;
    }

    // If there is a comparative operator, use it.
    if(type != TYPE_UNKNOWN)
    {
        // extra question mark appended: ignore case
        if(p[len] == '?')
        {
            ic = TRUE;
            ++len;
        }
        // extra '#' appended: match case
        else if(p[len] == '#')
        {
            ic = FALSE;
            ++len;
        }
        // nothing appended: use 'ignorecase'
        else
        {
            ic = p_ic;
        }

        // Get the second variable.
        *arg = skipwhite(p + len);

        if(eval_lev_5(arg, &var2, evaluate) == FAIL)
        {
            tv_clear(rettv);
            return FAIL;
        }

        if(evaluate)
        {
            if(type_is && rettv->v_type != var2.v_type)
            {
                // For "is" a different type always means FALSE,
                // for "notis" it means TRUE.
                n1 = (type == TYPE_NEQUAL);
            }
            else if(rettv->v_type == kNvarList || var2.v_type == kNvarList)
            {
                if(type_is)
                {
                    n1 = (rettv->v_type == var2.v_type
                          && rettv->vval.v_list == var2.vval.v_list);

                    if(type == TYPE_NEQUAL)
                    {
                        n1 = !n1;
                    }
                }
                else if(rettv->v_type != var2.v_type
                        || (type != TYPE_EQUAL && type != TYPE_NEQUAL))
                {
                    if(rettv->v_type != var2.v_type)
                    {
                        EMSG(_("E691: Can only compare List with List"));
                    }
                    else
                    {
                        EMSG(_("E692: Invalid operation for List"));
                    }

                    tv_clear(rettv);
                    tv_clear(&var2);
                    return FAIL;
                }
                else
                {
                    // Compare two Lists for being equal or unequal.
                    n1 = tv_list_equal(rettv->vval.v_list,
                                       var2.vval.v_list, ic, false);

                    if(type == TYPE_NEQUAL)
                    {
                        n1 = !n1;
                    }
                }
            }
            else if(rettv->v_type == kNvarDict || var2.v_type == kNvarDict)
            {
                if(type_is)
                {
                    n1 = (rettv->v_type == var2.v_type
                          && rettv->vval.v_dict == var2.vval.v_dict);

                    if(type == TYPE_NEQUAL)
                    {
                        n1 = !n1;
                    }
                }
                else if(rettv->v_type != var2.v_type
                        || (type != TYPE_EQUAL && type != TYPE_NEQUAL))
                {
                    if(rettv->v_type != var2.v_type)
                    {
                        EMSG(_("E735: Can only compare Dictionary with Dictionary"));
                    }
                    else
                    {
                        EMSG(_("E736: Invalid operation for Dictionary"));
                    }

                    tv_clear(rettv);
                    tv_clear(&var2);
                    return FAIL;
                }
                else
                {
                    // Compare two Dictionaries for being equal or unequal.
                    n1 = tv_dict_equal(rettv->vval.v_dict,
                                       var2.vval.v_dict, ic, false);

                    if(type == TYPE_NEQUAL)
                    {
                        n1 = !n1;
                    }
                }
            }
            else if(tv_is_func(*rettv) || tv_is_func(var2))
            {
                if(type != TYPE_EQUAL && type != TYPE_NEQUAL)
                {
                    EMSG(_("E694: Invalid operation for Funcrefs"));
                    tv_clear(rettv);
                    tv_clear(&var2);
                    return FAIL;
                }

                if((rettv->v_type == kNvarPartial && rettv->vval.v_partial == NULL)
                   || (var2.v_type == kNvarPartial && var2.vval.v_partial == NULL))
                {
                    // when a partial is NULL assume not equal
                    n1 = false;
                }
                else if(type_is)
                {
                    if(rettv->v_type == kNvarUfunc
                       && var2.v_type == kNvarUfunc)
                    {
                        // strings are considered the
                        // same if their value is the same
                        n1 = tv_equal(rettv, &var2, ic, false);
                    }
                    else if(rettv->v_type == kNvarPartial
                            && var2.v_type == kNvarPartial)
                    {
                        n1 = (rettv->vval.v_partial == var2.vval.v_partial);
                    }
                    else
                    {
                        n1 = false;
                    }
                }
                else
                {
                    n1 = tv_equal(rettv, &var2, ic, false);
                }

                if(type == TYPE_NEQUAL)
                {
                    n1 = !n1;
                }
            }
            // If one of the two variables is a float, compare as a float.
            // When using "=~" or "!~", always compare as string.
            else if((rettv->v_type == kNvarFloat || var2.v_type == kNvarFloat)
                    && type != TYPE_MATCH
                    && type != TYPE_NOMATCH)
            {
                float_kt f1;
                float_kt f2;

                if(rettv->v_type == kNvarFloat)
                {
                    f1 = rettv->vval.v_float;
                }
                else
                {
                    f1 = tv_get_number(rettv);
                }

                if(var2.v_type == kNvarFloat)
                {
                    f2 = var2.vval.v_float;
                }
                else
                {
                    f2 = tv_get_number(&var2);
                }

                n1 = false;

                switch(type)
                {
                    case TYPE_EQUAL:
                        n1 = (f1 == f2);
                        break;

                    case TYPE_NEQUAL:
                        n1 = (f1 != f2);
                        break;

                    case TYPE_GREATER:
                        n1 = (f1 > f2);
                        break;

                    case TYPE_GEQUAL:
                        n1 = (f1 >= f2);
                        break;

                    case TYPE_SMALLER:
                        n1 = (f1 < f2);
                        break;

                    case TYPE_SEQUAL:
                        n1 = (f1 <= f2);
                        break;

                    case TYPE_UNKNOWN:
                    case TYPE_MATCH:
                    case TYPE_NOMATCH:
                        break;
                }
            }
            // If one of the two variables is a number, compare as a number.
            // When using "=~" or "!~", always compare as string.
            else if((rettv->v_type == kNvarNumber || var2.v_type == kNvarNumber)
                    && type != TYPE_MATCH
                    && type != TYPE_NOMATCH)
            {
                n1 = tv_get_number(rettv);
                n2 = tv_get_number(&var2);

                switch(type)
                {
                    case TYPE_EQUAL:
                        n1 = (n1 == n2);
                        break;

                    case TYPE_NEQUAL:
                        n1 = (n1 != n2);
                        break;

                    case TYPE_GREATER:
                        n1 = (n1 > n2);
                        break;

                    case TYPE_GEQUAL:
                        n1 = (n1 >= n2);
                        break;

                    case TYPE_SMALLER:
                        n1 = (n1 < n2);
                        break;

                    case TYPE_SEQUAL:
                        n1 = (n1 <= n2);
                        break;

                    case TYPE_UNKNOWN:
                    case TYPE_MATCH:
                    case TYPE_NOMATCH:
                        break;
                }
            }
            else
            {
                char buf1[NUMBUFLEN];
                char buf2[NUMBUFLEN];
                const char *const s1 = tv_get_string_buf(rettv, buf1);
                const char *const s2 = tv_get_string_buf(&var2, buf2);

                if(type != TYPE_MATCH && type != TYPE_NOMATCH)
                {
                    i = mb_strcmp_ic((bool)ic, s1, s2);
                }
                else
                {
                    i = 0;
                }

                n1 = false;

                switch(type)
                {
                    case TYPE_EQUAL:
                        n1 = (i == 0);
                        break;

                    case TYPE_NEQUAL:
                        n1 = (i != 0);
                        break;

                    case TYPE_GREATER:
                        n1 = (i > 0);
                        break;

                    case TYPE_GEQUAL:
                        n1 = (i >= 0);
                        break;

                    case TYPE_SMALLER:
                        n1 = (i < 0);
                        break;

                    case TYPE_SEQUAL:
                        n1 = (i <= 0);
                        break;

                    case TYPE_MATCH:
                    case TYPE_NOMATCH:
                    {
                        n1 = pattern_match((uchar_kt *)s2, (uchar_kt *)s1, ic);

                        if(type == TYPE_NOMATCH)
                        {
                            n1 = !n1;
                        }

                        break;
                    }

                    case TYPE_UNKNOWN:
                        break;  // Avoid gcc warning.
                }
            }

            tv_clear(rettv);
            tv_clear(&var2);
            rettv->v_type = kNvarNumber;
            rettv->vval.v_number = n1;
        }
    }

    return OK;
}

/// Handle fourth level expression:
///
/// - + number addition
/// - - number subtraction
/// - . string concatenation
///
/// @note
/// - @b arg must point to the first non-white of the expression.
/// - @b arg is advanced to the next non-white after the recognized expression.
///
/// @return OK or FAIL
///
/// @todo move to eval/expressions
static int eval_lev_5(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    typval_st var2;
    typval_st var3;
    int op;
    long n1, n2;
    float_kt f1 = 0, f2 = 0;
    uchar_kt *p;

    // Get the first variable.
    if(eval_lev_6(arg, rettv, evaluate, FALSE) == FAIL)
    {
        return FAIL;
    }

    // Repeat computing, until no '+', '-' or '.' is following.
    for(;;)
    {
        op = **arg;

        if(op != '+' && op != '-' && op != '.')
        {
            break;
        }

        if((op != '+' || rettv->v_type != kNvarList)
           && (op == '.' || rettv->v_type != kNvarFloat))
        {
            // For "list + ...", an illegal use of the first operand as
            // a number cannot be determined before evaluating the 2nd
            // operand: if this is also a list, all is ok.
            // For "something . ...", "something - ..." or "non-list + ...",
            // we know that the first operand needs to be a string or number
            // without evaluating the 2nd operand.  So check before to avoid
            // side effects after an error.
            if(evaluate && !tv_check_str(rettv))
            {
                tv_clear(rettv);
                return FAIL;
            }
        }

        // Get the second variable.
        *arg = skipwhite(*arg + 1);

        if(eval_lev_6(arg, &var2, evaluate, op == '.') == FAIL)
        {
            tv_clear(rettv);
            return FAIL;
        }

        if(evaluate)
        {
            // Compute the result.
            if(op == '.')
            {
                char buf1[NUMBUFLEN];
                char buf2[NUMBUFLEN];
                // s1 already checked
                const char *const s1 = tv_get_string_buf(rettv, buf1);
                const char *const s2 = tv_get_string_buf_chk(&var2, buf2);

                if(s2 == NULL) // Type error?
                {
                    tv_clear(rettv);
                    tv_clear(&var2);
                    return FAIL;
                }

                p = (uchar_kt *)xstrdup_concat(s1, s2);
                tv_clear(rettv);
                rettv->v_type = kNvarString;
                rettv->vval.v_string = p;
            }
            else if(op == '+'
                    && rettv->v_type == kNvarList
                    && var2.v_type == kNvarList)
            {
                // Concatenate Lists.
                if(tv_list_concat(rettv->vval.v_list, var2.vval.v_list, &var3) == FAIL)
                {
                    tv_clear(rettv);
                    tv_clear(&var2);
                    return FAIL;
                }

                tv_clear(rettv);
                *rettv = var3;
            }
            else
            {
                bool error = false;

                if(rettv->v_type == kNvarFloat)
                {
                    f1 = rettv->vval.v_float;
                    n1 = 0;
                }
                else
                {
                    n1 = tv_get_number_chk(rettv, &error);

                    if(error)
                    {
                        // This can only happen for "list + non-list".  For
                        // "non-list + ..." or "something - ...", we returned
                        // before evaluating the 2nd operand.
                        tv_clear(rettv);
                        return FAIL;
                    }

                    if(var2.v_type == kNvarFloat)
                    {
                        f1 = n1;
                    }
                }

                if(var2.v_type == kNvarFloat)
                {
                    f2 = var2.vval.v_float;
                    n2 = 0;
                }
                else
                {
                    n2 = tv_get_number_chk(&var2, &error);

                    if(error)
                    {
                        tv_clear(rettv);
                        tv_clear(&var2);
                        return FAIL;
                    }

                    if(rettv->v_type == kNvarFloat)
                    {
                        f2 = n2;
                    }
                }

                tv_clear(rettv);

                // If there is a float on either side the result is a float.
                if(rettv->v_type == kNvarFloat || var2.v_type == kNvarFloat)
                {
                    if(op == '+')
                    {
                        f1 = f1 + f2;
                    }
                    else
                    {
                        f1 = f1 - f2;
                    }

                    rettv->v_type = kNvarFloat;
                    rettv->vval.v_float = f1;
                }
                else
                {
                    if(op == '+')
                    {
                        n1 = n1 + n2;
                    }
                    else
                    {
                        n1 = n1 - n2;
                    }

                    rettv->v_type = kNvarNumber;
                    rettv->vval.v_number = n1;
                }
            }

            tv_clear(&var2);
        }
    }

    return OK;
}

/// Handle fifth level expression:
///
/// - __*__  number multiplication
/// - __/__  number division
/// - __%__  number modulo
///
/// @param want_string   after "." operator
///
/// @note
/// - @b arg must point to the first non-white of the expression.
/// - @b arg is advanced to the next non-white after the recognized expression.
///
/// @return OK or FAIL
///
/// @todo move to eval/expressions
static int eval_lev_6(uchar_kt **arg,
                      typval_st *rettv,
                      int evaluate,
                      int want_string)
{
    typval_st var2;
    int op;
    long n1;
    long n2;
    int use_float = FALSE;
    float_kt f1 = 0;
    float_kt f2 = 0;
    bool error = false;

    // Get the first variable.
    if(eval_lev_7(arg, rettv, evaluate, want_string) == FAIL)
    {
        return FAIL;
    }

    // Repeat computing, until no '*', '/' or '%' is following.
    for(;;)
    {
        op = **arg;

        if(op != '*' && op != '/' && op != '%')
        {
            break;
        }

        if(evaluate)
        {
            if(rettv->v_type == kNvarFloat)
            {
                f1 = rettv->vval.v_float;
                use_float = TRUE;
                n1 = 0;
            }
            else
            {
                n1 = tv_get_number_chk(rettv, &error);
            }

            tv_clear(rettv);

            if(error)
            {
                return FAIL;
            }
        }
        else
        {
            n1 = 0;
        }

        // Get the second variable.
        *arg = skipwhite(*arg + 1);

        if(eval_lev_7(arg, &var2, evaluate, FALSE) == FAIL)
        {
            return FAIL;
        }

        if(evaluate)
        {
            if(var2.v_type == kNvarFloat)
            {
                if(!use_float)
                {
                    f1 = n1;
                    use_float = TRUE;
                }

                f2 = var2.vval.v_float;
                n2 = 0;
            }
            else
            {
                n2 = tv_get_number_chk(&var2, &error);
                tv_clear(&var2);

                if(error)
                {
                    return FAIL;
                }

                if(use_float)
                {
                    f2 = n2;
                }
            }

            // Compute the result.
            // When either side is a float the result is a float.
            if(use_float)
            {
                if(op == '*')
                {
                    f1 = f1 * f2;
                }
                else if(op == '/')
                {
                    // Division by zero triggers error from AddressSanitizer
                    #ifdef NAN
                    f1 = (f2 == 0
                          ? (f1 == 0 ? NAN : (f1 > 0 ? INFINITY : -INFINITY) )
                          : f1 / f2);
                    #else
                    f1 = (f2 == 0
                          ? ((f1 > 0 ? INFINITY : -INFINITY))
                          : f1 / f2);
                    #endif
                }
                else
                {
                    EMSG(_("E804: Cannot use '%' with Float"));
                    return FAIL;
                }

                rettv->v_type = kNvarFloat;
                rettv->vval.v_float = f1;
            }
            else
            {
                if(op == '*')
                {
                    n1 = n1 * n2;
                }
                else if(op == '/')
                {
                    if(n2 == 0) // give an error message ?
                    {
                        if(n1 == 0)
                        {
                            n1 = -0x7fffffffL - 1L; // similar to NaN
                        }
                        else if(n1 < 0)
                        {
                            n1 = -0x7fffffffL;
                        }
                        else
                        {
                            n1 = 0x7fffffffL;
                        }
                    }
                    else
                    {
                        n1 = n1 / n2;
                    }
                }
                else
                {
                    if(n2 == 0) // give an error message ?
                    {
                        n1 = 0;
                    }
                    else
                    {
                        n1 = n1 % n2;
                    }
                }

                rettv->v_type = kNvarNumber;
                rettv->vval.v_number = n1;
            }
        }
    }

    return OK;
}

/// Handle sixth level expression:
///
///  - @b number                number constant
///  - @b "string"              string constant
///  - @b 'string'              literal string constant
///  - @b &optionName           option value
///  - @b \@r                   register contents
///  - @b identifier            variable value
///  - @b function()            function call
///  - @b $VAR                  environment variable
///  - @b (expression)          nested expression
///  - @b [expr, expr]          List
///  - @b {key: val, key: val}  Dictionary
///
///  Also handle:
///  - '!' in front, logical NOT
///  - '-' in front, unary minus
///  - '+' in front, unary plus (ignored)
///  - trailing '[]'  subscript in String or List
///  - trailing '.name' entry in Dictionary
///
/// @param want_string  after '.' operator
///
/// @note
/// - @b arg must point to the first non-white of the expression.
/// - @b arg is advanced to the next non-white after the recognized expression.
///
/// @return OK or FAIL
///
/// @todo move to eval/expressions
static int eval_lev_7(uchar_kt **arg,
                      typval_st *rettv,
                      int evaluate,
                      int want_string)
{
    long n;
    int len;
    uchar_kt *s;
    uchar_kt *start_leader;
    uchar_kt *end_leader;
    int ret = OK;
    uchar_kt *alias;

    // Initialise variable so that tv_clear() can't mistake this for a
    // string and free a string that isn't there.
    rettv->v_type = kNvarUnknown;

    // Skip '!', '-' and '+' characters.  They are handled later.
    start_leader = *arg;

    while(**arg == '!' || **arg == '-' || **arg == '+')
    {
        *arg = skipwhite(*arg + 1);
    }

    end_leader = *arg;

    switch(**arg)
    {
        case '0': // Number constant.
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            uchar_kt *p = skipdigits(*arg + 1);
            int get_float = false;

            // We accept a float when the format matches
            // "[0-9]\+\.[0-9]\+\([eE][+-]\?[0-9]\+\)\?".  This is very
            // strict to avoid backwards compatibility problems.
            // Don't look for a float after the "." operator, so that
            // ":let vers = 1.2.3" doesn't fail.
            if(!want_string && p[0] == '.' && ascii_isdigit(p[1]))
            {
                get_float = true;
                p = skipdigits(p + 2);

                if(*p == 'e' || *p == 'E')
                {
                    ++p;

                    if(*p == '-' || *p == '+')
                    {
                        ++p;
                    }

                    if(!ascii_isdigit(*p))
                    {
                        get_float = false;
                    }
                    else
                    {
                        p = skipdigits(p + 1);
                    }
                }

                if(ASCII_ISALPHA(*p) || *p == '.')
                {
                    get_float = false;
                }
            }

            if(get_float)
            {
                float_kt f;
                *arg += string2float((char *) *arg, &f);

                if(evaluate)
                {
                    rettv->v_type = kNvarFloat;
                    rettv->vval.v_float = f;
                }
            }
            else
            {
                str_to_num(*arg, NULL, &len, kStrToNumAll, &n, NULL, 0);
                *arg += len;

                if(evaluate)
                {
                    rettv->v_type = kNvarNumber;
                    rettv->vval.v_number = n;
                }
            }

            break;
        }

        // String constant: "string".
        case '"':
            ret = get_string_tv(arg, rettv, evaluate);
            break;

        // Literal string constant: 'str''ing'.
        case '\'':
            ret = get_lit_string_tv(arg, rettv, evaluate);
            break;

        // List: [expr, expr]
        case '[':
            ret = get_list_tv(arg, rettv, evaluate);
            break;

        // Lambda: {arg, arg -> expr}
        // Dictionary: {key: val, key: val}
        case '{':
            ret = get_lambda_tv(arg, rettv, evaluate);

            if(ret == NOTDONE)
            {
                ret = get_dict_tv(arg, rettv, evaluate);
            }

            break;

        // Option value: &name
        case '&':
            ret = get_option_tv((const char **)arg, rettv, evaluate);
            break;

        // Environment variable: $VAR.
        case '$':
            ret = get_env_tv(arg, rettv, evaluate);
            break;

        // Register contents: @r.
        case '@':
            ++*arg;

            if(evaluate)
            {
                rettv->v_type = kNvarString;
                rettv->vval.v_string = get_reg_contents(**arg, kGRegExprSrc);
            }

            if(**arg != NUL)
            {
                ++*arg;
            }

            break;

        // nested expression: (expression).
        case '(':
            *arg = skipwhite(*arg + 1);
            ret = eval_lev_1(arg, rettv, evaluate); // recursive!

            if(**arg == ')')
            {
                ++*arg;
            }
            else if(ret == OK)
            {
                EMSG(_("E110: Missing ')'"));
                tv_clear(rettv);
                ret = FAIL;
            }

            break;

        default:
            ret = NOTDONE;
            break;
    }

    if(ret == NOTDONE)
    {
        // Must be a variable or function name.
        // Can also be a curly-braces kind of name: {expr}.
        s = *arg;
        len = get_name_len((const char **)arg, (char **)&alias, evaluate, true);

        if(alias != NULL)
        {
            s = alias;
        }

        if(len <= 0)
        {
            ret = FAIL;
        }
        else
        {
            if(**arg == '(') // recursive!
            {
                partial_st *partial;

                if(!evaluate)
                {
                    check_vars((const char *)s, len);
                }

                // If "s" is the name of a variable of type kNvarUfunc use its contents.
                s = deref_func_name((const char *)s, &len, &partial, !evaluate);

                // Invoke the function.
                ret = get_func_tv(s,
                                  len,
                                  rettv,
                                  arg,
                                  curwin->w_cursor.lnum,
                                  curwin->w_cursor.lnum,
                                  &len,
                                  evaluate,
                                  partial,
                                  NULL);

                // If evaluate is false rettv->v_type was not set in
                // get_func_tv, but it's needed in handle_subscript() to parse
                // what follows. So set it here.
                if(rettv->v_type == kNvarUnknown && !evaluate && **arg == '(')
                {
                    rettv->vval.v_string = (uchar_kt *)tv_empty_string;
                    rettv->v_type = kNvarUfunc;
                }

                // Stop the expression evaluation when immediately
                // aborting on error, or when an interrupt occurred or
                // an exception was thrown but not caught.
                if(aborting())
                {
                    if(ret == OK)
                    {
                        tv_clear(rettv);
                    }

                    ret = FAIL;
                }
            }
            else if(evaluate)
            {
                ret = get_var_tv((const char *)s, len, rettv, NULL, true, false);
            }
            else
            {
                check_vars((const char *)s, len);
                ret = OK;
            }
        }

        xfree(alias);
    }

    *arg = skipwhite(*arg);

    // Handle following '[', '(' and '.' for expr[expr], expr.name, expr(expr).
    if(ret == OK)
    {
        ret = handle_subscript((const char **)arg, rettv, evaluate, true);
    }

    // Apply logical NOT and unary '-', from right to left, ignore '+'.
    if(ret == OK && evaluate && end_leader > start_leader)
    {
        bool error = false;
        int val = 0;
        float_kt f = 0.0;

        if(rettv->v_type == kNvarFloat)
        {
            f = rettv->vval.v_float;
        }
        else
        {
            val = tv_get_number_chk(rettv, &error);
        }

        if(error)
        {
            tv_clear(rettv);
            ret = FAIL;
        }
        else
        {
            while(end_leader > start_leader)
            {
                --end_leader;

                if(*end_leader == '!')
                {
                    if(rettv->v_type == kNvarFloat)
                    {
                        f = !f;
                    }
                    else
                    {
                        val = !val;
                    }
                }
                else if(*end_leader == '-')
                {
                    if(rettv->v_type == kNvarFloat)
                    {
                        f = -f;
                    }
                    else
                    {
                        val = -val;
                    }
                }
            }

            if(rettv->v_type == kNvarFloat)
            {
                tv_clear(rettv);
                rettv->vval.v_float = f;
            }
            else
            {
                tv_clear(rettv);
                rettv->v_type = kNvarNumber;
                rettv->vval.v_number = val;
            }
        }
    }

    return ret;
}

/// Evaluate an **[expr]** or **[expr:expr]** index. Also **dict.key**
///
/// @param arg
/// @param rettv
/// @param evaluate
/// @param verbose      give error messages
///
/// __**arg__ points to the '[' or '.'.
///
/// @returns
/// FAIL or OK. "*arg" is advanced to after the ']'.
///
/// @todo move to eval/expressions
static int eval_index(uchar_kt **arg,
                      typval_st *rettv,
                      int evaluate,
                      int verbose)
{
    bool empty1 = false;
    bool empty2 = false;
    long n1, n2 = 0;
    ptrdiff_t len = -1;
    int range = false;
    uchar_kt *key = NULL;

    switch(rettv->v_type)
    {
        case kNvarUfunc:
        case kNvarPartial:
        {
            if(verbose)
            {
                EMSG(_("E695: Cannot index a Funcref"));
            }

            return FAIL;
        }

        case kNvarFloat:
        {
            if(verbose)
            {
                EMSG(_(e_float_as_string));
            }

            return FAIL;
        }

        case kNvarSpecial:
        {
            if(verbose)
            {
                EMSG(_("E909: Cannot index a special variable"));
            }

            return FAIL;
        }

        case kNvarUnknown:
        {
            if(evaluate)
            {
                return FAIL;
            }
            // fallthrough
        }

        case kNvarString:
        case kNvarNumber:
        case kNvarList:
        case kNvarDict:
        {
            break;
        }
    }

    typval_st var1 = TV_INITIAL_VALUE;
    typval_st var2 = TV_INITIAL_VALUE;

    if(**arg == '.')
    {
        // dict.name
        key = *arg + 1;

        for(len = 0; ASCII_ISALNUM(key[len]) || key[len] == '_'; ++len)
        { /* empty body */ }

        if(len == 0)
        {
            return FAIL;
        }

        *arg = skipwhite(key + len);
    }
    else
    {
        // something[idx]
        // Get the (first) variable from inside the [].
        *arg = skipwhite(*arg + 1);

        if(**arg == ':')
        {
            empty1 = true;
        }
        else if(eval_lev_1(arg, &var1, evaluate) == FAIL) // Recursive!
        {
            return FAIL;
        }
        else if(evaluate && !tv_check_str(&var1))
        {
            // Not a number or string.
            tv_clear(&var1);
            return FAIL;
        }

        // Get the second variable from inside the [:].
        if(**arg == ':')
        {
            range = TRUE;
            *arg = skipwhite(*arg + 1);

            if(**arg == ']')
            {
                empty2 = true;
            }
            else if(eval_lev_1(arg, &var2, evaluate) == FAIL) // Recursive!
            {
                if(!empty1)
                {
                    tv_clear(&var1);
                }

                return FAIL;
            }
            else if(evaluate && !tv_check_str(&var2))
            {
                // Not a number or string.
                if(!empty1)
                {
                    tv_clear(&var1);
                }

                tv_clear(&var2);
                return FAIL;
            }
        }

        // Check for the ']'
        if(**arg != ']')
        {
            if(verbose)
            {
                emsgf(_(e_missbrac));
            }

            tv_clear(&var1);

            if(range)
            {
                tv_clear(&var2);
            }

            return FAIL;
        }

        *arg = skipwhite(*arg + 1); // skip the ']'
    }

    if(evaluate)
    {
        n1 = 0;

        if(!empty1 && rettv->v_type != kNvarDict)
        {
            n1 = tv_get_number(&var1);
            tv_clear(&var1);
        }

        if(range)
        {
            if(empty2)
            {
                n2 = -1;
            }
            else
            {
                n2 = tv_get_number(&var2);
                tv_clear(&var2);
            }
        }

        switch(rettv->v_type)
        {
            case kNvarNumber:
            case kNvarString:
            {
                const char *const s = tv_get_string(rettv);
                char *v;
                len = (ptrdiff_t)strlen(s);

                if(range)
                {
                    // The resulting variable is a substring.
                    // If the indexes are out of range the result is empty.
                    if(n1 < 0)
                    {
                        n1 = len + n1;

                        if(n1 < 0)
                        {
                            n1 = 0;
                        }
                    }

                    if(n2 < 0)
                    {
                        n2 = len + n2;
                    }
                    else if(n2 >= len)
                    {
                        n2 = len;
                    }

                    if(n1 >= len || n2 < 0 || n1 > n2)
                    {
                        v = NULL;
                    }
                    else
                    {
                        v = xmemdupz(s + n1, (size_t)(n2 - n1 + 1));
                    }
                }
                else
                {
                    // The resulting variable is a string of a single character.
                    // If the index is too big or negative the result is empty.
                    if(n1 >= len || n1 < 0)
                    {
                        v = NULL;
                    }
                    else
                    {
                        v = xmemdupz(s + n1, 1);
                    }
                }

                tv_clear(rettv);
                rettv->v_type = kNvarString;
                rettv->vval.v_string = (uchar_kt *)v;
                break;
            }

            case kNvarList:
            {
                len = tv_list_len(rettv->vval.v_list);

                if(n1 < 0)
                {
                    n1 = len + n1;
                }

                if(!empty1 && (n1 < 0 || n1 >= len))
                {
                    // For a range we allow invalid values and
                    // return an empty list.
                    // A list index out of range is an error.
                    if(!range)
                    {
                        if(verbose)
                        {
                            EMSGN(_(e_listidx), n1);
                        }

                        return FAIL;
                    }

                    n1 = len;
                }

                if(range)
                {
                    list_st *l;
                    listitem_st *item;

                    if(n2 < 0)
                    {
                        n2 = len + n2;
                    }
                    else if(n2 >= len)
                    {
                        n2 = len - 1;
                    }

                    if(!empty2 && (n2 < 0 || n2 + 1 < n1))
                    {
                        n2 = -1;
                    }

                    l = tv_list_alloc();
                    item = tv_list_find(rettv->vval.v_list, n1);

                    while(n1++ <= n2)
                    {
                        tv_list_append_tv(l, &item->li_tv);
                        item = item->li_next;
                    }

                    tv_clear(rettv);
                    rettv->v_type = kNvarList;
                    rettv->vval.v_list = l;
                    l->lv_refcount++;
                }
                else
                {
                    tv_copy(&tv_list_find(rettv->vval.v_list, n1)->li_tv, &var1);
                    tv_clear(rettv);
                    *rettv = var1;
                }

                break;
            }

            case kNvarDict:
            {
                if(range)
                {
                    if(verbose)
                    {
                        emsgf(_(e_dictrange));
                    }

                    if(len == -1)
                    {
                        tv_clear(&var1);
                    }

                    return FAIL;
                }

                if(len == -1)
                {
                    key = (uchar_kt *)tv_get_string_chk(&var1);

                    if(key == NULL)
                    {
                        tv_clear(&var1);
                        return FAIL;
                    }
                }

                dictitem_st *const item =
                    tv_dict_find(rettv->vval.v_dict, (const char *)key, len);

                if(item == NULL && verbose)
                {
                    emsgf(_(e_dictkey), key);
                }

                if(len == -1)
                {
                    tv_clear(&var1);
                }

                if(item == NULL)
                {
                    return FAIL;
                }

                tv_copy(&item->di_tv, &var1);
                tv_clear(rettv);
                *rettv = var1;
                break;
            }

            case kNvarSpecial:
            case kNvarUfunc:
            case kNvarFloat:
            case kNvarPartial:
            case kNvarUnknown:
            {
                break; // Not evaluating, skipping over subscript
            }
        }
    }

    return OK;
}

/// Get an option value
///
/// @param[in,out]  arg   Points to the '&' or '+' before the option name. Is
///                       advanced to the character after the option name.
/// @param[out]  rettv    Location where result is saved.
/// @param[in]  evaluate  If not true, rettv is not populated.
///
/// @return OK or FAIL.
///
/// @todo  move to eval/executor
static int get_option_tv(const char **const arg,
                         typval_st *const rettv,
                         const bool evaluate)
FUNC_ATTR_NONNULL_ARG(1)
{
    long numval;
    uchar_kt *stringval;
    int opt_type;
    int c;
    bool working = (**arg == '+'); // has("+option")
    int ret = OK;
    int opt_flags;

    // Isolate the option name and find its value.
    char *option_end = (char *)find_option_end(arg, &opt_flags);

    if(option_end == NULL)
    {
        if(rettv != NULL)
        {
            EMSG2(_("E112: Option name missing: %s"), *arg);
        }

        return FAIL;
    }

    if(!evaluate)
    {
        *arg = option_end;
        return OK;
    }

    c = *option_end;
    *option_end = NUL;

    opt_type = get_option_value((uchar_kt *)(*arg),
                                &numval,
                                rettv == NULL ? NULL : &stringval,
                                opt_flags);

    // invalid name
    if(opt_type == -3)
    {
        if(rettv != NULL)
        {
            EMSG2(_("E113: Unknown option: %s"), *arg);
        }

        ret = FAIL;
    }
    else if(rettv != NULL)
    {
        if(opt_type == -2)
        {
            // hidden string option
            rettv->v_type = kNvarString;
            rettv->vval.v_string = NULL;
        }
        else if(opt_type == -1)
        {
            // hidden number option
            rettv->v_type = kNvarNumber;
            rettv->vval.v_number = 0;
        }
        else if(opt_type == 1)
        {
            // number option
            rettv->v_type = kNvarNumber;
            rettv->vval.v_number = numval;
        }
        else
        {
            // string option
            rettv->v_type = kNvarString;
            rettv->vval.v_string = stringval;
        }
    }
    else if(working && (opt_type == -2 || opt_type == -1))
    {
        ret = FAIL;
    }

    *option_end = c; // put back for error messages
    *arg = option_end;

    return ret;
}

/// Allocate a variable for a string constant.
/// Return OK or FAIL.
static int get_string_tv(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    uchar_kt *p;
    uchar_kt *name;
    unsigned int extra = 0;

    // Find the end of the string, skipping backslashed characters.
    for(p = *arg + 1; *p != NUL && *p != '"'; mb_ptr_adv(p))
    {
        if(*p == '\\' && p[1] != NUL)
        {
            ++p;

            // A "\<x>" form occupies at least 4 characters, and
            // produces up to 6 characters: reserve space for 2 extra
            if(*p == '<')
            {
                extra += 2;
            }
        }
    }

    if(*p != '"')
    {
        EMSG2(_("E114: Missing quote: %s"), *arg);
        return FAIL;
    }

    // If only parsing, set *arg and return here
    if(!evaluate)
    {
        *arg = p + 1;
        return OK;
    }

    // Copy the string into allocated memory,
    // handling backslashed characters.
    name = xmalloc(p - *arg + extra);
    rettv->v_type = kNvarString;
    rettv->vval.v_string = name;

    for(p = *arg + 1; *p != NUL && *p != '"';)
    {
        if(*p == '\\')
        {
            switch(*++p)
            {
                case 'b':
                    *name++ = BS;
                    ++p;
                    break;

                case 'e':
                    *name++ = ESC;
                    ++p;
                    break;

                case 'f':
                    *name++ = FF;
                    ++p;
                    break;

                case 'n':
                    *name++ = NL;
                    ++p;
                    break;

                case 'r':
                    *name++ = CAR;
                    ++p;
                    break;

                case 't':
                    *name++ = TAB;
                    ++p;
                    break;

                case 'X': // hex: "\x1", "\x12"
                case 'x':
                case 'u': // Unicode: "\u0023"
                case 'U':
                    if(ascii_isxdigit(p[1]))
                    {
                        int n, nr;
                        int c = toupper(*p);

                        if(c == 'X')
                        {
                            n = 2;
                        }
                        else if(*p == 'u')
                        {
                            n = 4;
                        }
                        else
                        {
                            n = 8;
                        }

                        nr = 0;

                        while(--n >= 0 && ascii_isxdigit(p[1]))
                        {
                            ++p;
                            nr = (nr << 4) + hex_to_num(*p);
                        }

                        ++p;

                        // For "\u" store the number according to 'encoding'.
                        if(c != 'X')
                        {
                            name += (*mb_char2bytes)(nr, name);
                        }
                        else
                        {
                            *name++ = nr;
                        }
                    }

                    break;

                // octal: "\1", "\12", "\123"
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    *name = *p++ - '0';

                    if(*p >= '0' && *p <= '7')
                    {
                        *name = (*name << 3) + *p++ - '0';

                        if(*p >= '0' && *p <= '7')
                        {
                            *name = (*name << 3) + *p++ - '0';
                        }
                    }

                    ++name;
                    break;

                // Special key, e.g.: "\<C-W>"
                case '<':
                    extra = trans_special((const uchar_kt **)&p,
                                          ustrlen(p), name, true, true);

                    if(extra != 0)
                    {
                        name += extra;
                        break;
                    }

                    FALL_THROUGH_ATTRIBUTE;

                default:
                    mb_copy_char((const uchar_kt **)&p, &name);
                    break;
            }
        }
        else
        {
            mb_copy_char((const uchar_kt **)&p, &name);
        }
    }

    *name = NUL;

    if(*p != NUL) // just in case
    {
        p++;
    }

    *arg = p;
    return OK;
}

/// Allocate a variable for a 'str''ing' constant.
/// Return OK or FAIL.
static int get_lit_string_tv(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    uchar_kt *p;
    uchar_kt *str;
    int reduce = 0;

    // Find the end of the string, skipping ''.
    for(p = *arg + 1; *p != NUL; mb_ptr_adv(p))
    {
        if(*p == '\'')
        {
            if(p[1] != '\'')
            {
                break;
            }

            ++reduce;
            ++p;
        }
    }

    if(*p != '\'')
    {
        EMSG2(_("E115: Missing quote: %s"), *arg);
        return FAIL;
    }

    // If only parsing return after setting "*arg"
    if(!evaluate)
    {
        *arg = p + 1;
        return OK;
    }

    // Copy the string into allocated memory,
    // handling '' to ' reduction.
    str = xmalloc((p - *arg) - reduce);
    rettv->v_type = kNvarString;
    rettv->vval.v_string = str;

    for(p = *arg + 1; *p != NUL;)
    {
        if(*p == '\'')
        {
            if(p[1] != '\'')
            {
                break;
            }

            ++p;
        }

        mb_copy_char((const uchar_kt **)&p, &str);
    }

    *str = NUL;
    *arg = p + 1;
    return OK;
}

/// @return the function name of the partial.
uchar_kt *partial_name(partial_st *pt)
{
    if(pt->pt_name != NULL)
    {
        return pt->pt_name;
    }

    return pt->pt_func->uf_name;
}

/// @todo move to eval/typval.h
static void partial_free(partial_st *pt)
{
    for(int i = 0; i < pt->pt_argc; i++)
    {
        tv_clear(&pt->pt_argv[i]);
    }

    xfree(pt->pt_argv);
    tv_dict_unref(pt->pt_dict);

    if(pt->pt_name != NULL)
    {
        func_unref(pt->pt_name);
        xfree(pt->pt_name);
    }
    else
    {
        func_ptr_unref(pt->pt_func);
    }

    xfree(pt);
}

/// Unreference a closure:
/// decrement the reference count and free it when it becomes zero.
///
/// @todo ove to eval/typval.h
void partial_unref(partial_st *pt)
{
    if(pt != NULL && --pt->pt_refcount <= 0)
    {
        partial_free(pt);
    }
}

/// Allocate a variable for a List and fill it from "*arg".
/// Return OK or FAIL.
static int get_list_tv(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    list_st *l = NULL;
    typval_st tv;
    listitem_st *item;

    if(evaluate)
    {
        l = tv_list_alloc();
    }

    *arg = skipwhite(*arg + 1);

    while(**arg != ']' && **arg != NUL)
    {
        if(eval_lev_1(arg, &tv, evaluate) == FAIL) // recursive!
        {
            goto failret;
        }

        if(evaluate)
        {
            item = tv_list_item_alloc();
            item->li_tv = tv;
            item->li_tv.v_lock = 0;
            tv_list_append(l, item);
        }

        if(**arg == ']')
        {
            break;
        }

        if(**arg != ',')
        {
            EMSG2(_("E696: Missing comma in List: %s"), *arg);
            goto failret;
        }

        *arg = skipwhite(*arg + 1);
    }

    if(**arg != ']')
    {
        EMSG2(_("E697: Missing end of List ']': %s"), *arg);

failret:

        if(evaluate)
        {
            tv_list_free(l);
        }

        return FAIL;
    }

    *arg = skipwhite(*arg + 1);

    if(evaluate)
    {
        rettv->v_type = kNvarList;
        rettv->vval.v_list = l;
        ++l->lv_refcount;
    }

    return OK;
}

/// @param tv1
/// @param tv2
/// @param ic   ignore case
bool func_equal(typval_st *tv1, typval_st *tv2, bool ic)
{
    int a1;
    int a2;
    uchar_kt *s1;
    uchar_kt *s2;
    dict_st *d1;
    dict_st *d2;

    // empty and NULL function name considered the same
    s1 = tv1->v_type == kNvarUfunc
         ? tv1->vval.v_string : partial_name(tv1->vval.v_partial);

    if(s1 != NULL && *s1 == NUL)
    {
        s1 = NULL;
    }

    s2 = tv2->v_type == kNvarUfunc
         ? tv2->vval.v_string : partial_name(tv2->vval.v_partial);

    if(s2 != NULL && *s2 == NUL)
    {
        s2 = NULL;
    }

    if(s1 == NULL || s2 == NULL)
    {
        if(s1 != s2)
        {
            return false;
        }
    }
    else if(ustrcmp(s1, s2) != 0)
    {
        return false;
    }

    // empty dict and NULL dict is different
    d1 = tv1->v_type == kNvarUfunc ? NULL : tv1->vval.v_partial->pt_dict;
    d2 = tv2->v_type == kNvarUfunc ? NULL : tv2->vval.v_partial->pt_dict;

    if(d1 == NULL || d2 == NULL)
    {
        if(d1 != d2)
        {
            return false;
        }
    }
    else if(!tv_dict_equal(d1, d2, ic, true))
    {
        return false;
    }

    // empty list and no list considered the same
    a1 = tv1->v_type == kNvarUfunc ? 0 : tv1->vval.v_partial->pt_argc;
    a2 = tv2->v_type == kNvarUfunc ? 0 : tv2->vval.v_partial->pt_argc;

    if(a1 != a2)
    {
        return false;
    }

    for(int i = 0; i < a1; i++)
    {
        if(!tv_equal(tv1->vval.v_partial->pt_argv + i,
                     tv2->vval.v_partial->pt_argv + i,
                     ic,
                     true))
        {
            return false;
        }
    }

    return true;
}

/// Get next (unique) copy ID
///
/// Used for traversing nested structures e.g.
/// when serializing them or garbage collecting.
int get_copyID(void)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    // CopyID for recursively traversing lists and dicts
    //
    // This value is needed to avoid endless recursiveness.
    // Last bit is used for previous_funccal and normally
    // ignored when comparing.
    static int current_copyID = 0;

    current_copyID += COPYID_INC;

    return current_copyID;
}

/// Used by get_func_tv()
static garray_st funcargs = GA_EMPTY_INIT_VALUE;

// Garbage collection for lists and dictionaries.
//
// We use reference counts to be able to free most items right away when they
// are no longer used.  But for composite items it's possible that it becomes
// unused while the reference count is > 0: When there is a recursive
// reference.  Example:
//  - :let l = [1, 2, 3]
//  - :let d = {9: l}
//  - :let l[1] = d
//
// Since this is quite unusual we handle this with garbage collection: every
// once in a while find out which lists and dicts are not referenced from any
// variable.
//
// Here is a good reference text about garbage collection (refers to Python
// but it applies to all reference-counting mechanisms):
// http://python.ca/nas/python/gc/

/// Do garbage collection for lists and dicts.
///
/// @param testing  true if called from test_garbagecollect_now().
/// @returns        true if some memory was freed.
bool garbage_collect(bool testing)
{
    bool abort = false;

#define ABORTING(func)  abort = abort || func
    if(!testing)
    {
        // Only do this once.
        want_garbage_collect = false;
        may_garbage_collect = false;
        garbage_collect_at_exit = false;
    }

    // We advance by two (COPYID_INC) because we add one for items referenced
    // through previous_funccal.
    const int copyID = get_copyID();

    // 1. Go through all accessible variables and mark all lists and dicts
    // with copyID.

    // Don't free variables in the previous_funccal list unless they are only
    // referenced through previous_funccal.  This must be first, because if
    // the item is referenced elsewhere the funccal must not be freed.
    for(func_call_st *fc = previous_funccal; fc != NULL; fc = fc->caller)
    {
        fc->fc_copyID = copyID + 1;
        ABORTING(set_ref_in_ht)(&fc->l_vars.dv_hashtab, copyID + 1, NULL);
        ABORTING(set_ref_in_ht)(&fc->l_avars.dv_hashtab, copyID + 1, NULL);
    }

    // script-local variables
    for(int i = 1; i <= ga_scripts.ga_len; ++i)
    {
        ABORTING(set_ref_in_ht)(&SCRIPT_VARS(i), copyID, NULL);
    }

    FOR_ALL_BUFFERS(buf)
    {
        // buffer-local variables
        ABORTING(set_ref_in_item)(&buf->b_bufvar.di_tv, copyID, NULL, NULL);

        // buffer marks (ShaDa additional data)
        ABORTING(set_ref_in_fmark)(buf->b_last_cursor, copyID);
        ABORTING(set_ref_in_fmark)(buf->b_last_insert, copyID);
        ABORTING(set_ref_in_fmark)(buf->b_last_change, copyID);

        for(size_t i = 0; i < NMARKS; i++)
        {
            ABORTING(set_ref_in_fmark)(buf->b_namedm[i], copyID);
        }

        // buffer change list (ShaDa additional data)
        for(int i = 0; i < buf->b_changelistlen; i++)
        {
            ABORTING(set_ref_in_fmark)(buf->b_changelist[i], copyID);
        }

        // buffer ShaDa additional data
        ABORTING(set_ref_dict)(buf->additional_data, copyID);
    }
    FOR_ALL_TAB_WINDOWS(tp, wp)
    {
        // window-local variables
        ABORTING(set_ref_in_item)(&wp->w_winvar.di_tv, copyID, NULL, NULL);

        // window jump list (ShaDa additional data)
        for(int i = 0; i < wp->w_jumplistlen; i++)
        {
            ABORTING(set_ref_in_fmark)(wp->w_jumplist[i].fmark, copyID);
        }
    }

    if(aucmd_win != NULL)
    {
        ABORTING(set_ref_in_item)(&aucmd_win->w_winvar.di_tv, copyID, NULL, NULL);
    }

    // registers (ShaDa additional data)
    {
        const void *reg_iter = NULL;

        do
        {
            yankreg_st reg;
            char name = NUL;
            reg_iter = op_register_iter(reg_iter, &name, &reg);

            if(name != NUL)
            {
                ABORTING(set_ref_dict)(reg.additional_data, copyID);
            }
        } while(reg_iter != NULL);
    }

    // global marks (ShaDa additional data)
    {
        const void *mark_iter = NULL;

        do
        {
            xfilemark_st fm;
            char name = NUL;
            mark_iter = mark_global_iter(mark_iter, &name, &fm);

            if(name != NUL)
            {
                ABORTING(set_ref_dict)(fm.fmark.additional_data, copyID);
            }
        } while(mark_iter != NULL);
    }

    // tabpage-local variables
    FOR_ALL_TABS(tp)
    {
        ABORTING(set_ref_in_item)(&tp->tp_winvar.di_tv, copyID, NULL, NULL);
    }

    // global variables
    ABORTING(set_ref_in_ht)(&globvarht, copyID, NULL);

    // function-local variables
    for(func_call_st *fc = current_funccal; fc != NULL; fc = fc->caller)
    {
        fc->fc_copyID = copyID;

        ABORTING(set_ref_in_ht)(&fc->l_vars.dv_hashtab, copyID, NULL);
        ABORTING(set_ref_in_ht)(&fc->l_avars.dv_hashtab, copyID, NULL);
    }

    // named functions (matters for closures)
    ABORTING(set_ref_in_functions(copyID));

    // Jobs
    {
        terminal_jobdata_st *data;
        map_foreach_value(jobs, data, {
            set_ref_in_callback(&data->on_stdout, copyID, NULL, NULL);
            set_ref_in_callback(&data->on_stderr, copyID, NULL, NULL);
            set_ref_in_callback(&data->on_exit, copyID, NULL, NULL);
        })
    }

    // Timers
    {
        timer_st *timer;
        map_foreach_value(timers, timer, {
            set_ref_in_callback(&timer->callback, copyID, NULL, NULL);
        })
    }

    // function call arguments, if v:testing is set.
    for(int i = 0; i < funcargs.ga_len; i++)
    {
        ABORTING(set_ref_in_item)(((typval_st **)funcargs.ga_data)[i], copyID, NULL, NULL);
    }

    // v: vars
    ABORTING(set_ref_in_ht)(&vimvarht, copyID, NULL);

    // history items (ShaDa additional elements)
    if(p_hi)
    {
        for(uint8_t i = 0; i < HIST_COUNT; i++)
        {
            const void *iter = NULL;

            do
            {
                history_st hist;
                iter = hist_iter(iter, i, false, &hist);

                if(hist.hisstr != NULL)
                {
                    ABORTING(set_ref_list)(hist.additional_elements, copyID);
                }
            } while(iter != NULL);
        }
    }

    // previously used search/substitute patterns (ShaDa additional data)
    {
        search_pattern_st pat;

        get_search_pattern(&pat);
        ABORTING(set_ref_dict)(pat.additional_data, copyID);

        get_substitute_pattern(&pat);
        ABORTING(set_ref_dict)(pat.additional_data, copyID);
    }

    // previously used replacement string
    {
        subrepstr_st sub;
        sub_get_replacement(&sub);
        ABORTING(set_ref_list)(sub.additional_elements, copyID);
    }
    bool did_free = false;

    if(!abort)
    {
        // 2. Free lists and dictionaries that are not referenced.
        did_free = free_unref_items(copyID);

        // 3. Check if any funccal can be freed now.
        bool did_free_funccal = false;

        for(func_call_st **pfc = &previous_funccal; *pfc != NULL;)
        {
            if(can_free_funccal(*pfc, copyID))
            {
                func_call_st *fc = *pfc;

                *pfc = fc->caller;
                free_funccal(fc, true);

                did_free = true;
                did_free_funccal = true;
            }
            else
            {
                pfc = &(*pfc)->caller;
            }
        }

        if(did_free_funccal)
        {
            // When a funccal was freed some more items
            // might be garbage collected, so run again.
            (void)garbage_collect(testing);
        }
    }
    else if(p_verbose > 0)
    {
        verb_msg((uchar_kt *)_("Not enough memory to set references, "
                             "garbage collection aborted!"));
    }

#undef ABORTING

    return did_free;
}

/// Free lists and dictionaries that are no longer referenced.
///
/// @note This function may only be called from garbage_collect().
///
/// @param copyID Free lists/dictionaries that don't have this ID.
/// @return true, if something was freed.
static int free_unref_items(int copyID)
{
    dict_st *dd, *dd_next;
    list_st *ll, *ll_next;
    bool did_free = false;

    // Let all "free" functions know that we are here. This means no
    // dictionaries, lists, or jobs are to be freed, because we will
    // do that here.
    tv_in_free_unref_items = true;

    // PASS 1: free the contents of the items. We don't free the items
    // themselves yet, so that it is possible to decrement refcount counters.

    // Go through the list of dicts and free items without the copyID.
    // Don't free dicts that are referenced internally.
    for(dict_st *dd = gc_first_dict; dd != NULL; dd = dd->dv_used_next)
    {
        if((dd->dv_copyID & COPYID_MASK) != (copyID & COPYID_MASK))
        {
            // Free the Dictionary and ordinary items it contains, but don't
            // recurse into Lists and Dictionaries, they will be in the list
            // of dicts or list of lists.
            tv_dict_free_contents(dd);
            did_free = true;
        }
    }

    // Go through the list of lists and free items without the copyID.
    // But don't free a list that has a watcher (used in a for loop), these
    // are not referenced anywhere.
    for(list_st *ll = gc_first_list; ll != NULL; ll = ll->lv_used_next)
    {
        if((ll->lv_copyID & COPYID_MASK) != (copyID & COPYID_MASK)
           && ll->lv_watch == NULL)
        {
            // Free the List and ordinary items it contains, but don't recurse
            // into Lists and Dictionaries, they will be in the list of dicts
            // or list of lists.
            tv_list_free_contents(ll);
            did_free = true;
        }
    }

    // PASS 2: free the items themselves.
    for(dd = gc_first_dict; dd != NULL; dd = dd_next)
    {
        dd_next = dd->dv_used_next;

        if((dd->dv_copyID & COPYID_MASK) != (copyID & COPYID_MASK))
        {
            tv_dict_free_dict(dd);
        }
    }

    for(ll = gc_first_list; ll != NULL; ll = ll_next)
    {
        ll_next = ll->lv_used_next;

        if((ll->lv_copyID & COPYID_MASK) != (copyID & COPYID_MASK)
           && ll->lv_watch == NULL)
        {
            // Free the List and ordinary items it contains, but don't recurse
            // into Lists and Dictionaries, they will be in the list of dicts
            // or list of lists.
            tv_list_free_list(ll);
        }
    }

    tv_in_free_unref_items = false;
    return did_free;
}

/// Mark all lists and dicts referenced through hashtab "ht" with "copyID".
///
/// @param ht            Hashtab content will be marked.
/// @param copyID        New mark for lists and dicts.
/// @param list_stack    Used to add lists to be marked. Can be NULL.
///
/// @returns             true if setting references failed somehow.
bool set_ref_in_ht(hashtable_st *ht, int copyID, list_stack_T **list_stack)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    bool abort = false;
    ht_stack_T *ht_stack = NULL;

    hashtable_st *cur_ht = ht;

    for(;;)
    {
        if(!abort)
        {
            // Mark each item in the hashtab.  If the item contains a hashtab
            // it is added to ht_stack, if it contains a list it is added to
            // list_stack.
            HASHTAB_ITER(cur_ht, hi, {
                abort = abort
                        || set_ref_in_item(&TV_DICT_HI2DI(hi)->di_tv,
                                           copyID,
                                           &ht_stack,
                                           list_stack);
            });
        }

        if(ht_stack == NULL)
        {
            break;
        }

        // take an item from the stack
        cur_ht = ht_stack->ht;

        ht_stack_T *tempitem = ht_stack;

        ht_stack = ht_stack->prev;
        xfree(tempitem);
    }

    return abort;
}

/// Mark all lists and dicts referenced through list "l" with "copyID".
///
/// @param l             List content will be marked.
/// @param copyID        New mark for lists and dicts.
/// @param ht_stack      Used to add hashtabs to be marked. Can be NULL.
///
/// @returns             true if setting references failed somehow.
bool set_ref_in_list(list_st *l, int copyID, ht_stack_T **ht_stack)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    bool abort = false;
    list_stack_T *list_stack = NULL;

    list_st *cur_l = l;

    for(;;)
    {
        if(!abort)
        {
            // Mark each item in the list.  If the item contains a hashtab
            // it is added to ht_stack, if it contains a list it is added to
            // list_stack.
            for(listitem_st *li = cur_l->lv_first;
                !abort && li != NULL;
                li = li->li_next)
            {
                abort = set_ref_in_item(&li->li_tv,
                                        copyID,
                                        ht_stack,
                                        &list_stack);
            }
        }

        if(list_stack == NULL)
        {
            break;
        }

        // take an item from the stack
        cur_l = list_stack->list;

        list_stack_T *tempitem = list_stack;

        list_stack = list_stack->prev;
        xfree(tempitem);
    }

    return abort;
}

/// Mark all lists and dicts referenced through typval "tv" with "copyID".
///
/// @param tv            Typval content will be marked.
/// @param copyID        New mark for lists and dicts.
/// @param ht_stack      Used to add hashtabs to be marked. Can be NULL.
/// @param list_stack    Used to add lists to be marked. Can be NULL.
///
/// @returns             true if setting references failed somehow.
bool set_ref_in_item(typval_st *tv, int copyID, ht_stack_T **ht_stack,
                     list_stack_T **list_stack)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    bool abort = false;

    switch(tv->v_type)
    {
        case kNvarDict:
        {
            dict_st *dd = tv->vval.v_dict;

            if(dd != NULL && dd->dv_copyID != copyID)
            {
                // Didn't see this dict yet.
                dd->dv_copyID = copyID;

                if(ht_stack == NULL)
                {
                    abort = set_ref_in_ht(&dd->dv_hashtab, copyID, list_stack);
                }
                else
                {
                    ht_stack_T *newitem = xmalloc(sizeof(ht_stack_T));

                    if(newitem == NULL)
                    {
                        abort = true;
                    }
                    else
                    {
                        newitem->ht = &dd->dv_hashtab;
                        newitem->prev = *ht_stack;
                        *ht_stack = newitem;
                    }
                }

                queue_st *w = NULL;
                dict_watcher_st *watcher = NULL;

                QUEUE_FOREACH(w, &dd->watchers)
                {
                    watcher = tv_dict_watcher_node_data(w);

                    set_ref_in_callback(&watcher->callback,
                                        copyID,
                                        ht_stack,
                                        list_stack);
                }
            }

            break;
        }

        case kNvarList:
        {
            list_st *ll = tv->vval.v_list;

            if(ll != NULL && ll->lv_copyID != copyID)
            {
                // Didn't see this list yet.
                ll->lv_copyID = copyID;

                if(list_stack == NULL)
                {
                    abort = set_ref_in_list(ll, copyID, ht_stack);
                }
                else
                {
                    list_stack_T *newitem = xmalloc(sizeof(list_stack_T));

                    if(newitem == NULL)
                    {
                        abort = true;
                    }
                    else
                    {
                        newitem->list = ll;
                        newitem->prev = *list_stack;
                        *list_stack = newitem;
                    }
                }
            }

            break;
        }

        case kNvarPartial:
        {
            partial_st *pt = tv->vval.v_partial;

            // A partial does not have a copyID,
            // because it cannot contain itself.
            if(pt != NULL)
            {
                abort = set_ref_in_func(pt->pt_name, pt->pt_func, copyID);

                if(pt->pt_dict != NULL)
                {
                    typval_st dtv;

                    dtv.v_type = kNvarDict;
                    dtv.vval.v_dict = pt->pt_dict;

                    abort = abort || set_ref_in_item(&dtv,
                                                     copyID,
                                                     ht_stack,
                                                     list_stack);
                }

                for(int i = 0; i < pt->pt_argc; i++)
                {
                    abort = abort || set_ref_in_item(&pt->pt_argv[i],
                                                     copyID,
                                                     ht_stack,
                                                     list_stack);
                }
            }

            break;
        }

        case kNvarUfunc:
            abort = set_ref_in_func(tv->vval.v_string, NULL, copyID);
            break;

        case kNvarUnknown:
        case kNvarSpecial:
        case kNvarFloat:
        case kNvarNumber:
        case kNvarString:
        {
            break;
        }
    }

    return abort;
}

/// Set "copyID" in all functions available by name.
bool set_ref_in_functions(int copyID)
{
    int todo;
    hashitem_st *hi = NULL;
    bool abort = false;
    ufunc_st *fp;
    todo = (int)func_hashtab.ht_used;

    for(hi = func_hashtab.ht_array; todo > 0 && !got_int; hi++)
    {
        if(!HASHITEM_EMPTY(hi))
        {
            todo--;
            fp = HI2UF(hi);

            if(!func_name_refcount(fp->uf_name))
            {
                abort = abort || set_ref_in_func(NULL, fp, copyID);
            }
        }
    }

    return abort;
}

/// Mark all lists and dicts referenced in given mark
///
/// @returns true if setting references failed somehow.
static inline bool set_ref_in_fmark(mark_st fm, int copyID)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(fm.additional_data != NULL && fm.additional_data->dv_copyID != copyID)
    {
        fm.additional_data->dv_copyID = copyID;
        return set_ref_in_ht(&fm.additional_data->dv_hashtab, copyID, NULL);
    }

    return false;
}

/// Mark all lists and dicts referenced in given list and the list itself
///
/// @returns true if setting references failed somehow.
static inline bool set_ref_list(list_st *list, int copyID)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(list != NULL)
    {
        typval_st tv = (typval_st) {
            .v_type = kNvarList,
            .vval = { .v_list = list }
        };

        return set_ref_in_item(&tv, copyID, NULL, NULL);
    }

    return false;
}

/// Mark all lists and dicts referenced in given dict and the dict itself
///
/// @returns true if setting references failed somehow.
static inline bool set_ref_dict(dict_st *dict, int copyID)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(dict != NULL)
    {
        typval_st tv = (typval_st) {
            .v_type = kNvarDict,
            .vval = { .v_dict = dict }
        };

        return set_ref_in_item(&tv, copyID, NULL, NULL);
    }

    return false;
}

static bool set_ref_in_funccal(func_call_st *fc, int copyID)
{
    bool abort = false;

    if(fc->fc_copyID != copyID)
    {
        fc->fc_copyID = copyID;

        abort = abort || set_ref_in_ht(&fc->l_vars.dv_hashtab, copyID, NULL);
        abort = abort || set_ref_in_ht(&fc->l_avars.dv_hashtab, copyID, NULL);
        abort = abort || set_ref_in_func(NULL, fc->func, copyID);
    }

    return abort;
}

/// Allocate a variable for a Dictionary and fill it from "*arg".
/// Return OK or FAIL.  Returns NOTDONE for {expr}.
static int get_dict_tv(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    dict_st *d = NULL;
    typval_st tvkey;
    typval_st tv;
    uchar_kt *key = NULL;
    dictitem_st *item;
    uchar_kt *start = skipwhite(*arg + 1);
    char buf[NUMBUFLEN];

    // First check if it's not a curly-braces thing: {expr}.
    // Must do this without evaluating, otherwise a function
    // may be called twice. Unfortunately this means we need
    // to call eval_lev_1() twice for the first item.
    // But {} is an empty Dictionary.
    if(*start != '}')
    {
        if(eval_lev_1(&start, &tv, FALSE) == FAIL) // recursive!
        {
            return FAIL;
        }

        if(*start == '}')
        {
            return NOTDONE;
        }
    }

    if(evaluate)
    {
        d = tv_dict_alloc();
    }

    tvkey.v_type = kNvarUnknown;
    tv.v_type = kNvarUnknown;
    *arg = skipwhite(*arg + 1);

    while(**arg != '}' && **arg != NUL)
    {
        if(eval_lev_1(arg, &tvkey, evaluate) == FAIL) // recursive!
        {
            goto failret;
        }

        if(**arg != ':')
        {
            EMSG2(_("E720: Missing colon in Dictionary: %s"), *arg);
            tv_clear(&tvkey);
            goto failret;
        }

        if(evaluate)
        {
            key = (uchar_kt *)tv_get_string_buf_chk(&tvkey, buf);

            if(key == NULL)
            {
                // "key" is NULL when tv_get_string_buf_chk() gave an errmsg
                tv_clear(&tvkey);
                goto failret;
            }
        }

        *arg = skipwhite(*arg + 1);

        if(eval_lev_1(arg, &tv, evaluate) == FAIL) // Recursive!
        {
            if(evaluate)
            {
                tv_clear(&tvkey);
            }

            goto failret;
        }

        if(evaluate)
        {
            item = tv_dict_find(d, (const char *)key, -1);

            if(item != NULL)
            {
                EMSG2(_("E721: Duplicate key in Dictionary: \"%s\""), key);
                tv_clear(&tvkey);
                tv_clear(&tv);
                goto failret;
            }

            item = tv_dict_item_alloc((const char *)key);

            tv_clear(&tvkey);
            item->di_tv = tv;
            item->di_tv.v_lock = 0;

            if(tv_dict_add(d, item) == FAIL)
            {
                tv_dict_item_free(item);
            }
        }

        if(**arg == '}')
        {
            break;
        }

        if(**arg != ',')
        {
            EMSG2(_("E722: Missing comma in Dictionary: %s"), *arg);
            goto failret;
        }

        *arg = skipwhite(*arg + 1);
    }

    if(**arg != '}')
    {
        EMSG2(_("E723: Missing end of Dictionary '}': %s"), *arg);

failret:

        if(evaluate)
        {
            tv_dict_free(d);
        }

        return FAIL;
    }

    *arg = skipwhite(*arg + 1);

    if(evaluate)
    {
        rettv->v_type = kNvarDict;
        rettv->vval.v_dict = d;
        ++d->dv_refcount;
    }

    return OK;
}

/// Get function arguments.
static int get_function_args(uchar_kt **argp,
                             uchar_kt endchar,
                             garray_st *newargs,
                             int *varargs,
                             bool skip)
{
    int c;
    int i;
    bool mustend = false;
    uchar_kt *arg = *argp;
    uchar_kt *p = arg;

    if(newargs != NULL)
    {
        ga_init(newargs, (int)sizeof(uchar_kt *), 3);
    }

    if(varargs != NULL)
    {
        *varargs = false;
    }

    // Isolate the arguments: "arg1, arg2, ...)"
    while(*p != endchar)
    {
        if(p[0] == '.' && p[1] == '.' && p[2] == '.')
        {
            if(varargs != NULL)
            {
                *varargs = true;
            }

            p += 3;
            mustend = true;
        }
        else
        {
            arg = p;

            while(ASCII_ISALNUM(*p) || *p == '_')
            {
                p++;
            }

            if(arg == p
               || isdigit(*arg)
               || (p - arg == 9 && ustrncmp(arg, "firstline", 9) == 0)
               || (p - arg == 8 && ustrncmp(arg, "lastline", 8) == 0))
            {
                if(!skip)
                {
                    EMSG2(_("E125: Illegal argument: %s"), arg);
                }

                break;
            }

            if(newargs != NULL)
            {
                ga_grow(newargs, 1);
                c = *p;
                *p = NUL;
                arg = ustrdup(arg);

                if(arg == NULL)
                {
                    *p = c;
                    goto err_ret;
                }

                // Check for duplicate argument name.
                for(i = 0; i < newargs->ga_len; i++)
                {
                    if(ustrcmp(((uchar_kt **)(newargs->ga_data))[i], arg) == 0)
                    {
                        EMSG2(_("E853: Duplicate argument name: %s"), arg);
                        xfree(arg);
                        goto err_ret;
                    }
                }

                ((uchar_kt **)(newargs->ga_data))[newargs->ga_len] = arg;
                newargs->ga_len++;
                *p = c;
            }

            if(*p == ',')
            {
                p++;
            }
            else
            {
                mustend = true;
            }
        }

        p = skipwhite(p);

        if(mustend && *p != endchar)
        {
            if(!skip)
            {
                EMSG2(_(e_invarg2), *argp);
            }

            break;
        }
    }

    if(*p != endchar)
    {
        goto err_ret;
    }

    p++; // skip "endchar"
    *argp = p;

    return OK;

err_ret:

    if(newargs != NULL)
    {
        ga_clear_strings(newargs);
    }

    return FAIL;
}

/// Register function "fp" as using "current_funccal" as its scope.
static void register_closure(ufunc_st *fp)
{
    if(fp->uf_scoped == current_funccal)
    {
        // no change
        return;
    }

    funccal_unref(fp->uf_scoped, fp, false);

    fp->uf_scoped = current_funccal;
    current_funccal->fc_refcount++;
    ga_grow(&current_funccal->fc_funcs, 1);
    ((ufunc_st **)current_funccal->fc_funcs.ga_data)
    [current_funccal->fc_funcs.ga_len++] = fp;
}

/// Parse a lambda expression and get a Funcref from "*arg".
///
/// @return OK or FAIL.  Returns NOTDONE for dict or {expr}.
static int get_lambda_tv(uchar_kt **arg, typval_st *rettv, bool evaluate)
{
    garray_st newargs = GA_EMPTY_INIT_VALUE;
    garray_st *pnewargs;
    ufunc_st *fp = NULL;
    int varargs;
    int ret;
    uchar_kt *start = skipwhite(*arg + 1);
    uchar_kt *s;
    uchar_kt *e;
    static int lambda_no = 0;
    int *old_eval_lavars = eval_lavars_used;
    int eval_lavars = false;

    // First, check if this is a lambda expression. "->" must exists.
    ret = get_function_args(&start, '-', NULL, NULL, true);

    if(ret == FAIL || *start != '>')
    {
        return NOTDONE;
    }

    // Parse the arguments again.
    if(evaluate)
    {
        pnewargs = &newargs;
    }
    else
    {
        pnewargs = NULL;
    }

    *arg = skipwhite(*arg + 1);
    ret = get_function_args(arg, '-', pnewargs, &varargs, false);

    if(ret == FAIL || **arg != '>')
    {
        goto errret;
    }

    // Set up a flag for checking local variables and arguments.
    if(evaluate)
    {
        eval_lavars_used = &eval_lavars;
    }

    // Get the start and the end of the expression.
    *arg = skipwhite(*arg + 1);
    s = *arg;
    ret = skip_expr(arg);

    if(ret == FAIL)
    {
        goto errret;
    }

    e = *arg;
    *arg = skipwhite(*arg);

    if(**arg != '}')
    {
        goto errret;
    }

    (*arg)++;

    if(evaluate)
    {
        int len;
        int flags = 0;
        uchar_kt *p;
        uchar_kt name[20];
        partial_st *pt;
        garray_st newlines;

        lambda_no++;
        snprintf((char *)name, sizeof(name), "<lambda>%d", lambda_no);

        fp = (ufunc_st *)xcalloc(1, sizeof(ufunc_st) + ustrlen(name));
        pt = (partial_st *)xcalloc(1, sizeof(partial_st));

        if(pt == NULL)
        {
            xfree(fp);
            goto errret;
        }

        ga_init(&newlines, (int)sizeof(uchar_kt *), 1);
        ga_grow(&newlines, 1);

        // Add "return " before the expression.
        len = 7 + e - s + 1;
        p = (uchar_kt *)xmalloc(len);
        ((uchar_kt **)(newlines.ga_data))[newlines.ga_len++] = p;

        ustrcpy(p, "return ");
        ustrlcpy(p + 7, s, e - s + 1);
        fp->uf_refcount = 1;

        ustrcpy(fp->uf_name, name);
        hash_add(&func_hashtab, UF2HIKEY(fp));

        fp->uf_args = newargs;
        fp->uf_lines = newlines;

        if(current_funccal != NULL && eval_lavars)
        {
            flags |= FC_CLOSURE;
            register_closure(fp);
        }
        else
        {
            fp->uf_scoped = NULL;
        }

        fp->uf_tml_count = NULL;
        fp->uf_tml_total = NULL;
        fp->uf_tml_self = NULL;
        fp->uf_profiling = false;

        if(prof_def_func())
        {
            func_do_profile(fp);
        }

        fp->uf_varargs = true;
        fp->uf_flags = flags;
        fp->uf_calls = 0;
        fp->uf_script_ID = current_SID;

        pt->pt_func = fp;
        pt->pt_refcount = 1;

        rettv->vval.v_partial = pt;
        rettv->v_type = kNvarPartial;
    }

    eval_lavars_used = old_eval_lavars;

    return OK;

errret:

    ga_clear_strings(&newargs);
    xfree(fp);
    eval_lavars_used = old_eval_lavars;

    return FAIL;
}

/// Convert the string to a floating point number
///
/// This uses strtod(). setlocale(LC_NUMERIC, "C") has been used earlier to
/// make sure this always uses a decimal point.
///
/// @param[in]  text        String to convert.
/// @param[out]  ret_value  Location where conversion result is saved.
///
/// @return Length of the text that was consumed.
size_t string2float(const char *const text, float_kt *const ret_value)
FUNC_ATTR_NONNULL_ALL
{
    char *s = NULL;
    *ret_value = strtod(text, &s);
    return (size_t)(s - text);
}

/// Get the value of an environment variable.
///
/// If the environment variable was not set, silently assume it is empty.
///
/// @param arg Points to the '$'. It is advanced to after the name.
/// @return FAIL if the name is invalid.
///
static int get_env_tv(uchar_kt **arg, typval_st *rettv, int evaluate)
{
    uchar_kt *name;
    uchar_kt *string = NULL;
    int len;
    int cc;
    ++*arg;
    name = *arg;
    len = get_env_len((const uchar_kt **)arg);

    if(evaluate)
    {
        if(len == 0)
        {
            return FAIL; // Invalid empty name.
        }

        cc = name[len];
        name[len] = NUL;

        // First try vim_getenv(), fast for normal environment vars.
        string = (uchar_kt *)vim_getenv((char *)name);

        if(string == NULL || *string == NUL)
        {
            xfree(string);

            // Next try expanding things like $VIM and ${HOME}.
            string = expand_env_save(name - 1);

            if(string != NULL && *string == '$')
            {
                xfree(string);
                string = NULL;
            }
        }

        name[len] = cc;
        rettv->v_type = kNvarString;
        rettv->vval.v_string = string;
    }

    return OK;
}

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "funcs.generated.h"
#endif

/// Function given to ExpandGeneric() to obtain the list of internal
/// or user defined function names.
uchar_kt *get_function_name(expand_st *xp, int idx)
{
    static int intidx = -1;
    uchar_kt *name;

    if(idx == 0)
    {
        intidx = -1;
    }

    if(intidx < 0)
    {
        name = get_user_func_name(xp, idx);

        if(name != NULL)
        {
            return name;
        }
    }

    while((size_t)++intidx < ARRAY_SIZE(functions)
          && functions[intidx].name[0] == '\0')
    { /* empty body */ }

    if((size_t)intidx >= ARRAY_SIZE(functions))
    {
        return NULL;
    }

    const char *const key = functions[intidx].name;
    const size_t key_len = strlen(key);

    memcpy(IObuff, key, key_len);
    IObuff[key_len] = '(';

    if(functions[intidx].max_argc == 0)
    {
        IObuff[key_len + 1] = ')';
        IObuff[key_len + 2] = NUL;
    }
    else
    {
        IObuff[key_len + 1] = NUL;
    }

    return IObuff;
}

/// Function given to ExpandGeneric() to obtain the list of internal or
/// user defined variable or function names.
uchar_kt *get_expr_name(expand_st *xp, int idx)
{
    static int intidx = -1;
    uchar_kt *name;

    if(idx == 0)
    {
        intidx = -1;
    }

    if(intidx < 0)
    {
        name = get_function_name(xp, idx);

        if(name != NULL)
        {
            return name;
        }
    }

    return get_user_var_name(xp, ++intidx);
}

/// Find internal function in hash functions
///
/// @param[in]  name  Name of the function.
///
/// Returns pointer to the function definition or NULL if not found.
static const nviml_func_st *find_internal_func(const char *const name)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    size_t len = strlen(name);
    return find_internal_func_gperf(name, len);
}

/// Return name of the function corresponding to `name`
///
/// If `name` points to variable that is either a function or partial then
/// corresponding function name is returned. Otherwise it returns `name` itself.
///
/// @param[in]      name
/// Function name to check.
///
/// @param[in,out]  lenp
/// Location where length of the returned name is stored.
/// Must be set to the length of the `name` argument.
///
/// @param[out]  partialp
/// Location where partial will be stored if found
/// function appears to be a partial. May be NULL if this is not needed.
///
/// @param[in]  no_autoload
/// If true, do not source autoload scripts if function was not found.
///
/// @return name of the function.
static uchar_kt *deref_func_name(const char *name,
                               int *lenp,
                               partial_st **const partialp,
                               bool no_autoload)
FUNC_ATTR_NONNULL_ARG(1, 2)
{
    if(partialp != NULL)
    {
        *partialp = NULL;
    }

    dictitem_st *const v = find_var(name, (size_t)(*lenp), NULL, no_autoload);

    if(v != NULL && v->di_tv.v_type == kNvarUfunc)
    {
        if(v->di_tv.vval.v_string == NULL) // just in case
        {
            *lenp = 0;
            return (uchar_kt *)"";
        }

        *lenp = (int)ustrlen(v->di_tv.vval.v_string);
        return v->di_tv.vval.v_string;
    }

    if(v != NULL && v->di_tv.v_type == kNvarPartial)
    {
        partial_st *const pt = v->di_tv.vval.v_partial;

        if(pt == NULL) // just in case
        {
            *lenp = 0;
            return (uchar_kt *)"";
        }

        if(partialp != NULL)
        {
            *partialp = pt;
        }

        uchar_kt *s = partial_name(pt);
        *lenp = (int)ustrlen(s);
        return s;
    }

    return (uchar_kt *)name;
}

/// Allocate a variable for the result of a function.
///
/// @param name        name of the function
/// @param len         length of @b name
/// @param rettv
/// @param arg         argument, pointing to the '('
/// @param firstline   first line of range
/// @param lastline    last line of range
/// @param doesrange   return: function handled range
/// @param evaluate
/// @param partial     for extra arguments
/// @param selfdict    Dictionary for "self"
///
/// @return OK or FAIL.
static int get_func_tv(uchar_kt *name,
                       int len,
                       typval_st *rettv,
                       uchar_kt **arg,
                       linenum_kt firstline,
                       linenum_kt lastline,
                       int *doesrange,
                       int evaluate,
                       partial_st *partial,
                       dict_st *selfdict)
{
    int ret = OK;
    int argcount = 0; // number of arguments found
    uchar_kt *argp = *arg; // Get the arguments.
    typval_st argvars[MAX_FUNC_ARGS + 1]; // vars for arguments

    while(argcount < MAX_FUNC_ARGS - (partial == NULL ? 0 : partial->pt_argc))
    {
        argp = skipwhite(argp + 1); // skip the '(' or ','

        if(*argp == ')' || *argp == ',' || *argp == NUL)
        {
            break;
        }

        if(eval_lev_1(&argp, &argvars[argcount], evaluate) == FAIL)
        {
            ret = FAIL;
            break;
        }

        ++argcount;

        if(*argp != ',')
        {
            break;
        }
    }

    if(*argp == ')')
    {
        ++argp;
    }
    else
    {
        ret = FAIL;
    }

    if(ret == OK)
    {
        int i = 0;

        if(get_vim_var_nr(VV_TESTING))
        {
            // Prepare for calling garbagecollect_for_testing(), need to know
            // what variables are used on the call stack.
            if(funcargs.ga_itemsize == 0)
            {
                ga_init(&funcargs, (int)sizeof(typval_st *), 50);
            }

            for(i = 0; i < argcount; i++)
            {
                ga_grow(&funcargs, 1);
                ((typval_st **)funcargs.ga_data)[funcargs.ga_len++] = &argvars[i];
            }
        }

        ret = call_func(name,
                        len,
                        rettv,
                        argcount,
                        argvars,
                        NULL,
                        firstline,
                        lastline,
                        doesrange,
                        evaluate,
                        partial,
                        selfdict);

        funcargs.ga_len -= i;
    }
    else if(!aborting())
    {
        if(argcount == MAX_FUNC_ARGS)
        {
            emsg_funcname(N_("E740: Too many arguments for function %s"), name);
        }
        else
        {
            emsg_funcname(N_("E116: Invalid arguments for function %s"), name);
        }
    }

    while(--argcount >= 0)
    {
        tv_clear(&argvars[argcount]);
    }

    *arg = skipwhite(argp);
    return ret;
}

typedef enum
{
    ERROR_UNKNOWN = 0,
    ERROR_TOOMANY,
    ERROR_TOOFEW,
    ERROR_SCRIPT,
    ERROR_DICT,
    ERROR_NONE,
    ERROR_OTHER,
    ERROR_BOTH,
    ERROR_DELETED,
} FnameTransError;

#define FLEN_FIXED 40

/// In a script transform script-local names into actually used names
///
/// Transforms "<SID>" and "s:" prefixes to `K_SNR {N}` (e.g. K_SNR "123") and
/// "<SNR>" prefix to `K_SNR`. Uses `fname_buf` buffer that is supposed to have
/// #FLEN_FIXED + 1 length when it fits, otherwise it allocates memory.
///
/// @param[in]  name     Name to transform.
/// @param  fname_buf    Buffer to save resulting function name to, if it fits.
///                      Must have at least #FLEN_FIXED + 1 length.
/// @param[out]  tofree  Location where pointer to an allocated memory is saved
///                      in case result does not fit into fname_buf.
/// @param[out]  error   Location where error type is saved, @see
///                      FnameTransError.
///
/// @return transformed name: either `fname_buf` or a pointer to an allocated
///         memory.
static uchar_kt *fname_trans_sid(const uchar_kt *const name,
                               uchar_kt *const fname_buf,
                               uchar_kt **const tofree,
                               int *const error)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    uchar_kt *fname;
    const int llen = eval_fname_script((const char *)name);

    if(llen > 0)
    {
        fname_buf[0] = K_SPECIAL;
        fname_buf[1] = KS_EXTRA;
        fname_buf[2] = (int)KE_SNR;
        int i = 3;

        if(eval_fname_sid((const char *)name)) // "<SID>" or "s:"
        {
            if(current_SID <= 0)
            {
                *error = ERROR_SCRIPT;
            }
            else
            {
                snprintf((char *)fname_buf + 3,
                         FLEN_FIXED + 1,
                         "%" PRId64 "_",
                         (int64_t)current_SID);

                i = (int)ustrlen(fname_buf);
            }
        }

        if(i + ustrlen(name + llen) < FLEN_FIXED)
        {
            ustrcpy(fname_buf + i, name + llen);
            fname = fname_buf;
        }
        else
        {
            fname = xmalloc(i + ustrlen(name + llen) + 1);

            if(fname == NULL)
            {
                *error = ERROR_OTHER;
            }
            else
            {
                *tofree = fname;
                memmove(fname, fname_buf, (size_t)i);
                ustrcpy(fname + i, name + llen);
            }
        }
    }
    else
    {
        fname = (uchar_kt *)name;
    }

    return fname;
}

/// Mark all lists and dicts referenced through function "name" with "copyID".
/// "list_stack" is used to add lists to be marked. Can be NULL.
/// "ht_stack" is used to add hashtabs to be marked. Can be NULL.
///
/// @return true if setting references failed somehow.
bool set_ref_in_func(uchar_kt *name, ufunc_st *fp_in, int copyID)
{
    ufunc_st *fp = fp_in;
    func_call_st *fc;
    int error = ERROR_NONE;
    uchar_kt fname_buf[FLEN_FIXED + 1];
    uchar_kt *tofree = NULL;
    uchar_kt *fname;
    bool abort = false;

    if(name == NULL && fp_in == NULL)
    {
        return false;
    }

    if(fp_in == NULL)
    {
        fname = fname_trans_sid(name, fname_buf, &tofree, &error);
        fp = find_func(fname);
    }

    if(fp != NULL)
    {
        for(fc = fp->uf_scoped; fc != NULL; fc = fc->func->uf_scoped)
        {
            abort = abort || set_ref_in_funccal(fc, copyID);
        }
    }

    xfree(tofree);
    return abort;
}

/// Call a function with its resolved parameters
///
/// @param funcname
/// name of the function
///
/// @param len
/// length of @b funcname
///
/// @param rettv
/// return value goes here
///
/// @param argcount_in
/// number of "argvars"
///
/// @param argvars_in
/// vars for arguments, must have "argcount" (PLUS ONE elements!)
///
/// @param argv_func
/// function to fill in argvars
///
/// @param firstline
/// first line of range
///
/// @param lastline
/// last line of range
///
/// @param doesrange
/// return: function handled range
///
/// @param evaluate
///
/// @param partial
/// optional, can be NULL
///
/// @param selfdict_in
/// Dictionary for "self"
///
/// @b argv_func, when not NULL, can be used to fill in arguments
/// only when the invoked function uses them. It is called like this:
/// new_argcount = argv_func(current_argcount, argv, called_func_argcount)
///
/// @return
/// FAIL when the function can't be called, OK otherwise.
/// Also returns OK when an error was encountered while executing the function.
int call_func(const uchar_kt *funcname,
              int len,
              typval_st *rettv,
              int argcount_in,
              typval_st *argvars_in,
              argv_func_ft argv_func,
              linenum_kt firstline,
              linenum_kt lastline,
              int *doesrange,
              bool evaluate,
              partial_st *partial,
              dict_st *selfdict_in)
{
    int ret = FAIL;
    int error = ERROR_NONE;
    ufunc_st *fp;
    uchar_kt fname_buf[FLEN_FIXED + 1];
    uchar_kt *tofree = NULL;
    uchar_kt *fname;
    uchar_kt *name;
    int argcount = argcount_in;
    typval_st *argvars = argvars_in;
    dict_st *selfdict = selfdict_in;
    typval_st argv[MAX_FUNC_ARGS + 1]; // used when "partial" is not NULL
    int argv_clear = 0;

    // Make a copy of the name, if it comes from a funcref variable it could
    // be changed or deleted in the called function.
    name = ustrndup(funcname, len);

    if(name == NULL)
    {
        return ret;
    }

    fname = fname_trans_sid(name, fname_buf, &tofree, &error);
    *doesrange = false;

    if(partial != NULL)
    {
        // When the function has a partial with a dict and there is a dict
        // argument, use the dict argument. That is backwards compatible.
        // When the dict was bound explicitly use the one from the partial.
        if(partial->pt_dict != NULL
           && (selfdict_in == NULL || !partial->pt_auto))
        {
            selfdict = partial->pt_dict;
        }

        if(error == ERROR_NONE && partial->pt_argc > 0)
        {
            for(argv_clear = 0; argv_clear < partial->pt_argc; argv_clear++)
            {
                tv_copy(&partial->pt_argv[argv_clear], &argv[argv_clear]);
            }

            for(int i = 0; i < argcount_in; i++)
            {
                argv[i + argv_clear] = argvars_in[i];
            }

            argvars = argv;
            argcount = partial->pt_argc + argcount_in;
        }
    }

    // execute the function if no errors detected and executing
    if(evaluate && error == ERROR_NONE)
    {
        uchar_kt *rfname = fname;

        // Ignore "g:" before a function name.
        if(fname[0] == 'g' && fname[1] == ':')
        {
            rfname = fname + 2;
        }

        rettv->v_type = kNvarNumber; // default rettv is number zero
        rettv->vval.v_number = 0;
        error = ERROR_UNKNOWN;

        if(!builtin_function((const char *)rfname, -1))
        {
            // User defined function.
            if(partial != NULL && partial->pt_func != NULL)
            {
                fp = partial->pt_func;
            }
            else
            {
                fp = find_func(rfname);
            }

            // Trigger FuncUndefined event, may load the function.
            if(fp == NULL
               && apply_autocmds(EVENT_FUNCUNDEFINED, rfname, rfname, TRUE, NULL)
               && !aborting())
            {
                // executed an autocommand, search for the function again
                fp = find_func(rfname);
            }

            // Try loading a package.
            if(fp == NULL && script_autoload((const char *)rfname,
                                             ustrlen(rfname),
                                             true) && !aborting())
            {
                // Loaded a package, search for the function again.
                fp = find_func(rfname);
            }

            if(fp != NULL && (fp->uf_flags & FC_DELETED))
            {
                error = ERROR_DELETED;
            }
            else if(fp != NULL)
            {
                if(argv_func != NULL)
                {
                    argcount = argv_func(argcount, argvars, fp->uf_args.ga_len);
                }

                if(fp->uf_flags & FC_RANGE)
                {
                    *doesrange = true;
                }

                if(argcount < fp->uf_args.ga_len)
                {
                    error = ERROR_TOOFEW;
                }
                else if(!fp->uf_varargs && argcount > fp->uf_args.ga_len)
                {
                    error = ERROR_TOOMANY;
                }
                else if((fp->uf_flags & FC_DICT) && selfdict == NULL)
                {
                    error = ERROR_DICT;
                }
                else
                {
                    // Call the user function.
                    call_user_func(fp,
                                   argcount,
                                   argvars,
                                   rettv,
                                   firstline,
                                   lastline,
                                   (fp->uf_flags & FC_DICT) ? selfdict : NULL);

                    error = ERROR_NONE;
                }
            }
        }
        else
        {
            // Find the function name in the table, call its implementation.
            const nviml_func_st *const fdef =
                find_internal_func((const char *)fname);

            if(fdef != NULL)
            {
                if(argcount < fdef->min_argc)
                {
                    error = ERROR_TOOFEW;
                }
                else if(argcount > fdef->max_argc)
                {
                    error = ERROR_TOOMANY;
                }
                else
                {
                    argvars[argcount].v_type = kNvarUnknown;
                    fdef->func(argvars, rettv, fdef->data);
                    error = ERROR_NONE;
                }
            }
        }

        // The function call (or "FuncUndefined" autocommand sequence) might
        // have been aborted by an error, an interrupt, or an explicitly thrown
        // exception that has not been caught so far.  This situation can be
        // tested for by calling aborting().  For an error in an internal
        // function or for the "E132" error in call_user_func(), however, the
        // throw point at which the "force_abort" flag (temporarily reset by
        // emsg()) is normally updated has not been reached yet. We need to
        // update that flag first to make aborting() reliable.
        update_force_abort();
    }

    if(error == ERROR_NONE)
    {
        ret = OK;
    }

    // Report an error unless the argument evaluation or function call has been
    // cancelled due to an aborting error, an interrupt, or an exception.
    if(!aborting())
    {
        switch(error)
        {
            case ERROR_UNKNOWN:
                emsg_funcname(N_("E117: Unknown function: %s"), name);
                break;

            case ERROR_DELETED:
                emsg_funcname(N_("E933: Function was deleted: %s"), name);
                break;

            case ERROR_TOOMANY:
                emsg_funcname(e_toomanyarg, name);
                break;

            case ERROR_TOOFEW:
                emsg_funcname(N_("E119: Not enough arguments for function: %s"), name);
                break;

            case ERROR_SCRIPT:
                emsg_funcname(N_("E120: Using <SID> not in a script context: %s"), name);
                break;

            case ERROR_DICT:
                emsg_funcname(N_("E725: Calling dict function without Dictionary: %s"), name);
                break;
        }
    }

    while(argv_clear > 0)
    {
        tv_clear(&argv[--argv_clear]);
    }

    xfree(tofree);
    xfree(name);
    return ret;
}

/// Give an error message with a function name. Handle <SNR> things.
/// "ermsg" is to be passed without translation, use N_() instead of _().
static void emsg_funcname(char *ermsg, uchar_kt *name)
{
    uchar_kt *p;

    if(*name == K_SPECIAL)
    {
        p = ustrdup_concat((uchar_kt *)"<SNR>", name + 3);
    }
    else
    {
        p = name;
    }

    EMSG2(_(ermsg), p);

    if(p != name)
    {
        xfree(p);
    }
}

/// Return TRUE for a non-zero Number and a non-empty String.
static int non_zero_arg(typval_st *argvars)
{
    return ((argvars[0].v_type == kNvarNumber
             && argvars[0].vval.v_number != 0)
            || (argvars[0].v_type == kNvarSpecial
                && argvars[0].vval.v_special == kSpecialVarTrue)
            || (argvars[0].v_type == kNvarString
                && argvars[0].vval.v_string != NULL
                && *argvars[0].vval.v_string != NUL));
}

///////////////////////////////////////////////////////////////////////////////
////////////////// Implementation of the built-in functions //////////////////
///////////////////////////////////////////////////////////////////////////////

/// Apply a floating point C function on a typval with one float_kt.
///
/// Some versions of glibc on i386 have an optimization that makes it harder to
/// call math functions indirectly from inside an inlined function, causing
/// compile-time errors. Avoid `inline` in that case. #3072
static void float_op_wrapper(typval_st *argvars, typval_st *rettv, func_ptr_ft fptr)
{
    float_kt f;
    float_kt(*function)(float_kt) = (float_kt(*)(float_kt))fptr;
    rettv->v_type = kNvarFloat;

    if(tv_get_float_chk(argvars, &f))
    {
        rettv->vval.v_float = function(f);
    }
    else
    {
        rettv->vval.v_float = 0.0;
    }
}

static void api_wrapper(typval_st *argvars, typval_st *rettv, func_ptr_ft fptr)
{
    api_dispatch_ft fn = (api_dispatch_ft)fptr;
    Array args = ARRAY_DICT_INIT;

    for(typval_st *tv = argvars; tv->v_type != kNvarUnknown; tv++)
    {
        ADD(args, nvim_to_object(tv));
    }

    error_st err = ERROR_INIT;
    Object result = fn(VIML_INTERNAL_CALL, args, &err);

    if(ERROR_SET(&err))
    {
        nvim_errmsg_writeln(cstr_as_string(err.msg));
        goto end;
    }

    if(!object_to_vim(result, rettv, &err))
    {
        EMSG2(_("Error converting the call result: %s"), err.msg);
    }

end:

    api_free_array(args);
    api_free_object(result);
    api_clear_error(&err);
}

/// "abs(expr)" function
static void f_abs(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[0].v_type == kNvarFloat)
    {
        float_op_wrapper(argvars, rettv, (func_ptr_ft)&fabs);
    }
    else
    {
        number_kt n;
        bool error = false;
        n = tv_get_number_chk(&argvars[0], &error);

        if(error)
        {
            rettv->vval.v_number = -1;
        }
        else if(n > 0)
        {
            rettv->vval.v_number = n;
        }
        else
        {
            rettv->vval.v_number = -n;
        }
    }
}

/// "add(list, item)" function
static void f_add(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    list_st *l;
    rettv->vval.v_number = 1; // Default: Failed

    if(argvars[0].v_type == kNvarList)
    {
        if((l = argvars[0].vval.v_list) != NULL
           && !tv_check_lock(l->lv_lock, "add() argument", TV_TRANSLATE))
        {
            tv_list_append_tv(l, &argvars[1]);
            tv_copy(&argvars[0], rettv);
        }
    }
    else
    {
        EMSG(_(e_listreq));
    }
}

/// "and(expr, expr)" function
static void f_and(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = tv_get_number_chk(&argvars[0], NULL)
                           & tv_get_number_chk(&argvars[1], NULL);
}


/// "api_info()" function
static void f_api_info(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    Dictionary metadata = api_metadata();
    (void)object_to_vim(DICTIONARY_OBJ(metadata), rettv, NULL);
    api_free_dictionary(metadata);
}

/// "append(lnum, string/list)" function
static void f_append(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    long lnum;
    list_st *l = NULL;
    listitem_st *li = NULL;
    typval_st *tv;
    long added = 0;

    // When coming here from Insert mode, sync undo, so that this can be
    // undone separately from what was previously inserted.
    if(u_sync_once == 2)
    {
        u_sync_once = 1; // notify that u_sync() was called
        u_sync(TRUE);
    }

    lnum = tv_get_lnum(argvars);

    if(lnum >= 0
       && lnum <= curbuf->b_ml.ml_line_count
       && u_save(lnum, lnum + 1) == OK)
    {
        if(argvars[1].v_type == kNvarList)
        {
            l = argvars[1].vval.v_list;

            if(l == NULL)
            {
                return;
            }

            li = l->lv_first;
        }

        for(;;)
        {
            if(l == NULL)
            {
                tv = &argvars[1]; // Append a string.
            }
            else if(li == NULL)
            {
                break; // End of list.
            }
            else
            {
                tv = &li->li_tv; // Append item from list.
            }

            const char *const line = tv_get_string_chk(tv);

            if(line == NULL) // Type error.
            {
                rettv->vval.v_number = 1;  // Failed.
                break;
            }

            ml_append(lnum + added, (uchar_kt *)line, (columnum_kt)0, false);
            added++;

            if(l == NULL)
            {
                break;
            }

            li = li->li_next;
        }

        appended_lines_mark(lnum, added);

        if(curwin->w_cursor.lnum > lnum)
        {
            curwin->w_cursor.lnum += added;
        }
    }
    else
    {
        rettv->vval.v_number = 1; // Failed
    }
}

/// "argc()" function
static void f_argc(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = carg_cnt;
}

/// "argidx()" function
static void f_argidx(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = curwin->w_arg_idx;
}

/// "arglistid" function
static void f_arglistid(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = -1;
    win_st *wp = find_tabwin(&argvars[0], &argvars[1]);

    if(wp != NULL)
    {
        rettv->vval.v_number = wp->w_alist->id;
    }
}

/// "argv(nr)" function
static void f_argv(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int idx;

    if(argvars[0].v_type != kNvarUnknown)
    {
        idx = tv_get_number_chk(&argvars[0], NULL);

        if(idx >= 0 && idx < carg_cnt)
        {
            rettv->vval.v_string = ustrdup(alist_name(&carg_list[idx]));
        }
        else
        {
            rettv->vval.v_string = NULL;
        }

        rettv->v_type = kNvarString;
    }
    else
    {
        tv_list_alloc_ret(rettv);

        for(idx = 0; idx < carg_cnt; idx++)
        {
            tv_list_append_string(rettv->vval.v_list,
                                  (const char *)alist_name(&carg_list[idx]), -1);
        }
    }
}

/// Prepare "gap" for an assert error and add the sourcing position.
static void prepare_assert_error(garray_st *gap)
{
    char buf[NUMBUFLEN];
    ga_init(gap, 1, 100);

    if(sourcing_name != NULL)
    {
        ga_concat(gap, sourcing_name);

        if(sourcing_lnum > 0)
        {
            ga_concat(gap, (uchar_kt *)" ");
        }
    }

    if(sourcing_lnum > 0)
    {
        xsnprintf(buf, ARRAY_SIZE(buf),
                  "line %" PRId64, (int64_t)sourcing_lnum);

        ga_concat(gap, (uchar_kt *)buf);
    }

    if(sourcing_name != NULL || sourcing_lnum > 0)
    {
        ga_concat(gap, (uchar_kt *)": ");
    }
}

/// Fill "gap" with information about an assert error.
static void fill_assert_error(garray_st *gap,
                              typval_st *opt_msg_tv,
                              uchar_kt *exp_str,
                              typval_st *exp_tv,
                              typval_st *got_tv,
                              assert_type_et atype)
{
    uchar_kt *tofree;

    if(opt_msg_tv->v_type != kNvarUnknown)
    {
        tofree = (uchar_kt *) encode_tv2string(opt_msg_tv, NULL);
        ga_concat(gap, tofree);
        xfree(tofree);
    }
    else
    {
        if(atype == ASSERT_MATCH || atype == ASSERT_NOTMATCH)
        {
            ga_concat(gap, (uchar_kt *)"Pattern ");
        }
        else
        {
            ga_concat(gap, (uchar_kt *)"Expected ");
        }

        if(exp_str == NULL)
        {
            tofree = (uchar_kt *) encode_tv2string(exp_tv, NULL);
            ga_concat(gap, tofree);
            xfree(tofree);
        }
        else
        {
            ga_concat(gap, exp_str);
        }

        tofree = (uchar_kt *)encode_tv2string(got_tv, NULL);

        if(atype == ASSERT_MATCH)
        {
            ga_concat(gap, (uchar_kt *)" does not match ");
        }
        else if(atype == ASSERT_NOTMATCH)
        {
            ga_concat(gap, (uchar_kt *)" does match ");
        }
        else if(atype == ASSERT_NOTEQUAL)
        {
            ga_concat(gap, (uchar_kt *)" differs from ");
        }
        else
        {
            ga_concat(gap, (uchar_kt *)" but got ");
        }

        ga_concat(gap, tofree);
        xfree(tofree);
    }
}

/// Add an assert error to v:errors.
static void assert_error(garray_st *gap)
{
    vimvar_st *vp = &vimvars[VV_ERRORS];

    if(vp->vv_type != kNvarList || vimvars[VV_ERRORS].vv_list == NULL)
    {
        // Make sure v:errors is a list.
        set_vim_var_list(VV_ERRORS, tv_list_alloc());
    }

    tv_list_append_string(vimvars[VV_ERRORS].vv_list,
                          (const char *)gap->ga_data,
                          (ptrdiff_t)gap->ga_len);
}

static void assert_equal_common(typval_st *argvars, assert_type_et atype)
{
    garray_st ga;

    if(tv_equal(&argvars[0], &argvars[1], false, false) != (atype == ASSERT_EQUAL))
    {
        prepare_assert_error(&ga);

        fill_assert_error(&ga, &argvars[2], NULL,
                          &argvars[0], &argvars[1], atype);

        assert_error(&ga);
        ga_clear(&ga);
    }
}

/// "assert_equal(expected, actual[, msg])" function
static void f_assert_equal(typval_st *argvars,
                           typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    assert_equal_common(argvars, ASSERT_EQUAL);
}

/// "assert_notequal(expected, actual[, msg])" function
static void f_assert_notequal(typval_st *argvars,
                              typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                              func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    assert_equal_common(argvars, ASSERT_NOTEQUAL);
}

/// "assert_exception(string[, msg])" function
static void f_assert_exception(typval_st *argvars,
                               typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                               func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    garray_st ga;
    const char *const error = tv_get_string_chk(&argvars[0]);

    if(vimvars[VV_EXCEPTION].vv_str == NULL)
    {
        prepare_assert_error(&ga);
        ga_concat(&ga, (uchar_kt *)"v:exception is not set");
        assert_error(&ga);
        ga_clear(&ga);
    }
    else if(error != NULL
            && strstr((char *)vimvars[VV_EXCEPTION].vv_str, error) == NULL)
    {
        prepare_assert_error(&ga);

        fill_assert_error(&ga,
                          &argvars[1],
                          NULL,
                          &argvars[0],
                          &vimvars[VV_EXCEPTION].vv_tv,
                          ASSERT_OTHER);

        assert_error(&ga);
        ga_clear(&ga);
    }
}

/// "assert_fails(cmd [, error])" function
static void f_assert_fails(typval_st *argvars,
                           typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const cmd = tv_get_string_chk(&argvars[0]);
    garray_st ga;
    called_emsg = false;
    suppress_errthrow = true;
    emsg_silent = true;
    do_cmdline_cmd(cmd);

    if(!called_emsg)
    {
        prepare_assert_error(&ga);
        ga_concat(&ga, (const uchar_kt *)"command did not fail: ");
        ga_concat(&ga, (const uchar_kt *)cmd);
        assert_error(&ga);
        ga_clear(&ga);
    }
    else if(argvars[1].v_type != kNvarUnknown)
    {
        char buf[NUMBUFLEN];
        const char *const error = tv_get_string_buf_chk(&argvars[1], buf);

        if(error == NULL
           || strstr((char *)vimvars[VV_ERRMSG].vv_str, error) == NULL)
        {
            prepare_assert_error(&ga);

            fill_assert_error(&ga,
                              &argvars[2],
                              NULL,
                              &argvars[1],
                              &vimvars[VV_ERRMSG].vv_tv,
                              ASSERT_OTHER);

            assert_error(&ga);
            ga_clear(&ga);
        }
    }

    called_emsg = false;
    suppress_errthrow = false;
    emsg_silent = false;
    emsg_on_display = false;
    set_vim_var_string(VV_ERRMSG, NULL, 0);
}

void assert_inrange(typval_st *argvars)
{
    bool error = false;
    const number_kt lower = tv_get_number_chk(&argvars[0], &error);
    const number_kt upper = tv_get_number_chk(&argvars[1], &error);
    const number_kt actual = tv_get_number_chk(&argvars[2], &error);

    if(error)
    {
        return;
    }

    if(actual < lower || actual > upper)
    {
        garray_st ga;
        prepare_assert_error(&ga);
        char msg[55];

        xsnprintf(msg, sizeof(msg),
                  "range %" NumberKtPrtFmt " - %" NumberKtPrtFmt ",",
                  lower, upper);

        fill_assert_error(&ga,
                          &argvars[3],
                          (uchar_kt *)msg,
                          NULL,
                          &argvars[2],
                          ASSERT_INRANGE);

        assert_error(&ga);
        ga_clear(&ga);
    }
}

/// Common for assert_true() and assert_false().
static void assert_bool(typval_st *argvars, bool is_true)
{
    bool error = false;
    garray_st ga;

    if((argvars[0].v_type != kNvarNumber
        || (tv_get_number_chk(&argvars[0], &error) == 0) == is_true
        || error)
       && (argvars[0].v_type != kNvarSpecial
           || (argvars[0].vval.v_special
               != (nvlvar_special_value_et) (is_true
                                              ? kSpecialVarTrue
                                              : kSpecialVarFalse))))
    {
        prepare_assert_error(&ga);

        fill_assert_error(&ga,
                          &argvars[1],
                          (uchar_kt *)(is_true ? "True" : "False"),
                          NULL,
                          &argvars[0],
                          ASSERT_OTHER);

        assert_error(&ga);
        ga_clear(&ga);
    }
}

/// "assert_false(actual[, msg])" function
static void f_assert_false(typval_st *argvars,
                           typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    assert_bool(argvars, false);
}

static void assert_match_common(typval_st *argvars, assert_type_et atype)
{
    char buf1[NUMBUFLEN];
    char buf2[NUMBUFLEN];
    const char *const pat = tv_get_string_buf_chk(&argvars[0], buf1);
    const char *const text = tv_get_string_buf_chk(&argvars[1], buf2);

    if(pat == NULL || text == NULL)
    {
        EMSG(_(e_invarg));
    }
    else if(pattern_match((uchar_kt *)pat, (uchar_kt *)text, false)
            != (atype == ASSERT_MATCH))
    {
        garray_st ga;
        prepare_assert_error(&ga);

        fill_assert_error(&ga, &argvars[2], NULL,
                          &argvars[0], &argvars[1], atype);

        assert_error(&ga);
        ga_clear(&ga);
    }
}

/// "assert_inrange(lower, upper[, msg])" function
static void f_assert_inrange(typval_st *argvars,
                             typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                             func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    assert_inrange(argvars);
}

/// "assert_match(pattern, actual[, msg])" function
static void f_assert_match(typval_st *argvars,
                           typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    assert_match_common(argvars, ASSERT_MATCH);
}

/// "assert_notmatch(pattern, actual[, msg])" function
static void f_assert_notmatch(typval_st *argvars,
                              typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                              func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    assert_match_common(argvars, ASSERT_NOTMATCH);
}

// "assert_true(actual[, msg])" function
static void f_assert_true(typval_st *argvars,
                          typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    assert_bool(argvars, true);
}

/// "atan2()" function
static void f_atan2(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    float_kt fx;
    float_kt fy;
    rettv->v_type = kNvarFloat;

    if(tv_get_float_chk(argvars, &fx) && tv_get_float_chk(&argvars[1], &fy))
    {
        rettv->vval.v_float = atan2(fx, fy);
    }
    else
    {
        rettv->vval.v_float = 0.0;
    }
}

/// "browse(save, title, initdir, default)" function
static void f_browse(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_string = NULL;
    rettv->v_type = kNvarString;
}

/// "browsedir(title, initdir)" function
static void f_browsedir(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    f_browse(argvars, rettv, NULL);
}


/// Find a buffer by number or exact name.
static filebuf_st *find_buffer(typval_st *avar)
{
    filebuf_st *buf = NULL;

    if(avar->v_type == kNvarNumber)
    {
        buf = buflist_findnr((int)avar->vval.v_number);
    }
    else if(avar->v_type == kNvarString && avar->vval.v_string != NULL)
    {
        buf = buflist_findname_exp(avar->vval.v_string);

        if(buf == NULL)
        {
            // No full path name match, try a match with a URL
            // or a "nofile" buffer, these don't use the full path.
            FOR_ALL_BUFFERS(bp)
            {
                if(bp->b_fname != NULL
                   && (path_with_url((char *)bp->b_fname) || bt_nofile(bp))
                   && ustrcmp(bp->b_fname, avar->vval.v_string) == 0)
                {
                    buf = bp;
                    break;
                }
            }
        }
    }

    return buf;
}

/// "bufexists(expr)" function
static void f_bufexists(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = (find_buffer(&argvars[0]) != NULL);
}

/// "buflisted(expr)" function
static void f_buflisted(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    filebuf_st *buf;
    buf = find_buffer(&argvars[0]);
    rettv->vval.v_number = (buf != NULL && buf->b_p_bl);
}

/// "bufloaded(expr)" function
static void f_bufloaded(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    filebuf_st *buf;
    buf = find_buffer(&argvars[0]);
    rettv->vval.v_number = (buf != NULL && buf->b_ml.ml_mfp != NULL);
}

/// Get buffer by number or pattern.
static filebuf_st *get_buf_tv(typval_st *tv, int curtab_only)
{
    uchar_kt *name = tv->vval.v_string;
    int save_magic;
    uchar_kt *save_cpo;
    filebuf_st *buf;

    if(tv->v_type == kNvarNumber)
    {
        return buflist_findnr((int)tv->vval.v_number);
    }

    if(tv->v_type != kNvarString)
    {
        return NULL;
    }

    if(name == NULL || *name == NUL)
    {
        return curbuf;
    }

    if(name[0] == '$' && name[1] == NUL)
    {
        return lastbuf;
    }

    // Ignore 'magic' and 'cpoptions' here to make scripts portable
    save_magic = p_magic;
    p_magic = TRUE;
    save_cpo = p_cpo;
    p_cpo = (uchar_kt *)"";

    buf = buflist_findnr(buflist_findpat(name,
                                         name + ustrlen(name),
                                         TRUE,
                                         FALSE,
                                         curtab_only));
    p_magic = save_magic;
    p_cpo = save_cpo;

    // If not found, try expanding the name, like done for bufexists().
    if(buf == NULL)
    {
        buf = find_buffer(tv);
    }

    return buf;
}

/// "bufname(expr)" function
static void f_bufname(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    if(!tv_check_str_or_nr(&argvars[0]))
    {
        return;
    }

    emsg_off++;
    const filebuf_st *const buf = get_buf_tv(&argvars[0], false);
    emsg_off--;

    if(buf != NULL && buf->b_fname != NULL)
    {
        rettv->vval.v_string = ustrdup(buf->b_fname);
    }
}

/// "bufnr(expr)" function
static void f_bufnr(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    bool error = false;
    rettv->vval.v_number = -1;

    if(!tv_check_str_or_nr(&argvars[0]))
    {
        return;
    }

    emsg_off++;
    const filebuf_st *buf = get_buf_tv(&argvars[0], false);
    emsg_off--;

    // If the buffer isn't found and the second
    // argument is not zero create a new buffer.
    const char *name;

    if(buf == NULL
       && argvars[1].v_type != kNvarUnknown
       && tv_get_number_chk(&argvars[1], &error) != 0
       && !error
       && (name = tv_get_string_chk(&argvars[0])) != NULL
       && !error)
    {
        buf = buflist_new((uchar_kt *)name, NULL, 1, 0);
    }

    if(buf != NULL)
    {
        rettv->vval.v_number = buf->b_id;
    }
}

static void buf_win_common(typval_st *argvars, typval_st *rettv, bool get_nr)
{
    if(!tv_check_str_or_nr(&argvars[0]))
    {
        rettv->vval.v_number = -1;
        return;
    }

    emsg_off++;
    filebuf_st *buf = get_buf_tv(&argvars[0], true);

    if(buf == NULL) // no need to search if buffer was not found
    {
        rettv->vval.v_number = -1;
        goto end;
    }

    int winnr = 0;
    int winid;
    bool found_buf = false;

    FOR_ALL_WINDOWS_IN_TAB(wp, curtab)
    {
        winnr++;

        if(wp->w_buffer == buf)
        {
            found_buf = true;
            winid = wp->handle;
            break;
        }
    }
    rettv->vval.v_number = (found_buf ? (get_nr ? winnr : winid) : -1);

end:

    emsg_off--;
}

/// "bufwinid(nr)" function
static void f_bufwinid(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    buf_win_common(argvars, rettv, false);
}

/// "bufwinnr(nr)" function
static void f_bufwinnr(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    buf_win_common(argvars, rettv, true);
}

/// "byte2line(byte)" function
static void f_byte2line(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    long boff = tv_get_number(&argvars[0]) - 1;

    if(boff < 0)
    {
        rettv->vval.v_number = -1;
    }
    else
    {
        rettv->vval.v_number =
            (number_kt)ml_find_line_or_offset(curbuf, 0, &boff);
    }
}

static void byteidx(typval_st *argvars, typval_st *rettv, int comp)
{
    const char *const str = tv_get_string_chk(&argvars[0]);
    number_kt idx = tv_get_number_chk(&argvars[1], NULL);
    rettv->vval.v_number = -1;

    if(str == NULL || idx < 0)
    {
        return;
    }

    const char *t = str;

    for(; idx > 0; idx--)
    {
        if(*t == NUL) // EOL reached.
        {
            return;
        }

        if(comp)
        {
            t += utf_ptr2len((const uchar_kt *)t);
        }
        else
        {
            t += (*mb_ptr2len)((const uchar_kt *)t);
        }
    }

    rettv->vval.v_number = (number_kt)(t - str);
}

/// "byteidx()" function
static void f_byteidx(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    byteidx(argvars, rettv, FALSE);
}

/// "byteidxcomp()" function
static void f_byteidxcomp(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    byteidx(argvars, rettv, TRUE);
}

int func_call(uchar_kt *name,
              typval_st *args,
              partial_st *partial,
              dict_st *selfdict,
              typval_st *rettv)
{
    listitem_st *item;
    typval_st argv[MAX_FUNC_ARGS + 1];
    int argc = 0;
    int dummy;
    int r = 0;

    for(item = args->vval.v_list->lv_first; item != NULL; item = item->li_next)
    {
        if(argc == MAX_FUNC_ARGS - (partial == NULL ? 0 : partial->pt_argc))
        {
            EMSG(_("E699: Too many arguments"));
            break;
        }

        // Make a copy of each argument. This is needed to be able to set
        // v_lock to kNvlVarFixed in the copy without changing the original list.
        tv_copy(&item->li_tv, &argv[argc++]);
    }

    if(item == NULL)
    {
        r = call_func(name,
                      (int)ustrlen(name),
                      rettv,
                      argc,
                      argv,
                      NULL,
                      curwin->w_cursor.lnum,
                      curwin->w_cursor.lnum,
                      &dummy,
                      true,
                      partial,
                      selfdict);
    }

    // Free the arguments.
    while(argc > 0)
    {
        tv_clear(&argv[--argc]);
    }

    return r;
}

/// "call(func, arglist [, dict])" function
static void f_call(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[1].v_type != kNvarList)
    {
        EMSG(_(e_listreq));
        return;
    }

    if(argvars[1].vval.v_list == NULL)
    {
        return;
    }

    uchar_kt *func;
    partial_st *partial = NULL;
    dict_st *selfdict = NULL;

    if(argvars[0].v_type == kNvarUfunc)
    {
        func = argvars[0].vval.v_string;
    }
    else if(argvars[0].v_type == kNvarPartial)
    {
        partial = argvars[0].vval.v_partial;
        func = partial_name(partial);
    }
    else
    {
        func = (uchar_kt *)tv_get_string(&argvars[0]);
    }

    if(*func == NUL)
    {
        return; // type error or empty name
    }

    if(argvars[2].v_type != kNvarUnknown)
    {
        if(argvars[2].v_type != kNvarDict)
        {
            EMSG(_(e_dictreq));
            return;
        }

        selfdict = argvars[2].vval.v_dict;
    }

    func_call(func, &argvars[1], partial, selfdict, rettv);
}

/// "changenr()" function
static void f_changenr(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = curbuf->b_u_seq_cur;
}

/// "char2nr(string)" function
static void f_char2nr(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[1].v_type != kNvarUnknown)
    {
        if(!tv_check_num(&argvars[1]))
        {
            return;
        }
    }

    rettv->vval.v_number =
        utf_ptr2char((const uchar_kt *)tv_get_string(&argvars[0]));
}

/// "cindent(lnum)" function
static void f_cindent(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    apos_st pos;
    linenum_kt lnum;
    pos = curwin->w_cursor;
    lnum = tv_get_lnum(argvars);

    if(lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
        curwin->w_cursor.lnum = lnum;
        rettv->vval.v_number = get_c_indent();
        curwin->w_cursor = pos;
    }
    else
    {
        rettv->vval.v_number = -1;
    }
}

/// "clearmatches()" function
static void f_clearmatches(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                           typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    clear_matches(curwin);
}

/// "col(string)" function
static void f_col(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    columnum_kt col = 0;
    apos_st *fp;
    int fnum = curbuf->b_id;
    fp = var2fpos(&argvars[0], FALSE, &fnum);

    if(fp != NULL && fnum == curbuf->b_id)
    {
        if(fp->col == MAXCOL)
        {
            // '> can be MAXCOL, get the length of the line then
            if(fp->lnum <= curbuf->b_ml.ml_line_count)
            {
                col = (columnum_kt)ustrlen(ml_get(fp->lnum)) + 1;
            }
            else
            {
                col = MAXCOL;
            }
        }
        else
        {
            col = fp->col + 1;

            // col(".") when the cursor is on the NUL at the end of the line
            // because of "coladd" can be seen as an extra column.
            if(virtual_active() && fp == &curwin->w_cursor)
            {
                uchar_kt  *p = get_cursor_pos_ptr();

                if(curwin->w_cursor.coladd
                   >= (columnum_kt)chartabsize(p, curwin->w_virtcol - curwin->w_cursor.coladd))
                {
                    int l;

                    if(*p != NUL && p[(l = (*mb_ptr2len)(p))] == NUL)
                    {
                        col += l;
                    }
                }
            }
        }
    }

    rettv->vval.v_number = col;
}

/// "complete()" function
static void f_complete(typval_st *argvars,
                       typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if((curmod & kInsertMode) == 0)
    {
        EMSG(_("E785: complete() can only be used in Insert mode"));
        return;
    }

    // Check for undo allowed here, because if something was already inserted
    // the line was already saved for undo and this check isn't done.
    if(!undo_allowed())
    {
        return;
    }

    if(argvars[1].v_type != kNvarList || argvars[1].vval.v_list == NULL)
    {
        EMSG(_(e_invarg));
        return;
    }

    const int startcol = tv_get_number_chk(&argvars[0], NULL);

    if(startcol <= 0)
    {
        return;
    }

    set_completion(startcol - 1, argvars[1].vval.v_list);
}

/// "complete_add()" function
static void f_complete_add(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = ins_compl_add_tv(&argvars[0], 0);
}

/// "complete_check()" function
static void f_complete_check(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                             typval_st *rettv,
                             func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int saved = RedrawingDisabled;
    RedrawingDisabled = 0;
    ins_compl_check_keys(0, true);
    rettv->vval.v_number = compl_interrupted;
    RedrawingDisabled = saved;
}

/// "confirm(message, buttons[, default [, type]])" function
static void f_confirm(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char buf[NUMBUFLEN];
    char buf2[NUMBUFLEN];
    const char *message;
    const char *buttons = NULL;
    int def = 1;
    int type = VIM_GENERIC;
    const char *typestr;
    bool error = false;
    message = tv_get_string_chk(&argvars[0]);

    if(message == NULL)
    {
        error = true;
    }

    if(argvars[1].v_type != kNvarUnknown)
    {
        buttons = tv_get_string_buf_chk(&argvars[1], buf);

        if(buttons == NULL)
        {
            error = true;
        }

        if(argvars[2].v_type != kNvarUnknown)
        {
            def = tv_get_number_chk(&argvars[2], &error);

            if(argvars[3].v_type != kNvarUnknown)
            {
                typestr = tv_get_string_buf_chk(&argvars[3], buf2);

                if(typestr == NULL)
                {
                    error = true;
                }
                else
                {
                    switch(TOUPPER_ASC(*typestr))
                    {
                        case 'E':
                            type = VIM_ERROR;
                            break;

                        case 'Q':
                            type = VIM_QUESTION;
                            break;

                        case 'I':
                            type = VIM_INFO;
                            break;

                        case 'W':
                            type = VIM_WARNING;
                            break;

                        case 'G':
                            type = VIM_GENERIC;
                            break;
                    }
                }
            }
        }
    }

    if(buttons == NULL || *buttons == NUL)
    {
        buttons = _("&Ok");
    }

    if(!error)
    {
        rettv->vval.v_number = do_dialog(type,
                                         NULL,
                                         (uchar_kt *)message,
                                         (uchar_kt *)buttons,
                                         def,
                                         NULL,
                                         false);
    }
}

/// "copy()" function
static void f_copy(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    var_item_copy(NULL, &argvars[0], rettv, false, 0);
}

/// "count()" function
static void f_count(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    long n = 0;
    int ic = FALSE;

    if(argvars[0].v_type == kNvarList)
    {
        listitem_st *li;
        list_st *l;
        long idx;

        if((l = argvars[0].vval.v_list) != NULL)
        {
            li = l->lv_first;

            if(argvars[2].v_type != kNvarUnknown)
            {
                bool error = false;
                ic = tv_get_number_chk(&argvars[2], &error);

                if(argvars[3].v_type != kNvarUnknown)
                {
                    idx = tv_get_number_chk(&argvars[3], &error);

                    if(!error)
                    {
                        li = tv_list_find(l, idx);

                        if(li == NULL)
                        {
                            EMSGN(_(e_listidx), idx);
                        }
                    }
                }

                if(error)
                {
                    li = NULL;
                }
            }

            for(; li != NULL; li = li->li_next)
            {
                if(tv_equal(&li->li_tv, &argvars[1], ic, FALSE))
                {
                    ++n;
                }
            }
        }
    }
    else if(argvars[0].v_type == kNvarDict)
    {
        int todo;
        dict_st *d;
        hashitem_st *hi;

        if((d = argvars[0].vval.v_dict) != NULL)
        {
            bool error = false;

            if(argvars[2].v_type != kNvarUnknown)
            {
                ic = tv_get_number_chk(&argvars[2], &error);

                if(argvars[3].v_type != kNvarUnknown)
                {
                    EMSG(_(e_invarg));
                }
            }

            todo = error ? 0 : (int)d->dv_hashtab.ht_used;

            for(hi = d->dv_hashtab.ht_array; todo > 0; ++hi)
            {
                if(!HASHITEM_EMPTY(hi))
                {
                    todo--;

                    if(tv_equal(&TV_DICT_HI2DI(hi)->di_tv, &argvars[1], ic, false))
                    {
                        n++;
                    }
                }
            }
        }
    }
    else
    {
        EMSG2(_(e_listdictarg), "count()");
    }

    rettv->vval.v_number = n;
}

/// "cscope_connection([{num} , {dbpath} [, {prepend}]])" function
///
/// Checks the existence of a cscope connection.
static void f_cscope_connection(typval_st *argvars,
                                typval_st *rettv,
                                func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int num = 0;
    const char *dbpath = NULL;
    const char *prepend = NULL;
    char buf[NUMBUFLEN];

    if(argvars[0].v_type != kNvarUnknown && argvars[1].v_type != kNvarUnknown)
    {
        num = (int)tv_get_number(&argvars[0]);
        dbpath = tv_get_string(&argvars[1]);

        if(argvars[2].v_type != kNvarUnknown)
        {
            prepend = tv_get_string_buf(&argvars[2], buf);
        }
    }

    rettv->vval.v_number =
        cs_connection(num, (uchar_kt *)dbpath, (uchar_kt *)prepend);
}

/// "cursor(lnum, col)" function, or
/// "cursor(list)"
///
/// Moves the cursor to the specified line and column.
///
/// @returns 0 when the position could be set, -1 otherwise.
static void f_cursor(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    long line, col;
    long coladd = 0;
    bool set_curswant = true;
    rettv->vval.v_number = -1;

    if(argvars[1].v_type == kNvarUnknown)
    {
        apos_st pos;
        columnum_kt curswant = -1;

        if(list2fpos(argvars, &pos, NULL, &curswant) == FAIL)
        {
            EMSG(_(e_invarg));
            return;
        }

        line = pos.lnum;
        col = pos.col;
        coladd = pos.coladd;

        if(curswant >= 0)
        {
            curwin->w_curswant = curswant - 1;
            set_curswant = false;
        }
    }
    else
    {
        line = tv_get_lnum(argvars);
        col = tv_get_number_chk(&argvars[1], NULL);

        if(argvars[2].v_type != kNvarUnknown)
        {
            coladd = tv_get_number_chk(&argvars[2], NULL);
        }
    }

    if(line < 0 || col < 0 || coladd < 0)
    {
        return; // type error; errmsg already given
    }

    if(line > 0)
    {
        curwin->w_cursor.lnum = line;
    }

    if(col > 0)
    {
        curwin->w_cursor.col = col - 1;
    }

    curwin->w_cursor.coladd = coladd;

    // Make sure the cursor is in a valid position.
    check_cursor();

    // Correct cursor for multi-byte character.
    mb_adjust_cursor();

    curwin->w_set_curswant = set_curswant;
    rettv->vval.v_number = 0;
}

/// "deepcopy()" function
static void f_deepcopy(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int noref = 0;

    if(argvars[1].v_type != kNvarUnknown)
    {
        noref = tv_get_number_chk(&argvars[1], NULL);
    }

    if(noref < 0 || noref > 1)
    {
        emsgf(_(e_invarg));
    }
    else
    {
        var_item_copy(NULL, &argvars[0], rettv,
                      true, (noref == 0 ? get_copyID() : 0));
    }
}

/// "delete()" function
static void f_delete(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = -1;

    if(check_restricted() || check_secure())
    {
        return;
    }

    const char *const name = tv_get_string(&argvars[0]);

    if(name == NULL || *name == NUL)
    {
        EMSG(_(e_invarg));
        return;
    }

    char nbuf[NUMBUFLEN];
    const char *flags;

    if(argvars[1].v_type != kNvarUnknown)
    {
        flags = tv_get_string_buf(&argvars[1], nbuf);
    }
    else
    {
        flags = "";
    }

    if(*flags == NUL)
    {
        // delete a file
        rettv->vval.v_number = os_remove(name) == 0 ? 0 : -1;
    }
    else if(strcmp(flags, "d") == 0)
    {
        // delete an empty directory
        rettv->vval.v_number = os_rmdir(name) == 0 ? 0 : -1;
    }
    else if(strcmp(flags, "rf") == 0)
    {
        // delete a directory recursively
        rettv->vval.v_number = delete_recursive(name);
    }
    else
    {
        EMSG2(_(e_invexpr2), flags);
    }
}

/// dictwatcheradd(dict, key, funcref) function
static void f_dictwatcheradd(typval_st *argvars,
                             typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                             func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarDict)
    {
        emsgf(_(e_invarg2), "dict");
        return;
    }
    else if(argvars[0].vval.v_dict == NULL)
    {
        const char *const arg_errmsg = _("dictwatcheradd() argument");
        const size_t arg_errmsg_len = strlen(arg_errmsg);

        emsgf(_(e_readonlyvar), (int)arg_errmsg_len, arg_errmsg);

        return;
    }

    if(argvars[1].v_type != kNvarString && argvars[1].v_type != kNvarNumber)
    {
        emsgf(_(e_invarg2), "key");
        return;
    }

    const char *const key_pattern = tv_get_string_chk(argvars + 1);

    if(key_pattern == NULL)
    {
        return;
    }

    const size_t key_pattern_len = strlen(key_pattern);
    callback_st callback;

    if(!callback_from_typval(&callback, &argvars[2]))
    {
        emsgf(_(e_invarg2), "funcref");
        return;
    }

    tv_dict_watcher_add(argvars[0].vval.v_dict,
                        key_pattern, key_pattern_len, callback);
}

// dictwatcherdel(dict, key, funcref) function
static void f_dictwatcherdel(typval_st *argvars,
                             typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                             func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarDict)
    {
        emsgf(_(e_invarg2), "dict");
        return;
    }

    if(argvars[2].v_type != kNvarUfunc && argvars[2].v_type != kNvarString)
    {
        emsgf(_(e_invarg2), "funcref");
        return;
    }

    const char *const key_pattern = tv_get_string_chk(argvars + 1);

    if(key_pattern == NULL)
    {
        return;
    }

    callback_st callback;

    if(!callback_from_typval(&callback, &argvars[2]))
    {
        return;
    }

    if(!tv_dict_watcher_remove(argvars[0].vval.v_dict,
                               key_pattern,
                               strlen(key_pattern),
                               callback))
    {
        EMSG("Couldn't find a watcher matching key and callback");
    }

    callback_free(&callback);
}

/// "did_filetype()" function
static void f_did_filetype(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = did_filetype;
}

/// "diff_filler()" function
static void f_diff_filler(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = diff_check_fill(curwin, tv_get_lnum(argvars));
}

/// "diff_hlID()" function
static void f_diff_hlID(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    linenum_kt lnum = tv_get_lnum(argvars);
    static linenum_kt prev_lnum = 0;
    static int changedtick = 0;
    static int fnum = 0;
    static int change_start = 0;
    static int change_end = 0;
    static hlf_et hlID = (hlf_et)0;
    int filler_lines;
    int col;

    // ignore type error in {lnum} arg
    if(lnum < 0)
    {
        lnum = 0;
    }

    if(lnum != prev_lnum
       || changedtick != curbuf->b_changedtick
       || fnum != curbuf->b_id)
    {
        // New line, buffer, change: need to get the values.
        filler_lines = diff_check(curwin, lnum);

        if(filler_lines < 0)
        {
            if(filler_lines == -1)
            {
                change_start = MAXCOL;
                change_end = -1;

                if(diff_find_change(curwin, lnum, &change_start, &change_end))
                {
                    hlID = HLF_ADD; // added line
                }
                else
                {
                    hlID = HLF_CHD; // changed line
                }
            }
            else
            {
                hlID = HLF_ADD; // added line
            }
        }
        else
        {
            hlID = (hlf_et)0;
        }

        prev_lnum = lnum;
        changedtick = curbuf->b_changedtick;
        fnum = curbuf->b_id;
    }

    if(hlID == HLF_CHD || hlID == HLF_TXD)
    {
        col = tv_get_number(&argvars[1]) - 1; // Ignore type error in {col}.

        if(col >= change_start && col <= change_end)
        {
            hlID = HLF_TXD; // Changed text.
        }
        else
        {
            hlID = HLF_CHD; // Changed line.
        }
    }

    rettv->vval.v_number = hlID == (hlf_et)0 ? 0 : (int)hlID;
}

/// "empty({expr})" function
static void f_empty(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    bool n = true;

    switch(argvars[0].v_type)
    {
        case kNvarString:
        case kNvarUfunc:
            n = argvars[0].vval.v_string == NULL
                || *argvars[0].vval.v_string == NUL;
            break;

        case kNvarPartial:
            n = false;
            break;

        case kNvarNumber:
            n = argvars[0].vval.v_number == 0;
            break;

        case kNvarFloat:
            n = argvars[0].vval.v_float == 0.0;
            break;

        case kNvarList:
            n = argvars[0].vval.v_list == NULL
                || argvars[0].vval.v_list->lv_first == NULL;
            break;

        case kNvarDict:
            n = argvars[0].vval.v_dict == NULL
                || argvars[0].vval.v_dict->dv_hashtab.ht_used == 0;
            break;

        case kNvarSpecial:
            n = argvars[0].vval.v_special != kSpecialVarTrue;
            break;

        case kNvarUnknown:
            EMSG2(_(e_intern2), "f_empty(UNKNOWN)");
            break;
    }

    rettv->vval.v_number = n;
}

/// "escape({string}, {chars})" function
static void f_escape(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char buf[NUMBUFLEN];

    rettv->vval.v_string =
            ustrdup_escape((const uchar_kt *)tv_get_string(&argvars[0]),
                            (const uchar_kt *)tv_get_string_buf(&argvars[1], buf));

    rettv->v_type = kNvarString;
}

/// "eval()" function
static void f_eval(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *s = tv_get_string_chk(&argvars[0]);

    if(s != NULL)
    {
        s = (const char *)skipwhite((const uchar_kt *)s);
    }

    const char *const expr_start = s;

    if(s == NULL || eval_lev_1((uchar_kt **)&s, rettv, true) == FAIL)
    {
        if(expr_start != NULL && !aborting())
        {
            EMSG2(_(e_invexpr2), expr_start);
        }

        need_clr_eos = FALSE;
        rettv->v_type = kNvarNumber;
        rettv->vval.v_number = 0;
    }
    else if(*s != NUL)
    {
        EMSG(_(e_trailing));
    }
}

/// "eventhandler()" function
static void f_eventhandler(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = vgetc_busy;
}

/// "executable()" function
static void f_executable(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *name = tv_get_string(&argvars[0]);

    // Check in $PATH and also check directly if there is a directory name
    rettv->vval.v_number = (os_can_exe((const uchar_kt *)name, NULL, true)
                            || (gettail_dir(name) != name
                                && os_can_exe((const uchar_kt *)name, NULL, false)));
}

static uchar_kt *get_list_line(int FUNC_ARGS_UNUSED_MATCH(c),
                             void *cookie,
                             int FUNC_ARGS_UNUSED_MATCH(indent))
{
    const listitem_st **const p = (const listitem_st **)cookie;
    const listitem_st *item = *p;

    if(item == NULL)
    {
        return NULL;
    }

    char buf[NUMBUFLEN];

    const char *const s = tv_get_string_buf_chk(&item->li_tv, buf);
    *p = item->li_next;

    return (uchar_kt *)(s == NULL ? NULL : xstrdup(s));
}

/// "execute(command)" function
static void f_execute(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const int save_msg_silent = msg_silent;
    const int save_emsg_silent = emsg_silent;
    const bool save_emsg_noredir = emsg_noredir;
    garray_st *const save_capture_ga = capture_ga;

    if(check_secure())
    {
        return;
    }

    if(argvars[1].v_type != kNvarUnknown)
    {
        char buf[NUMBUFLEN];
        const char *const s = tv_get_string_buf_chk(&argvars[1], buf);

        if(s == NULL)
        {
            return;
        }

        if(strncmp(s, "silent", 6) == 0)
        {
            msg_silent++;
        }

        if(strcmp(s, "silent!") == 0)
        {
            emsg_silent = true;
            emsg_noredir = true;
        }
    }
    else
    {
        msg_silent++;
    }

    garray_st capture_local;
    ga_init(&capture_local, (int)sizeof(char), 80);
    capture_ga = &capture_local;

    if(argvars[0].v_type != kNvarList)
    {
        do_cmdline_cmd(tv_get_string(&argvars[0]));
    }
    else if(argvars[0].vval.v_list != NULL)
    {
        list_st *const list = argvars[0].vval.v_list;
        list->lv_refcount++;

        listitem_st *const item = list->lv_first;

        do_cmdline(NULL,
                   get_list_line,
                   (void *)&item,
                   DOCMD_NOWAIT|DOCMD_VERBOSE|DOCMD_REPEAT|DOCMD_KEYTYPED);

        list->lv_refcount--;
    }

    msg_silent = save_msg_silent;
    emsg_silent = save_emsg_silent;
    emsg_noredir = save_emsg_noredir;

    ga_append(capture_ga, NUL);

    rettv->v_type = kNvarString;
    rettv->vval.v_string = ustrdup(capture_ga->ga_data);
    ga_clear(capture_ga);
    capture_ga = save_capture_ga;
}

/// "exepath()" function
static void f_exepath(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *arg = tv_get_string(&argvars[0]);
    uchar_kt *path = NULL;

    (void)os_can_exe((const uchar_kt *)arg, &path, true);

    rettv->v_type = kNvarString;
    rettv->vval.v_string = path;
}

/// "exists()" function
static void f_exists(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int n = false;
    int len = 0;
    const char *p = tv_get_string(&argvars[0]);

    if(*p == '$') // Environment variable.
    {
        // First try "normal" environment variables (fast).
        if(os_getenv(p + 1) != NULL)
        {
            n = true;
        }
        else
        {
            // Try expanding things like $VIM and ${HOME}.
            uchar_kt *const exp = expand_env_save((uchar_kt *)p);

            if(exp != NULL && *exp != '$')
            {
                n = true;
            }

            xfree(exp);
        }
    }
    else if(*p == '&' || *p == '+') // Option.
    {
        n = (get_option_tv(&p, NULL, true) == OK);

        if(*skipwhite((const uchar_kt *)p) != NUL)
        {
            n = false; // Trailing garbage.
        }
    }
    else if(*p == '*') // Internal or user defined function.
    {
        n = function_exists(p + 1, false);
    }
    else if(*p == ':')
    {
        n = cmd_exists(p + 1);
    }
    else if(*p == '#')
    {
        if(p[1] == '#')
        {
            n = autocmd_supported(p + 2);
        }
        else
        {
            n = au_exists(p + 1);
        }
    }
    else // Internal variable.
    {
        typval_st tv;

        // get_name_len() takes care of expanding curly braces
        const char *name = p;
        char *tofree;
        len = get_name_len((const char **)&p, &tofree, true, false);

        if(len > 0)
        {
            if(tofree != NULL)
            {
                name = tofree;
            }

            n = (get_var_tv(name, len, &tv, NULL, false, true) == OK);

            if(n)
            {
                // Handle d.key, l[idx], f(expr).
                n = (handle_subscript(&p, &tv, true, false) == OK);

                if(n)
                {
                    tv_clear(&tv);
                }
            }
        }

        if(*p != NUL)
        {
            n = FALSE;
        }

        xfree(tofree);
    }

    rettv->vval.v_number = n;
}

/// "expand()" function
static void f_expand(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    size_t len;
    uchar_kt *errormsg;
    int options = WILD_SILENT|WILD_USE_NL|WILD_LIST_NOTFOUND;
    expand_st xpc;
    bool error = false;
    uchar_kt *result;
    rettv->v_type = kNvarString;

    if(argvars[1].v_type != kNvarUnknown
       && argvars[2].v_type != kNvarUnknown
       && tv_get_number_chk(&argvars[2], &error)
       && !error)
    {
        rettv->v_type = kNvarList;
        rettv->vval.v_list = NULL;
    }

    const char *s = tv_get_string(&argvars[0]);

    if(*s == '%' || *s == '#' || *s == '<')
    {
        emsg_off++;
        result = eval_vars((uchar_kt *)s, (uchar_kt *)s, &len, NULL, &errormsg, NULL);
        emsg_off--;

        if(rettv->v_type == kNvarList)
        {
            tv_list_alloc_ret(rettv);

            if(result != NULL)
            {
                tv_list_append_string(rettv->vval.v_list, (const char *)result, -1);
            }
        }
        else
        {
            rettv->vval.v_string = result;
        }
    }
    else
    {
        // When the optional second argument is non-zero, don't remove matches
        // for 'wildignore' and don't put matches for 'suffixes' at the end.
        if(argvars[1].v_type != kNvarUnknown
           && tv_get_number_chk(&argvars[1], &error))
        {
            options |= WILD_KEEP_ALL;
        }

        if(!error)
        {
            ExpandInit(&xpc);
            xpc.xp_context = EXPAND_FILES;

            if(p_wic)
            {
                options += WILD_ICASE;
            }

            if(rettv->v_type == kNvarString)
            {
                rettv->vval.v_string =
                    ExpandOne(&xpc, (uchar_kt *)s, NULL, options, WILD_ALL);
            }
            else
            {
                tv_list_alloc_ret(rettv);
                ExpandOne(&xpc, (uchar_kt *)s, NULL, options, WILD_ALL_KEEP);

                for(int i = 0; i < xpc.xp_numfiles; i++)
                {
                    tv_list_append_string(rettv->vval.v_list,
                                          (const char *)xpc.xp_files[i], -1);
                }

                ExpandCleanup(&xpc);
            }
        }
        else
        {
            rettv->vval.v_string = NULL;
        }
    }
}

/// - "extend(list, list [, idx])" function
/// - "extend(dict, dict [, action])" function
static void f_extend(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const arg_errmsg = N_("extend() argument");

    if(argvars[0].v_type == kNvarList && argvars[1].v_type == kNvarList)
    {
        long before;
        bool error = false;
        list_st *const l1 = argvars[0].vval.v_list;
        list_st *const l2 = argvars[1].vval.v_list;

        if(l1 == NULL)
        {
            const bool locked =
                tv_check_lock(kNvlVarFixed, arg_errmsg, TV_TRANSLATE);

            (void)locked;
            assert(locked == true);
        }
        else if(l2 == NULL)
        {
            // Do nothing
            tv_copy(&argvars[0], rettv);
        }
        else if(!tv_check_lock(l1->lv_lock, arg_errmsg, TV_TRANSLATE))
        {
            listitem_st *item;

            if(argvars[2].v_type != kNvarUnknown)
            {
                before = tv_get_number_chk(&argvars[2], &error);

                if(error)
                {
                    return; // Type error; errmsg already given.
                }

                if(before == l1->lv_len)
                {
                    item = NULL;
                }
                else
                {
                    item = tv_list_find(l1, before);

                    if(item == NULL)
                    {
                        EMSGN(_(e_listidx), before);
                        return;
                    }
                }
            }
            else
            {
                item = NULL;
            }

            tv_list_extend(l1, l2, item);
            tv_copy(&argvars[0], rettv);
        }
    }
    else if(argvars[0].v_type == kNvarDict && argvars[1].v_type == kNvarDict)
    {
        dict_st *const d1 = argvars[0].vval.v_dict;
        dict_st *const d2 = argvars[1].vval.v_dict;

        if(d1 == NULL)
        {
            const bool locked =
                tv_check_lock(kNvlVarFixed, arg_errmsg, TV_TRANSLATE);

            (void)locked;
            assert(locked == true);
        }
        else if(d2 == NULL)
        {
            // Do nothing
            tv_copy(&argvars[0], rettv);
        }
        else if(!tv_check_lock(d1->dv_lock, arg_errmsg, TV_TRANSLATE))
        {
            const char *action = "force";

            // Check the third argument.
            if(argvars[2].v_type != kNvarUnknown)
            {
                const char *const av[] = { "keep", "force", "error" };
                action = tv_get_string_chk(&argvars[2]);

                if(action == NULL)
                {
                    return; // Type error; error message already given.
                }

                size_t i;

                for(i = 0; i < ARRAY_SIZE(av); i++)
                {
                    if(strcmp(action, av[i]) == 0)
                    {
                        break;
                    }
                }

                if(i == 3)
                {
                    EMSG2(_(e_invarg2), action);
                    return;
                }
            }

            tv_dict_extend(d1, d2, action);
            tv_copy(&argvars[0], rettv);
        }
    }
    else
    {
        EMSG2(_(e_listdictarg), "extend()");
    }
}

/// "feedkeys()" function
static void f_feedkeys(typval_st *argvars,
                       typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    // This is not allowed in the sandbox. If the commands would still be
    // executed in the sandbox it would be OK, but it probably happens later,
    // when "sandbox" is no longer set.
    if(check_secure())
    {
        return;
    }

    const char *const keys = tv_get_string(&argvars[0]);
    char nbuf[NUMBUFLEN];
    const char *flags = NULL;

    if(argvars[1].v_type != kNvarUnknown)
    {
        flags = tv_get_string_buf(&argvars[1], nbuf);
    }

    nvim_feedkeys(cstr_as_string((char *)keys),
                  cstr_as_string((char *)flags), true);
}

/// "filereadable()" function
static void f_filereadable(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const p = tv_get_string(&argvars[0]);

    rettv->vval.v_number =
        (*p && !os_isdir((const uchar_kt *)p) && os_file_is_readable(p));
}

/// Return 0 for not writable, 1 for writable file,
/// 2 for a dir which we have rights to write into.
static void f_filewritable(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *filename = tv_get_string(&argvars[0]);
    rettv->vval.v_number = os_file_is_writable(filename);
}


static void findfilendir(typval_st *argvars, typval_st *rettv, int find_what)
{
    uchar_kt *fresult = NULL;
    uchar_kt *path = *curbuf->b_p_path == NUL ? p_path : curbuf->b_p_path;
    int count = 1;
    bool first = true;
    bool error = false;
    rettv->vval.v_string = NULL;
    rettv->v_type = kNvarString;
    const char *fname = tv_get_string(&argvars[0]);
    char pathbuf[NUMBUFLEN];

    if(argvars[1].v_type != kNvarUnknown)
    {
        const char *p = tv_get_string_buf_chk(&argvars[1], pathbuf);

        if(p == NULL)
        {
            error = true;
        }
        else
        {
            if(*p != NUL)
            {
                path = (uchar_kt *)p;
            }

            if(argvars[2].v_type != kNvarUnknown)
            {
                count = tv_get_number_chk(&argvars[2], &error);
            }
        }
    }

    if(count < 0)
    {
        tv_list_alloc_ret(rettv);
    }

    if(*fname != NUL && !error)
    {
        do
        {
            if(rettv->v_type == kNvarString || rettv->v_type == kNvarList)
            {
                xfree(fresult);
            }

            fresult = find_file_in_path_option(first ? (uchar_kt *)fname : NULL,
                                               first ? strlen(fname) : 0,
                                               0,
                                               first,
                                               path,
                                               find_what,
                                               curbuf->b_ffname,
                                               (find_what == FINDFILE_DIR
                                                ? (uchar_kt *)"" : curbuf->b_p_sua));
            first = false;

            if(fresult != NULL && rettv->v_type == kNvarList)
            {
                tv_list_append_string(rettv->vval.v_list, (const char *)fresult, -1);
            }
        } while((rettv->v_type == kNvarList || --count > 0) && fresult != NULL);
    }

    if(rettv->v_type == kNvarString)
    {
        rettv->vval.v_string = fresult;
    }
}


/// Implementation of map() and filter().
static void filter_map(typval_st *argvars, typval_st *rettv, int map)
{
    typval_st *expr;
    listitem_st *li;
    listitem_st *nli;
    list_st *l = NULL;
    dictitem_st *di;
    hashtable_st *ht;
    hashitem_st *hi;
    dict_st *d = NULL;
    typval_st save_val;
    typval_st save_key;
    int rem = false;
    int todo;
    int idx = 0;
    int save_did_emsg;
    uchar_kt *ermsg = (uchar_kt *)(map ? "map()" : "filter()");

    const char *const arg_errmsg =
        (map ? N_("map() argument") : N_("filter() argument"));

    if(argvars[0].v_type == kNvarList)
    {
        if((l = argvars[0].vval.v_list) == NULL
           || (!map && tv_check_lock(l->lv_lock, arg_errmsg, TV_TRANSLATE)))
        {
            return;
        }
    }
    else if(argvars[0].v_type == kNvarDict)
    {
        if((d = argvars[0].vval.v_dict) == NULL
           || (!map && tv_check_lock(d->dv_lock, arg_errmsg, TV_TRANSLATE)))
        {
            return;
        }
    }
    else
    {
        EMSG2(_(e_listdictarg), ermsg);
        return;
    }

    expr = &argvars[1];

    // On type errors, the preceding call has already displayed an error
    // message. Avoid a misleading error message for an empty string that
    // was not passed as argument.
    if(expr->v_type != kNvarUnknown)
    {
        prepare_vimvar(VV_VAL, &save_val);

        // We reset "did_emsg" to be able to detect whether an error
        // occurred during evaluation of the expression.
        save_did_emsg = did_emsg;
        did_emsg = FALSE;
        prepare_vimvar(VV_KEY, &save_key);

        if(argvars[0].v_type == kNvarDict)
        {
            vimvars[VV_KEY].vv_type = kNvarString;
            ht = &d->dv_hashtab;
            hash_lock(ht);
            todo = (int)ht->ht_used;

            for(hi = ht->ht_array; todo > 0; ++hi)
            {
                if(!HASHITEM_EMPTY(hi))
                {
                    --todo;
                    di = TV_DICT_HI2DI(hi);

                    if(map
                       && (tv_check_lock(di->di_tv.v_lock, arg_errmsg, TV_TRANSLATE)
                           || var_check_ro(di->di_flags, arg_errmsg, TV_TRANSLATE)))
                    {
                        break;
                    }

                    vimvars[VV_KEY].vv_str = ustrdup(di->di_key);
                    int r = filter_map_one(&di->di_tv, expr, map, &rem);
                    tv_clear(&vimvars[VV_KEY].vv_tv);

                    if(r == FAIL || did_emsg)
                    {
                        break;
                    }

                    if(!map && rem)
                    {
                        if(var_check_fixed(di->di_flags, arg_errmsg, TV_TRANSLATE)
                           || var_check_ro(di->di_flags, arg_errmsg, TV_TRANSLATE))
                        {
                            break;
                        }

                        tv_dict_item_remove(d, di);
                    }
                }
            }

            hash_unlock(ht);
        }
        else
        {
            vimvars[VV_KEY].vv_type = kNvarNumber;

            for(li = l->lv_first; li != NULL; li = nli)
            {
                if(map
                   && tv_check_lock(li->li_tv.v_lock, arg_errmsg, TV_TRANSLATE))
                {
                    break;
                }

                nli = li->li_next;
                vimvars[VV_KEY].vv_nr = idx;

                if(filter_map_one(&li->li_tv, expr, map, &rem) == FAIL || did_emsg)
                {
                    break;
                }

                if(!map && rem)
                {
                    tv_list_item_remove(l, li);
                }

                idx++;
            }
        }

        restore_vimvar(VV_KEY, &save_key);
        restore_vimvar(VV_VAL, &save_val);
        did_emsg |= save_did_emsg;
    }

    tv_copy(&argvars[0], rettv);
}

static int filter_map_one(typval_st *tv, typval_st *expr, int map, int *remp)
{
    typval_st rettv;
    typval_st argv[3];
    int retval = FAIL;
    int dummy;
    tv_copy(tv, &vimvars[VV_VAL].vv_tv);
    argv[0] = vimvars[VV_KEY].vv_tv;
    argv[1] = vimvars[VV_VAL].vv_tv;

    if(expr->v_type == kNvarUfunc)
    {
        const uchar_kt *const s = expr->vval.v_string;

        if(call_func(s,
                     (int)ustrlen(s),
                     &rettv,
                     2,
                     argv,
                     NULL,
                     0L,
                     0L,
                     &dummy,
                     true,
                     NULL,
                     NULL) == FAIL)
        {
            goto theend;
        }
    }
    else if(expr->v_type == kNvarPartial)
    {
        partial_st *partial = expr->vval.v_partial;
        const uchar_kt *const s = partial_name(partial);

        if(call_func(s,
                     (int)ustrlen(s),
                     &rettv,
                     2,
                     argv,
                     NULL,
                     0L,
                     0L,
                     &dummy,
                     true,
                     partial,
                     NULL) == FAIL)
        {
            goto theend;
        }
    }
    else
    {
        char buf[NUMBUFLEN];
        const char *s = tv_get_string_buf_chk(expr, buf);

        if(s == NULL)
        {
            goto theend;
        }

        s = (const char *)skipwhite((const uchar_kt *)s);

        if(eval_lev_1((uchar_kt **)&s, &rettv, true) == FAIL)
        {
            goto theend;
        }

        if(*s != NUL) // check for trailing chars after expr
        {
            emsgf(_(e_invexpr2), s);
            goto theend;
        }
    }

    if(map)
    {
        // map(): replace the list item value.
        tv_clear(tv);
        rettv.v_lock = 0;
        *tv = rettv;
    }
    else
    {
        bool error = false;

        // filter(): when expr is zero remove the item
        *remp = (tv_get_number_chk(&rettv, &error) == 0);
        tv_clear(&rettv);

        // On type error, nothing has been removed; return FAIL to stop the
        // loop. The error message was given by tv_get_number_chk().
        if(error)
        {
            goto theend;
        }
    }

    retval = OK;

theend:

    tv_clear(&vimvars[VV_VAL].vv_tv);

    return retval;
}

/// "filter()" function
static void f_filter(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    filter_map(argvars, rettv, FALSE);
}

/// "finddir({fname}[, {path}[, {count}]])" function
static void f_finddir(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    findfilendir(argvars, rettv, FINDFILE_DIR);
}

/// "findfile({fname}[, {path}[, {count}]])" function
static void f_findfile(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    findfilendir(argvars, rettv, FINDFILE_FILE);
}

/// "float2nr({float})" function
static void f_float2nr(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    float_kt f;

    if(tv_get_float_chk(argvars, &f))
    {
        if(f < VARNUMBER_MIN)
        {
            rettv->vval.v_number = VARNUMBER_MIN;
        }
        else if(f > VARNUMBER_MAX)
        {
            rettv->vval.v_number = VARNUMBER_MAX;
        }
        else
        {
            rettv->vval.v_number = (number_kt)f;
        }
    }
}

/// "fmod()" function
static void f_fmod(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    float_kt fx;
    float_kt fy;
    rettv->v_type = kNvarFloat;

    if(tv_get_float_chk(argvars, &fx) && tv_get_float_chk(&argvars[1], &fy))
    {
        rettv->vval.v_float = fmod(fx, fy);
    }
    else
    {
        rettv->vval.v_float = 0.0;
    }
}

/// "fnameescape({string})" function
static void f_fnameescape(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_string =
        (uchar_kt *)vim_strsave_fnameescape(tv_get_string(&argvars[0]), false);

    rettv->v_type = kNvarString;
}

/// "fnamemodify({fname}, {mods})" function
static void f_fnamemodify(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    uchar_kt *fbuf = NULL;
    size_t len;
    char buf[NUMBUFLEN];
    const char *fname = tv_get_string_chk(&argvars[0]);
    const char *const mods = tv_get_string_buf_chk(&argvars[1], buf);

    if(fname == NULL || mods == NULL)
    {
        fname = NULL;
    }
    else
    {
        len = strlen(fname);
        size_t usedlen = 0;

        (void)modify_fname((uchar_kt *)mods, &usedlen,
                           (uchar_kt **)&fname, &fbuf, &len);
    }

    rettv->v_type = kNvarString;

    if(fname == NULL)
    {
        rettv->vval.v_string = NULL;
    }
    else
    {
        rettv->vval.v_string = (uchar_kt *)xmemdupz(fname, len);
    }

    xfree(fbuf);
}


/// "foldclosed()" function
static void foldclosed_both(typval_st *argvars, typval_st *rettv, int end)
{
    const linenum_kt lnum = tv_get_lnum(argvars);

    if(lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
        linenum_kt first;
        linenum_kt last;

        if(hasFoldingWin(curwin, lnum, &first, &last, false, NULL))
        {
            if(end)
            {
                rettv->vval.v_number = (number_kt)last;
            }
            else
            {
                rettv->vval.v_number = (number_kt)first;
            }

            return;
        }
    }

    rettv->vval.v_number = -1;
}

/// "foldclosed()" function
static void f_foldclosed(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    foldclosed_both(argvars, rettv, FALSE);
}

/// "foldclosedend()" function
static void f_foldclosedend(typval_st *argvars,
                            typval_st *rettv,
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    foldclosed_both(argvars, rettv, TRUE);
}

/// "foldlevel()" function
static void f_foldlevel(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const linenum_kt lnum = tv_get_lnum(argvars);

    if(lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
        rettv->vval.v_number = foldLevel(lnum);
    }
}

/// nvimL: @b foldtext() function
static void f_foldtext(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    linenum_kt foldstart;
    linenum_kt foldend;
    uchar_kt  *dashes;
    linenum_kt lnum;
    uchar_kt *s;
    uchar_kt *r;
    int len;
    char *txt;

    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;
    foldstart = (linenum_kt)get_vim_var_nr(VV_FOLDSTART);
    foldend = (linenum_kt)get_vim_var_nr(VV_FOLDEND);
    dashes = get_vim_var_str(VV_FOLDDASHES);

    if(foldstart > 0
       && foldend <= curbuf->b_ml.ml_line_count
       && dashes != NULL)
    {
        // Find first non-empty line in the fold.
        for(lnum = foldstart; lnum < foldend; ++lnum)
        {
            if(!linewhite(lnum))
            {
                break;
            }
        }

        // Find interesting text in this line.
        s = skipwhite(ml_get(lnum));

        // skip C comment-start
        if(s[0] == '/' && (s[1] == '*' || s[1] == '/'))
        {
            s = skipwhite(s + 2);

            if(*skipwhite(s) == NUL && lnum + 1 < foldend)
            {
                s = skipwhite(ml_get(lnum + 1));

                if(*s == '*')
                {
                    s = skipwhite(s + 1);
                }
            }
        }

        unsigned long count = (unsigned long)(foldend - foldstart + 1);
        txt = ngettext("+-%s%3ld line: ", "+-%s%3ld lines: ", count);

        r = xmalloc(ustrlen(txt)
                    + ustrlen(dashes) // for %s
                    + 20             // for %3ld
                    + ustrlen(s));    // concatenated

        sprintf((char *)r, txt, dashes, count);
        len = (int)ustrlen(r);
        ustrcat(r, s);

        // remove 'foldmarker' and 'commentstring'
        foldtext_cleanup(r + len);
        rettv->vval.v_string = r;
    }
}

/// "foldtextresult(lnum)" function
static void f_foldtextresult(typval_st *argvars,
                             typval_st *rettv,
                             func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    uchar_kt *text;
    uchar_kt buf[FOLD_TEXT_LEN];
    foldinfo_st foldinfo;
    int fold_count;
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;
    linenum_kt lnum = tv_get_lnum(argvars);

    // Treat illegal types and illegal string values for {lnum} the same.
    if(lnum < 0)
    {
        lnum = 0;
    }

    fold_count = foldedCount(curwin, lnum, &foldinfo);

    if(fold_count > 0)
    {
        text = get_foldtext(curwin, lnum, lnum + fold_count - 1, &foldinfo, buf);

        if(text == buf)
        {
            text = ustrdup(text);
        }

        rettv->vval.v_string = text;
    }
}

/// "foreground()" function
static void f_foreground(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{ /* empty body */ }

static void common_function(typval_st *argvars,
                            typval_st *rettv,
                            bool is_funcref,
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    uchar_kt *s;
    uchar_kt *name;
    bool use_string = false;
    partial_st *arg_pt = NULL;
    uchar_kt *trans_name = NULL;

    if(argvars[0].v_type == kNvarUfunc)
    {
        // function(MyFunc, [arg], dict)
        s = argvars[0].vval.v_string;
    }
    else if(argvars[0].v_type == kNvarPartial
            && argvars[0].vval.v_partial != NULL)
    {
        // function(dict.MyFunc, [arg])
        arg_pt = argvars[0].vval.v_partial;
        s = partial_name(arg_pt);
    }
    else
    {
        // function('MyFunc', [arg], dict)
        s = (uchar_kt *)tv_get_string(&argvars[0]);
        use_string = true;
    }

    if((use_string && ustrchr(s, AUTOLOAD_CHAR) == NULL) || is_funcref)
    {
        name = s;
        trans_name = trans_function_name(&name,
                                         false,
                                         TFN_INT | TFN_QUIET | TFN_NO_AUTOLOAD | TFN_NO_DEREF,
                                         NULL,
                                         NULL);

        if(*name != NUL)
        {
            s = NULL;
        }
    }

    if(s == NULL
       || *s == NUL
       || (use_string && ascii_isdigit(*s))
       || (is_funcref && trans_name == NULL))
    {
        emsgf(_(e_invarg2), (use_string ? tv_get_string(&argvars[0]) : (const char *)s));
        // Don't check an autoload name for existence here.
    }
    else if(trans_name != NULL
            && (is_funcref
                ? find_func(trans_name) == NULL
                : !translated_function_exists((const char *)trans_name)))
    {
        EMSG2(_("E700: Unknown function: %s"), s);
    }
    else
    {
        int dict_idx = 0;
        int arg_idx = 0;
        list_st *list = NULL;

        if(ustrncmp(s, "s:", 2) == 0 || ustrncmp(s, "<SID>", 5) == 0)
        {
            char sid_buf[25];
            int off = *s == 's' ? 2 : 5;

            // Expand s: and <SID> into <SNR>nr_, so that the function can
            // also be called from another script. Using trans_function_name()
            // would also work, but some plugins depend on the name being
            // printable text.
            snprintf(sid_buf, sizeof(sid_buf),
                     "<SNR>%" PRId64 "_", (int64_t)current_SID);

            name = xmalloc(ustrlen(sid_buf) + ustrlen(s + off) + 1);

            if(name != NULL)
            {
                ustrcpy(name, sid_buf);
                ustrcat(name, s + off);
            }
        }
        else
        {
            name = ustrdup(s);
        }

        if(argvars[1].v_type != kNvarUnknown)
        {
            if(argvars[2].v_type != kNvarUnknown)
            {
                // function(name, [args], dict)
                arg_idx = 1;
                dict_idx = 2;
            }
            else if(argvars[1].v_type == kNvarDict)
            {
                // function(name, dict)
                dict_idx = 1;
            }
            else
            {
                // function(name, [args])
                arg_idx = 1;
            }

            if(dict_idx > 0)
            {
                if(argvars[dict_idx].v_type != kNvarDict)
                {
                    EMSG(_("E922: expected a dict"));
                    xfree(name);
                    goto theend;
                }

                if(argvars[dict_idx].vval.v_dict == NULL)
                {
                    dict_idx = 0;
                }
            }

            if(arg_idx > 0)
            {
                if(argvars[arg_idx].v_type != kNvarList)
                {
                    EMSG(_("E923: Second argument of function() "
                           "must be a list or a dict"));

                    xfree(name);
                    goto theend;
                }

                list = argvars[arg_idx].vval.v_list;

                if(list == NULL || list->lv_len == 0)
                {
                    arg_idx = 0;
                }
            }
        }

        if(dict_idx > 0 || arg_idx > 0 || arg_pt != NULL || is_funcref)
        {
            partial_st *const pt = xcalloc(1, sizeof(*pt));

            // result is a kNvarPartial
            if(arg_idx > 0 || (arg_pt != NULL && arg_pt->pt_argc > 0))
            {
                const int arg_len = (arg_pt == NULL ? 0 : arg_pt->pt_argc);
                const int lv_len = (list == NULL ? 0 : list->lv_len);

                pt->pt_argc = arg_len + lv_len;
                pt->pt_argv = xmalloc(sizeof(pt->pt_argv[0]) * pt->pt_argc);

                if(pt->pt_argv == NULL)
                {
                    xfree(pt);
                    xfree(name);
                    goto theend;
                }

                int i = 0;

                for(; i < arg_len; i++)
                {
                    tv_copy(&arg_pt->pt_argv[i], &pt->pt_argv[i]);
                }

                if(lv_len > 0)
                {
                    for(listitem_st *li = list->lv_first; li != NULL; li = li->li_next)
                    {
                        tv_copy(&li->li_tv, &pt->pt_argv[i++]);
                    }
                }
            }

            // For "function(dict.func, [], dict)" and "func" is a partial
            // use "dict". That is backwards compatible.
            if(dict_idx > 0)
            {
                // The dict is bound explicitly, pt_auto is false
                pt->pt_dict = argvars[dict_idx].vval.v_dict;
                (pt->pt_dict->dv_refcount)++;
            }
            else if(arg_pt != NULL)
            {
                // If the dict was bound automatically the result is also
                // bound automatically.
                pt->pt_dict = arg_pt->pt_dict;
                pt->pt_auto = arg_pt->pt_auto;

                if(pt->pt_dict != NULL)
                {
                    (pt->pt_dict->dv_refcount)++;
                }
            }

            pt->pt_refcount = 1;

            if(arg_pt != NULL && arg_pt->pt_func != NULL)
            {
                pt->pt_func = arg_pt->pt_func;
                func_ptr_ref(pt->pt_func);
                xfree(name);
            }
            else if(is_funcref)
            {
                pt->pt_func = find_func(trans_name);
                func_ptr_ref(pt->pt_func);
                xfree(name);
            }
            else
            {
                pt->pt_name = name;
                func_ref(name);
            }

            rettv->v_type = kNvarPartial;
            rettv->vval.v_partial = pt;
        }
        else
        {
            // result is a kNvarUfunc
            rettv->v_type = kNvarUfunc;
            rettv->vval.v_string = name;
            func_ref(name);
        }
    }

theend:

    xfree(trans_name);
}

static void f_funcref(typval_st *argvars, typval_st *rettv, func_ptr_ft fptr)
{
    common_function(argvars, rettv, true, fptr);
}

static void f_function(typval_st *argvars, typval_st *rettv, func_ptr_ft fptr)
{
    common_function(argvars, rettv, false, fptr);
}

/// "garbagecollect()" function
static void f_garbagecollect(typval_st *argvars,
                             typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                             func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    // This is postponed until we are back at the toplevel, because we may be
    // using Lists and Dicts internally.  E.g.: ":echo [garbagecollect()]".
    want_garbage_collect = true;

    if(argvars[0].v_type != kNvarUnknown && tv_get_number(&argvars[0]) == 1)
    {
        garbage_collect_at_exit = true;
    }
}

/// "get()" function
static void f_get(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    listitem_st *li;
    list_st *l;
    dictitem_st *di;
    dict_st *d;
    typval_st *tv = NULL;

    if(argvars[0].v_type == kNvarList)
    {
        if((l = argvars[0].vval.v_list) != NULL)
        {
            bool error = false;
            li = tv_list_find(l, tv_get_number_chk(&argvars[1], &error));

            if(!error && li != NULL)
            {
                tv = &li->li_tv;
            }
        }
    }
    else if(argvars[0].v_type == kNvarDict)
    {
        if((d = argvars[0].vval.v_dict) != NULL)
        {
            di = tv_dict_find(d, tv_get_string(&argvars[1]), -1);

            if(di != NULL)
            {
                tv = &di->di_tv;
            }
        }
    }
    else if(tv_is_func(argvars[0]))
    {
        partial_st *pt;
        partial_st fref_pt;

        if(argvars[0].v_type == kNvarPartial)
        {
            pt = argvars[0].vval.v_partial;
        }
        else
        {
            memset(&fref_pt, 0, sizeof(fref_pt));
            fref_pt.pt_name = argvars[0].vval.v_string;
            pt = &fref_pt;
        }

        if(pt != NULL)
        {
            const char *const what = tv_get_string(&argvars[1]);

            if(strcmp(what, "func") == 0 || strcmp(what, "name") == 0)
            {
                rettv->v_type = (*what == 'f' ? kNvarUfunc : kNvarString);
                const uchar_kt *name = partial_name(pt);
                assert(name != NULL);
                rettv->vval.v_string = ustrdup(name);

                if(rettv->v_type == kNvarUfunc)
                {
                    func_ref(rettv->vval.v_string);
                }
            }
            else if(strcmp(what, "dict") == 0)
            {
                rettv->v_type = kNvarDict;
                rettv->vval.v_dict = pt->pt_dict;

                if(pt->pt_dict != NULL)
                {
                    (pt->pt_dict->dv_refcount)++;
                }
            }
            else if(strcmp(what, "args") == 0)
            {
                rettv->v_type = kNvarList;

                if(tv_list_alloc_ret(rettv) != NULL)
                {
                    for(int i = 0; i < pt->pt_argc; i++)
                    {
                        tv_list_append_tv(rettv->vval.v_list, &pt->pt_argv[i]);
                    }
                }
            }
            else
            {
                EMSG2(_(e_invarg2), what);
            }

            return;
        }
    }
    else
    {
        EMSG2(_(e_listdictarg), "get()");
    }

    if(tv == NULL)
    {
        if(argvars[2].v_type != kNvarUnknown)
        {
            tv_copy(&argvars[2], rettv);
        }
    }
    else
    {
        tv_copy(tv, rettv);
    }
}

/// Returns information about signs placed in a buffer as list of dicts.
static void get_buffer_signs(filebuf_st *buf, list_st *l)
{
    for(signlist_st *sign = buf->b_signlist; sign; sign = sign->next)
    {
        dict_st *const d = tv_dict_alloc();

        tv_dict_add_nr(d, S_LEN("id"), sign->id);
        tv_dict_add_nr(d, S_LEN("lnum"), sign->lnum);

        tv_dict_add_str(d, S_LEN("name"),
                        (const char *)sign_typenr2name(sign->typenr));

        tv_list_append_dict(l, d);
    }
}

/// Returns buffer options, variables and other attributes in a dictionary.
static dict_st *get_buffer_info(filebuf_st *buf)
{
    dict_st *const dict = tv_dict_alloc();
    tv_dict_add_nr(dict, S_LEN("bufnr"), buf->b_id);

    tv_dict_add_str(dict,
                    S_LEN("name"),
                    buf->b_ffname != NULL ? (const char *)buf->b_ffname : "");

    tv_dict_add_nr(dict, S_LEN("lnum"), buflist_findlnum(buf));
    tv_dict_add_nr(dict, S_LEN("loaded"), buf->b_ml.ml_mfp != NULL);
    tv_dict_add_nr(dict, S_LEN("listed"), buf->b_p_bl);
    tv_dict_add_nr(dict, S_LEN("changed"), bufIsChanged(buf));
    tv_dict_add_nr(dict, S_LEN("changedtick"), buf->b_changedtick);

    tv_dict_add_nr(dict,
                   S_LEN("hidden"),
                   buf->b_ml.ml_mfp != NULL && buf->b_nwindows == 0);

    // Get a reference to buffer variables
    tv_dict_add_dict(dict, S_LEN("variables"), buf->b_vars);

    // List of windows displaying this buffer
    list_st *const windows = tv_list_alloc();

    FOR_ALL_TAB_WINDOWS(tp, wp)
    {
        if(wp->w_buffer == buf)
        {
            tv_list_append_number(windows, (number_kt)wp->handle);
        }
    }

    tv_dict_add_list(dict, S_LEN("windows"), windows);

    if(buf->b_signlist != NULL)
    {
        // List of signs placed in this buffer
        list_st *const signs = tv_list_alloc();

        get_buffer_signs(buf, signs);
        tv_dict_add_list(dict, S_LEN("signs"), signs);
    }

    return dict;
}

/// "getbufinfo()" function
static void f_getbufinfo(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    filebuf_st *argbuf = NULL;
    bool filtered = false;
    bool sel_buflisted = false;
    bool sel_bufloaded = false;
    tv_list_alloc_ret(rettv);

    // List of all the buffers or selected buffers
    if(argvars[0].v_type == kNvarDict)
    {
        dict_st *sel_d = argvars[0].vval.v_dict;

        if(sel_d != NULL)
        {
            dictitem_st *di;
            filtered = true;
            di = tv_dict_find(sel_d, S_LEN("buflisted"));

            if(di != NULL && tv_get_number(&di->di_tv))
            {
                sel_buflisted = true;
            }

            di = tv_dict_find(sel_d, S_LEN("bufloaded"));

            if(di != NULL && tv_get_number(&di->di_tv))
            {
                sel_bufloaded = true;
            }
        }
    }
    else if(argvars[0].v_type != kNvarUnknown)
    {
        // Information about one buffer. Argument specifies the buffer
        if(tv_check_num(&argvars[0]))  // issue errmsg if type error
        {
            emsg_off++;
            argbuf = get_buf_tv(&argvars[0], false);
            emsg_off--;

            if(argbuf == NULL)
            {
                return;
            }
        }
    }

    // Return information about all the buffers or a specified buffer
    FOR_ALL_BUFFERS(buf)
    {
        if(argbuf != NULL && argbuf != buf)
        {
            continue;
        }

        if(filtered
           && ((sel_bufloaded && buf->b_ml.ml_mfp == NULL)
               || (sel_buflisted && !buf->b_p_bl)))
        {
            continue;
        }

        dict_st *const d = get_buffer_info(buf);

        if(d != NULL)
        {
            tv_list_append_dict(rettv->vval.v_list, d);
        }

        if(argbuf != NULL)
        {
            return;
        }
    }
}

/// Get line or list of lines from buffer "buf" into "rettv".
/// Return a range (from start to end) of lines in rettv from
/// the specified buffer. If 'retlist' is TRUE, then the lines
/// are returned as a Vim List.
static void get_buffer_lines(filebuf_st *buf,
                             linenum_kt start,
                             linenum_kt end,
                             int retlist,
                             typval_st *rettv)
{
    uchar_kt *p;
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    if(retlist)
    {
        tv_list_alloc_ret(rettv);
    }

    if(buf == NULL || buf->b_ml.ml_mfp == NULL || start < 0)
    {
        return;
    }

    if(!retlist)
    {
        if(start >= 1 && start <= buf->b_ml.ml_line_count)
        {
            p = ml_get_buf(buf, start, FALSE);
        }
        else
        {
            p = (uchar_kt *)"";
        }

        rettv->vval.v_string = ustrdup(p);
    }
    else
    {
        if(end < start)
        {
            return;
        }

        if(start < 1)
        {
            start = 1;
        }

        if(end > buf->b_ml.ml_line_count)
        {
            end = buf->b_ml.ml_line_count;
        }

        while(start <= end)
        {
            tv_list_append_string(rettv->vval.v_list,
                                  (const char *)ml_get_buf(buf, start++, false),
                                  -1);
        }
    }
}

/// Get the line number from VimL object
///
/// @note Unlike tv_get_lnum(), this one supports only "$" special string.
///
/// @param[in]  tv
/// Object to get value from. Is expected to be a number or
/// a special string "$".
///
/// @param[in]  buf
/// Buffer to take last line number from in case tv is "$".
/// May be NULL, in this case "$" results in zero return.
///
/// @return Line number or 0 in case of error.
static linenum_kt tv_get_lnum_buf(const typval_st *const tv,
                                const filebuf_st *const buf)
FUNC_ATTR_NONNULL_ARG(1)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(tv->v_type == kNvarString
       && tv->vval.v_string != NULL
       && tv->vval.v_string[0] == '$'
       && buf != NULL)
    {
        return buf->b_ml.ml_line_count;
    }

    return tv_get_number_chk(tv, NULL);
}

/// "getbufline()" function
static void f_getbufline(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    filebuf_st *buf = NULL;

    if(tv_check_str_or_nr(&argvars[0]))
    {
        emsg_off++;
        buf = get_buf_tv(&argvars[0], false);
        emsg_off--;
    }

    const linenum_kt lnum = tv_get_lnum_buf(&argvars[1], buf);

    const linenum_kt end = (argvars[2].v_type == kNvarUnknown
                          ? lnum : tv_get_lnum_buf(&argvars[2], buf));

    get_buffer_lines(buf, lnum, end, true, rettv);
}

/// "getbufvar()" function
static void f_getbufvar(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    bool done = false;
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    if(!tv_check_str_or_nr(&argvars[0]))
    {
        goto f_getbufvar_end;
    }

    const char *varname = tv_get_string_chk(&argvars[1]);
    emsg_off++;

    filebuf_st *const buf = get_buf_tv(&argvars[0], false);
    if(buf != NULL && varname != NULL)
    {
        // set curbuf to be our buf, temporarily
        filebuf_st *const save_curbuf = curbuf;
        curbuf = buf;

        if(*varname == '&') // buffer-local-option
        {
            if(varname[1] == NUL)
            {
                // get all buffer-local options in a dict
                dict_st *opts = get_winbuf_options(true);

                if(opts != NULL)
                {
                    rettv->v_type = kNvarDict;
                    rettv->vval.v_dict = opts;
                    opts->dv_refcount++;
                    done = true;
                }
            }
            else if(get_option_tv(&varname, rettv, true) == OK)
            {
                // buffer-local-option
                done = true;
            }
        }
        else
        {
            // Look up the variable.
            // Let getbufvar({nr}, "") return the "b:" dictionary.
            dictitem_st *const v = find_var_in_ht(&curbuf->b_vars->dv_hashtab,
                                                 'b',
                                                 varname,
                                                 strlen(varname),
                                                 false);

            if(v != NULL)
            {
                tv_copy(&v->di_tv, rettv);
                done = true;
            }
        }

        // restore previous notion of curbuf
        curbuf = save_curbuf;
    }

    emsg_off--;

f_getbufvar_end:

    if(!done && argvars[2].v_type != kNvarUnknown)
    {
        // use the default value
        tv_copy(&argvars[2], rettv);
    }
}

/// "getchar()" function
static void f_getchar(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    number_kt n;
    bool error = false;
    no_mapping++;

    for(;;)
    {
        // Position the cursor. Needed after a message that ends in a space,
        // or if event processing caused a redraw.
        ui_cursor_goto(msg_row, msg_col);

        if(argvars[0].v_type == kNvarUnknown)
        {
            // getchar(): blocking wait.
            if(!(char_avail() || using_script() || input_available()))
            {
                input_enable_events();
                (void)os_inchar(NULL, 0, -1, 0);
                input_disable_events();

                if(!multiqueue_empty(main_loop.events))
                {
                    multiqueue_process_events(main_loop.events);
                    continue;
                }
            }

            n = safe_vgetc();
        }
        else if(tv_get_number_chk(&argvars[0], &error) == 1)
        {
            // getchar(1): only check if char avail
            n = vpeekc_any();
        }
        else if(error || vpeekc_any() == NUL)
        {
            // illegal argument or getchar(0) and no char avail: return zero
            n = 0;
        }
        else
        {
            // getchar(0) and char avail: return char
            n = safe_vgetc();
        }

        if(n == K_IGNORE)
        {
            continue;
        }

        break;
    }

    no_mapping--;
    vimvars[VV_MOUSE_WIN].vv_nr = 0;
    vimvars[VV_MOUSE_WINID].vv_nr = 0;
    vimvars[VV_MOUSE_LNUM].vv_nr = 0;
    vimvars[VV_MOUSE_COL].vv_nr = 0;
    rettv->vval.v_number = n;

    if(IS_SPECIAL(n) || mod_mask != 0)
    {
        uchar_kt temp[10]; // modifier: 3, mbyte-char: 6, NUL: 1
        int i = 0;

        // Turn a special key into three bytes, plus modifier.
        if(mod_mask != 0)
        {
            temp[i++] = K_SPECIAL;
            temp[i++] = KS_MODIFIER;
            temp[i++] = mod_mask;
        }

        if(IS_SPECIAL(n))
        {
            temp[i++] = K_SPECIAL;
            temp[i++] = K_SECOND(n);
            temp[i++] = K_THIRD(n);
        }
        else
        {
            i += (*mb_char2bytes)(n, temp + i);
        }

        temp[i++] = NUL;
        rettv->v_type = kNvarString;
        rettv->vval.v_string = ustrdup(temp);

        if(is_mouse_key(n))
        {
            int row = mouse_row;
            int col = mouse_col;
            win_st *win;
            linenum_kt lnum;
            win_st *wp;
            int winnr = 1;

            if(row >= 0 && col >= 0)
            {
                // Find the window at the mouse coordinates and
                // compute the text position.
                win = mouse_find_win(&row, &col);
                (void)mouse_comp_pos(win, &row, &col, &lnum);

                for(wp = firstwin; wp != win; wp = wp->w_next)
                {
                    ++winnr;
                }

                vimvars[VV_MOUSE_WIN].vv_nr = winnr;
                vimvars[VV_MOUSE_WINID].vv_nr = wp->handle;
                vimvars[VV_MOUSE_LNUM].vv_nr = lnum;
                vimvars[VV_MOUSE_COL].vv_nr = col + 1;
            }
        }
    }
}

/// "getcharmod()" function
static void f_getcharmod(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = mod_mask;
}

/// "getcharsearch()" function
static void f_getcharsearch(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                            typval_st *rettv,
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    tv_dict_alloc_ret(rettv);
    dict_st *dict = rettv->vval.v_dict;
    tv_dict_add_str(dict, S_LEN("char"), last_csearch());
    tv_dict_add_nr(dict, S_LEN("forward"), last_csearch_forward());
    tv_dict_add_nr(dict, S_LEN("until"), last_csearch_until());
}

/// "getcmdline()" function
static void f_getcmdline(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = get_cmdline_str();
}

/// "getcmdpos()" function
static void f_getcmdpos(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = get_cmdline_pos() + 1;
}

/// "getcmdtype()" function
static void f_getcmdtype(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = xmallocz(1);
    rettv->vval.v_string[0] = get_cmdline_type();
}

/// "getcmdwintype()" function
static void f_getcmdwintype(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                            typval_st *rettv,
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;
    rettv->vval.v_string = xmallocz(1);
    rettv->vval.v_string[0] = cmdwin_type;
}

/// "getcompletion()" function
static void f_getcompletion(typval_st *argvars,
                            typval_st *rettv,
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    uchar_kt *pat;
    expand_st xpc;
    bool filtered = false;
    int options = WILD_SILENT | WILD_USE_NL | WILD_ADD_SLASH | WILD_NO_BEEP;

    if(argvars[2].v_type != kNvarUnknown)
    {
        filtered = (bool)tv_get_number_chk(&argvars[2], NULL);
    }

    if(p_wic)
    {
        options |= WILD_ICASE;
    }

    // For filtered results, 'wildignore' is used
    if(!filtered)
    {
        options |= WILD_KEEP_ALL;
    }

    if(argvars[0].v_type != kNvarString || argvars[1].v_type != kNvarString)
    {
        EMSG(_(e_invarg));
        return;
    }

    if(strcmp(tv_get_string(&argvars[1]), "cmdline") == 0)
    {
        set_one_cmd_context(&xpc, tv_get_string(&argvars[0]));
        xpc.xp_pattern_len = (int)ustrlen(xpc.xp_pattern);
        goto theend;
    }

    ExpandInit(&xpc);
    xpc.xp_pattern = (uchar_kt *)tv_get_string(&argvars[0]);
    xpc.xp_pattern_len = (int)ustrlen(xpc.xp_pattern);

    xpc.xp_context =
        cmdcomplete_str_to_type((uchar_kt *)tv_get_string(&argvars[1]));

    if(xpc.xp_context == EXPAND_NOTHING)
    {
        EMSG2(_(e_invarg2), argvars[1].vval.v_string);
        return;
    }

    if(xpc.xp_context == EXPAND_MENUS)
    {
        set_context_in_menu_cmd(&xpc, (uchar_kt *)"menu", xpc.xp_pattern, false);
        xpc.xp_pattern_len = (int)ustrlen(xpc.xp_pattern);
    }

    if(xpc.xp_context == EXPAND_CSCOPE)
    {
        set_context_in_cscope_cmd(&xpc,
                                  (const char *)xpc.xp_pattern, CMD_cscope);

        xpc.xp_pattern_len = (int)ustrlen(xpc.xp_pattern);
    }

    if(xpc.xp_context == EXPAND_SIGN)
    {
        set_context_in_sign_cmd(&xpc, xpc.xp_pattern);
        xpc.xp_pattern_len = (int)ustrlen(xpc.xp_pattern);
    }

theend:

    pat = addstar(xpc.xp_pattern, xpc.xp_pattern_len, xpc.xp_context);
    tv_list_alloc_ret(rettv);

    if(pat != NULL)
    {
        ExpandOne(&xpc, pat, NULL, options, WILD_ALL_KEEP);

        for(int i = 0; i < xpc.xp_numfiles; i++)
        {
            tv_list_append_string(rettv->vval.v_list,
                                  (const char *)xpc.xp_files[i], -1);
        }
    }

    xfree(pat);
    ExpandCleanup(&xpc);
}

/// `getcwd([{win}[, {tab}]])` function
///
/// Every scope not specified implies the currently selected scope object.
///
/// @pre  The arguments must be of type number.
/// @pre  There may not be more than two arguments.
/// @pre  An argument may not be -1 if preceding arguments are not all -1.
///
/// @post  The return value will be a string.
static void f_getcwd(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    // Possible scope of working directory to return.
    CdScope scope = kCdScopeInvalid;

    // Numbers of the scope objects (window, tab) we want the working directory
    // of. A `-1` means to skip this scope, a `0` means the current object.
    int scope_number[] = {
        [kCdScopeWindow] = 0, // Number of window to look at.
        [kCdScopeTab   ] = 0, // Number of tab to look at.
    };

    uchar_kt *cwd  = NULL; // Current working directory to print
    uchar_kt *from = NULL; // The original string to copy
    tabpage_st *tp  = curtab; // The tabpage to look at.
    win_st *win = curwin; // The window to look at.

    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    // Pre-conditions and scope extraction together
    for(int i = MIN_CD_SCOPE; i < MAX_CD_SCOPE; i++)
    {
        // If there is no argument there are
        // no more scopes after it, break out.
        if(argvars[i].v_type == kNvarUnknown)
        {
            break;
        }

        if(argvars[i].v_type != kNvarNumber)
        {
            EMSG(_(e_invarg));
            return;
        }

        scope_number[i] = argvars[i].vval.v_number;

        // It is an error for the scope number to be less than `-1`.
        if(scope_number[i] < -1)
        {
            EMSG(_(e_invarg));
            return;
        }

        // Use the narrowest scope the user requested
        if(scope_number[i] >= 0 && scope == kCdScopeInvalid)
        {
            // The scope is the current iteration step.
            scope = i;
        }
        else if(scope_number[i] < 0)
        {
            scope = i + 1;
        }
    }

    // If the user didn't specify anything, default to window scope
    if(scope == kCdScopeInvalid)
    {
        scope = MIN_CD_SCOPE;
    }

    // Find the tabpage by number
    if(scope_number[kCdScopeTab] > 0)
    {
        tp = find_tabpage(scope_number[kCdScopeTab]);

        if(!tp)
        {
            EMSG(_("E5000: Cannot find tab number."));
            return;
        }
    }

    // Find the window in `tp` by number, `NULL` if none.
    if(scope_number[kCdScopeWindow] >= 0)
    {
        if(scope_number[kCdScopeTab] < 0)
        {
            EMSG(_("E5001: Higher scope cannot be -1 if lower scope is >= 0."));
            return;
        }

        if(scope_number[kCdScopeWindow] > 0)
        {
            win = find_win_by_nr(&argvars[0], tp);

            if(!win)
            {
                EMSG(_("E5002: Cannot find window number."));
                return;
            }
        }
    }

    cwd = xmalloc(MAXPATHL);

    switch(scope)
    {
        case kCdScopeWindow:
            assert(win);
            from = win->w_localdir;

            if(from)
            {
                break;
            }

        // fallthrough
        case kCdScopeTab:
            assert(tp);
            from = tp->tp_localdir;

            if(from)
            {
                break;
            }

        // fallthrough
        case kCdScopeGlobal:
            if(globaldir) // `globaldir` is not always set.
            {
                from = globaldir;
            }
            else if(os_dirname(cwd, MAXPATHL) == FAIL) // Get the OS CWD.
            {
                from = (uchar_kt *)""; // Return empty string on failure.
            }

            break;

        case kCdScopeInvalid: // We should never get here
            assert(false);
    }

    if(from)
    {
        xstrncpy((char *)cwd, (char *)from, MAXPATHL);
    }

    rettv->vval.v_string = ustrdup(cwd);

#ifdef BACKSLASH_IN_FILENAME
    slash_adjust(rettv->vval.v_string);
#endif

    xfree(cwd);
}

/// "getfontname()" function
static void f_getfontname(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;
}

/// "getfperm({fname})" function
static void f_getfperm(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char *perm = NULL;
    uchar_kt flags[] = "rwx";
    const char *filename = tv_get_string(&argvars[0]);
    int32_t file_perm = os_getperm(filename);

    if(file_perm >= 0)
    {
        perm = xstrdup("---------");

        for(int i = 0; i < 9; i++)
        {
            if(file_perm & (1 << (8 - i)))
            {
                perm[i] = flags[i % 3];
            }
        }
    }

    rettv->v_type = kNvarString;
    rettv->vval.v_string = (uchar_kt *)perm;
}

/// "getfsize({fname})" function
static void f_getfsize(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *fname = tv_get_string(&argvars[0]);
    rettv->v_type = kNvarNumber;
    fileinfo_st file_info;

    if(os_fileinfo(fname, &file_info))
    {
        uint64_t filesize = os_fileinfo_size(&file_info);

        if(os_isdir((const uchar_kt *)fname))
        {
            rettv->vval.v_number = 0;
        }
        else
        {
            rettv->vval.v_number = (number_kt)filesize;

            // non-perfect check for overflow
            if((uint64_t)rettv->vval.v_number != filesize)
            {
                rettv->vval.v_number = -2;
            }
        }
    }
    else
    {
        rettv->vval.v_number = -1;
    }
}

/// "getftime({fname})" function
static void f_getftime(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *fname = tv_get_string(&argvars[0]);
    fileinfo_st file_info;

    if(os_fileinfo(fname, &file_info))
    {
        rettv->vval.v_number = (number_kt)file_info.stat.st_mtim.tv_sec;
    }
    else
    {
        rettv->vval.v_number = -1;
    }
}

/// "getftype({fname})" function
static void f_getftype(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    uchar_kt *type = NULL;
    char *t;
    const char *fname = tv_get_string(&argvars[0]);
    rettv->v_type = kNvarString;
    fileinfo_st file_info;

    if(os_fileinfo_link(fname, &file_info))
    {
        uint64_t mode = file_info.stat.st_mode;

#ifdef S_ISREG
        if(S_ISREG(mode))
        {
            t = "file";
        }
        else if(S_ISDIR(mode))
        {
            t = "dir";
        }
    #ifdef S_ISLNK
        else if(S_ISLNK(mode))
        {
            t = "link";
        }
    #endif

    #ifdef S_ISBLK
        else if(S_ISBLK(mode))
        {
            t = "bdev";
        }
    #endif

    #ifdef S_ISCHR
        else if(S_ISCHR(mode))
        {
            t = "cdev";
        }
    #endif

    #ifdef S_ISFIFO
        else if (S_ISFIFO(mode))
        {
            t = "fifo";
        }
    #endif

    #ifdef S_ISSOCK
        else if(S_ISSOCK(mode))
        {
            t = "fifo";
        }
    #endif
        else
        {
            t = "other";
        }
#else
    #ifdef S_IFMT
        switch(mode & S_IFMT)
        {
            case S_IFREG:
                t = "file";
                break;

            case S_IFDIR:
                t = "dir";
                break;
        #ifdef S_IFLNK
            case S_IFLNK:
                t = "link";
                break;
        #endif

        #ifdef S_IFBLK
            case S_IFBLK:
                t = "bdev";
                break;
        #endif

        #ifdef S_IFCHR
            case S_IFCHR:
                t = "cdev";
                break;
        #endif

        #ifdef S_IFIFO
            case S_IFIFO:
                t = "fifo";
                break;
        #endif

        #ifdef S_IFSOCK
            case S_IFSOCK:
                t = "socket";
                break;
        #endif
            default:
                t = "other";
        }
    #else
        if(os_isdir((const uchar_kt *)fname))
        {
            t = "dir";
        }
        else
        {
            t = "file";
        }
    #endif
#endif
        type = ustrdup((uchar_kt *)t);
    }

    rettv->vval.v_string = type;
}

/// "getline(lnum, [end])" function
static void f_getline(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    linenum_kt end;
    bool retlist;
    const linenum_kt lnum = tv_get_lnum(argvars);

    if(argvars[1].v_type == kNvarUnknown)
    {
        end = 0;
        retlist = false;
    }
    else
    {
        end = tv_get_lnum(&argvars[1]);
        retlist = true;
    }

    get_buffer_lines(curbuf, lnum, end, retlist, rettv);
}

static void get_qf_loc_list(int is_qf,
                            win_st *wp,
                            typval_st *what_arg,
                            typval_st *rettv)
{
    if(what_arg->v_type == kNvarUnknown)
    {
        tv_list_alloc_ret(rettv);

        if(is_qf || wp != NULL)
        {
            (void)get_errorlist(wp, -1, rettv->vval.v_list);
        }
    }
    else
    {
        tv_dict_alloc_ret(rettv);

        if(is_qf || wp != NULL)
        {
            if(what_arg->v_type == kNvarDict)
            {
                dict_st *d = what_arg->vval.v_dict;

                if(d != NULL)
                {
                    get_errorlist_properties(wp, d, rettv->vval.v_dict);
                }
            }
            else
            {
                EMSG(_(e_dictreq));
            }
        }
    }
}

/// "getloclist()" function
static void f_getloclist(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    win_st *wp = find_win_by_nr(&argvars[0], NULL);
    get_qf_loc_list(false, wp, &argvars[1], rettv);
}

///"getmatches()" function
static void f_getmatches(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    matchitem_st *cur = curwin->w_match_head;
    int i;
    tv_list_alloc_ret(rettv);

    while(cur != NULL)
    {
        dict_st *dict = tv_dict_alloc();

        if(cur->match.regprog == NULL)
        {
            // match added with matchaddpos()
            for(i = 0; i < MAX_POS_NUM_MATCH; ++i)
            {
                cpos_st   *llpos;
                char buf[6];
                llpos = &cur->pos.pos[i];

                if(llpos->lnum == 0)
                {
                    break;
                }

                list_st *l = tv_list_alloc();
                tv_list_append_number(l, (number_kt)llpos->lnum);

                if(llpos->col > 0)
                {
                    tv_list_append_number(l, (number_kt)llpos->col);
                    tv_list_append_number(l, (number_kt)llpos->len);
                }

                int len = snprintf(buf, sizeof(buf), "pos%d", i + 1);
                assert((size_t)len < sizeof(buf));
                tv_dict_add_list(dict, buf, (size_t)len, l);
            }
        }
        else
        {
            tv_dict_add_str(dict, S_LEN("pattern"), (const char *)cur->pattern);
        }

        tv_dict_add_str(dict, S_LEN("group"), (const char *)syn_id2name(cur->hlg_id));
        tv_dict_add_nr(dict, S_LEN("priority"), (number_kt)cur->priority);
        tv_dict_add_nr(dict, S_LEN("id"), (number_kt)cur->id);

        if(cur->conceal_char)
        {
            char buf[MB_MAXBYTES + 1];
            buf[(*mb_char2bytes)((int)cur->conceal_char, (uchar_kt *)buf)] = NUL;
            tv_dict_add_str(dict, S_LEN("conceal"), buf);
        }

        tv_list_append_dict(rettv->vval.v_list, dict);
        cur = cur->next;
    }
}

/// "getpid()" function
static void f_getpid(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = os_get_pid();
}

static void getpos_both(typval_st *argvars, typval_st *rettv, bool getcurpos)
{
    apos_st *fp;
    int fnum = -1;

    if(getcurpos)
    {
        fp = &curwin->w_cursor;
    }
    else
    {
        fp = var2fpos(&argvars[0], true, &fnum);
    }

    list_st *l = tv_list_alloc_ret(rettv);

    tv_list_append_number(l,
                          (fnum != -1)
                          ? (number_kt)fnum
                          : (number_kt)0);

    tv_list_append_number(l,
                          ((fp != NULL)
                           ? (number_kt)fp->lnum
                           : (number_kt)0));

    tv_list_append_number(l, ((fp != NULL)
                              ? (number_kt)(fp->col == MAXCOL
                                              ? MAXCOL : fp->col + 1)
                              : (number_kt)0));

    tv_list_append_number(l,
                          (fp != NULL)
                          ? (number_kt)fp->coladd
                          : (number_kt)0);

    if(getcurpos)
    {
        update_curswant();
        tv_list_append_number(l,
                              (curwin->w_curswant == MAXCOL
                              ? (number_kt)MAXCOL
                              : (number_kt)curwin->w_curswant + 1));
    }
}

/// "getcurpos(string)" function
static void f_getcurpos(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    getpos_both(argvars, rettv, true);
}

/// "getpos(string)" function
static void f_getpos(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    getpos_both(argvars, rettv, false);
}

/// "getqflist()" functions
static void f_getqflist(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    get_qf_loc_list(true, NULL, &argvars[0], rettv);
}

/// "getreg()" function
static void f_getreg(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *strregname;
    int arg2 = false;
    bool return_list = false;
    bool error = false;

    if(argvars[0].v_type != kNvarUnknown)
    {
        strregname = tv_get_string_chk(&argvars[0]);
        error = strregname == NULL;

        if(argvars[1].v_type != kNvarUnknown)
        {
            arg2 = tv_get_number_chk(&argvars[1], &error);

            if(!error && argvars[2].v_type != kNvarUnknown)
            {
                return_list = tv_get_number_chk(&argvars[2], &error);
            }
        }
    }
    else
    {
        strregname = (const char *)vimvars[VV_REG].vv_str;
    }

    if(error)
    {
        return;
    }

    int regname = (uint8_t)(strregname == NULL ? '"' : *strregname);

    if(regname == 0)
    {
        regname = '"';
    }

    if(return_list)
    {
        rettv->v_type = kNvarList;

        rettv->vval.v_list =
            get_reg_contents(regname, (arg2 ? kGRegExprSrc : 0) | kGRegList);

        if(rettv->vval.v_list == NULL)
        {
            rettv->vval.v_list = tv_list_alloc();
        }

        rettv->vval.v_list->lv_refcount++;
    }
    else
    {
        rettv->v_type = kNvarString;

        rettv->vval.v_string =
            get_reg_contents(regname, arg2 ? kGRegExprSrc : 0);
    }
}

/// "getregtype()" function
static void f_getregtype(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *strregname;

    if(argvars[0].v_type != kNvarUnknown)
    {
        strregname = tv_get_string_chk(&argvars[0]);

        if(strregname == NULL) // Type error; errmsg already given.
        {
            rettv->v_type = kNvarString;
            rettv->vval.v_string = NULL;
            return;
        }
    }
    else
    {
        // Default to v:register.
        strregname = (const char *)vimvars[VV_REG].vv_str;
    }

    int regname = (uint8_t)(strregname == NULL ? '"' : *strregname);

    if(regname == 0)
    {
        regname = '"';
    }

    columnum_kt reglen = 0;
    char buf[NUMBUFLEN + 2];
    motion_type_et reg_type = get_reg_type(regname, &reglen);

    format_reg_type(reg_type, reglen, buf, ARRAY_SIZE(buf));
    rettv->v_type = kNvarString;
    rettv->vval.v_string = (uchar_kt *)xstrdup(buf);
}

/// Returns information (variables, options, etc.) about a tab page
/// as a dictionary.
static dict_st *get_tabpage_info(tabpage_st *tp, int tp_idx)
{
    dict_st *const dict = tv_dict_alloc();
    tv_dict_add_nr(dict, S_LEN("tabnr"), tp_idx);
    list_st *const l = tv_list_alloc();

    FOR_ALL_WINDOWS_IN_TAB(wp, tp)
    {
        tv_list_append_number(l, (number_kt)wp->handle);
    }

    tv_dict_add_list(dict, S_LEN("windows"), l);

    // Make a reference to tabpage variables
    tv_dict_add_dict(dict, S_LEN("variables"), tp->tp_vars);
    return dict;
}

/// "gettabinfo()" function
static void f_gettabinfo(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    tabpage_st *tparg = NULL;
    tv_list_alloc_ret(rettv);

    if(argvars[0].v_type != kNvarUnknown)
    {
        // Information about one tab page
        tparg = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));

        if(tparg == NULL)
        {
            return;
        }
    }

    // Get information about a specific tab page or all tab pages
    int tpnr = 0;
    FOR_ALL_TABS(tp)
    {
        tpnr++;

        if(tparg != NULL && tp != tparg)
        {
            continue;
        }

        dict_st *const d = get_tabpage_info(tp, tpnr);

        if(d != NULL)
        {
            tv_list_append_dict(rettv->vval.v_list, d);
        }

        if(tparg != NULL)
        {
            return;
        }
    }
}

/// "gettabvar()" function
static void f_gettabvar(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    win_st *oldcurwin;
    tabpage_st *tp, *oldtabpage;
    dictitem_st  *v;
    bool done = false;
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;
    const char *const varname = tv_get_string_chk(&argvars[1]);
    tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));

    if(tp != NULL && varname != NULL)
    {
        // Set tp to be our tabpage, temporarily.  Also set the window to the
        // first window in the tabpage, otherwise the window is not valid.
        win_st *window = tp->tp_firstwin == NULL ? firstwin : tp->tp_firstwin;

        if(switch_win(&oldcurwin, &oldtabpage, window, tp, true) == OK)
        {
            // look up the variable
            // Let gettabvar({nr}, "") return the "t:" dictionary.
            v = find_var_in_ht(&tp->tp_vars->dv_hashtab,
                               't',
                               varname,
                               strlen(varname),
                               false);

            if(v != NULL)
            {
                tv_copy(&v->di_tv, rettv);
                done = true;
            }
        }

        // restore previous notion of curwin
        restore_win(oldcurwin, oldtabpage, TRUE);
    }

    if(!done && argvars[2].v_type != kNvarUnknown)
    {
        tv_copy(&argvars[2], rettv);
    }
}

/// "gettabwinvar()" function
static void f_gettabwinvar(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    getwinvar(argvars, rettv, 1);
}

/// Returns information about a window as a dictionary.
static dict_st *get_win_info(win_st *wp, int16_t tpnr, int16_t winnr)
{
    dict_st *const dict = tv_dict_alloc();

    tv_dict_add_nr(dict, S_LEN("tabnr"), tpnr);
    tv_dict_add_nr(dict, S_LEN("winnr"), winnr);
    tv_dict_add_nr(dict, S_LEN("winid"), wp->handle);
    tv_dict_add_nr(dict, S_LEN("height"), wp->w_height);
    tv_dict_add_nr(dict, S_LEN("width"), wp->w_width);
    tv_dict_add_nr(dict, S_LEN("bufnr"), wp->w_buffer->b_id);
    tv_dict_add_nr(dict, S_LEN("quickfix"), bt_quickfix(wp->w_buffer));

    tv_dict_add_nr(dict,
                   S_LEN("loclist"),
                   (bt_quickfix(wp->w_buffer) && wp->w_llist_ref != NULL));

    // Add a reference to window variables
    tv_dict_add_dict(dict, S_LEN("variables"), wp->w_vars);

    return dict;
}

/// "getwininfo()" function
static void f_getwininfo(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    win_st *wparg = NULL;
    tv_list_alloc_ret(rettv);

    if(argvars[0].v_type != kNvarUnknown)
    {
        wparg = win_id2wp(argvars);

        if(wparg == NULL)
        {
            return;
        }
    }

    // Collect information about either all the windows across all the tab
    // pages or one particular window.
    int16_t tabnr = 0;

    FOR_ALL_TABS(tp)
    {
        tabnr++;
        int16_t winnr = 0;

        FOR_ALL_WINDOWS_IN_TAB(wp, tp)
        {
            if(wparg != NULL && wp != wparg)
            {
                continue;
            }

            winnr++;
            dict_st *const d = get_win_info(wp, tabnr, winnr);

            if(d != NULL)
            {
                tv_list_append_dict(rettv->vval.v_list, d);
            }

            if(wparg != NULL)
            {
                // found information about a specific window
                return;
            }
        }
    }
}

/// nvimL: getwinposx() function
static void f_getwinposx(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = -1;
}

/// "getwinposy()" function
static void f_getwinposy(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = -1;
}

/// Find window specified by @b vp in tabpage @b tp
///
/// @param vp
/// @param tp NULL for current tab page
static win_st *find_win_by_nr(typval_st *vp, tabpage_st *tp)
{
    int nr = (int)tv_get_number_chk(vp, NULL);

    if(nr < 0)
    {
        return NULL;
    }

    if(nr == 0)
    {
        return curwin;
    }

    // This method accepts NULL as an alias for curtab.
    if(tp == NULL)
    {
        tp = curtab;
    }

    FOR_ALL_WINDOWS_IN_TAB(wp, tp)
    {
        if(nr >= LOWEST_WIN_ID)
        {
            if(wp->handle == nr)
            {
                return wp;
            }
        }
        else if(--nr <= 0)
        {
            return wp;
        }
    }

    return NULL;
}

/// Find window specified by "wvp" in tabpage "tvp".
static win_st *find_tabwin(typval_st *wvp, typval_st *tvp)
{
    win_st *wp = NULL;
    tabpage_st *tp = NULL;

    if(wvp->v_type != kNvarUnknown)
    {
        if(tvp->v_type != kNvarUnknown)
        {
            long n = tv_get_number(tvp);

            if(n >= 0)
            {
                tp = find_tabpage(n);
            }
        }
        else
        {
            tp = curtab;
        }

        if(tp != NULL)
        {
            wp = find_win_by_nr(wvp, tp);
        }
    }
    else
    {
        wp = curwin;
    }

    return wp;
}

/// "getwinvar()" function
static void f_getwinvar(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    getwinvar(argvars, rettv, 0);
}

/// getwinvar() and gettabwinvar()
///
/// @param argvars
/// @param rettv
/// @param off      1 for gettabwinvar()
static void getwinvar(typval_st *argvars, typval_st *rettv, int off)
{
    win_st *win, *oldcurwin;
    dictitem_st *v;
    tabpage_st *tp = NULL;
    tabpage_st *oldtabpage = NULL;
    bool done = false;

    if(off == 1)
    {
        tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
    }
    else
    {
        tp = curtab;
    }

    win = find_win_by_nr(&argvars[off], tp);
    const char *varname = tv_get_string_chk(&argvars[off + 1]);
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;
    emsg_off++;

    if(win != NULL && varname != NULL)
    {
        // Set curwin to be our win, temporarily.  Also set the tabpage,
        // otherwise the window is not valid. Only do this when needed,
        // autocommands get blocked.
        bool need_switch_win = tp != curtab || win != curwin;

        if(!need_switch_win
           || switch_win(&oldcurwin, &oldtabpage, win, tp, true) == OK)
        {
            if(*varname == '&')
            {
                if(varname[1] == NUL)
                {
                    // get all window-local options in a dict
                    dict_st *opts = get_winbuf_options(false);

                    if(opts != NULL)
                    {
                        rettv->v_type = kNvarDict;
                        rettv->vval.v_dict = opts;
                        opts->dv_refcount++;
                        done = true;
                    }
                }
                else if(get_option_tv(&varname, rettv, 1) == OK)
                {
                    // window-local-option
                    done = true;
                }
            }
            else
            {
                // Look up the variable.
                // Let getwinvar({nr}, "") return the "w:" dictionary.
                v = find_var_in_ht(&win->w_vars->dv_hashtab,
                                   'w',
                                   varname,
                                   strlen(varname),
                                   false);

                if(v != NULL)
                {
                    tv_copy(&v->di_tv, rettv);
                    done = true;
                }
            }
        }

        if(need_switch_win)
        {
            // restore previous notion of curwin
            restore_win(oldcurwin, oldtabpage, true);
        }
    }

    emsg_off--;

    if(!done && argvars[off + 2].v_type != kNvarUnknown)
    {
        // use the default return value
        tv_copy(&argvars[off + 2], rettv);
    }
}

/// "glob()" function
static void f_glob(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int options = WILD_SILENT|WILD_USE_NL;
    expand_st xpc;
    bool error = false;

    // When the optional second argument is non-zero, don't remove matches
    // for 'wildignore' and don't put matches for 'suffixes' at the end.
    rettv->v_type = kNvarString;

    if(argvars[1].v_type != kNvarUnknown)
    {
        if(tv_get_number_chk(&argvars[1], &error))
        {
            options |= WILD_KEEP_ALL;
        }

        if(argvars[2].v_type != kNvarUnknown)
        {
            if(tv_get_number_chk(&argvars[2], &error))
            {
                rettv->v_type = kNvarList;
                rettv->vval.v_list = NULL;
            }

            if(argvars[3].v_type != kNvarUnknown
               && tv_get_number_chk(&argvars[3], &error))
            {
                options |= WILD_ALLLINKS;
            }
        }
    }

    if(!error)
    {
        ExpandInit(&xpc);
        xpc.xp_context = EXPAND_FILES;

        if(p_wic)
        {
            options += WILD_ICASE;
        }

        if(rettv->v_type == kNvarString)
        {
            rettv->vval.v_string =
                ExpandOne(&xpc,
                          (uchar_kt *)tv_get_string(&argvars[0]),
                          NULL,
                          options,
                          WILD_ALL);
        }
        else
        {
            tv_list_alloc_ret(rettv);

            ExpandOne(&xpc,
                      (uchar_kt *)tv_get_string(&argvars[0]),
                      NULL,
                      options,
                      WILD_ALL_KEEP);

            for(int i = 0; i < xpc.xp_numfiles; i++)
            {
                tv_list_append_string(rettv->vval.v_list,
                                      (const char *)xpc.xp_files[i],
                                      -1);
            }

            ExpandCleanup(&xpc);
        }
    }
    else
    {
        rettv->vval.v_string = NULL;
    }
}

/// "globpath()" function
static void f_globpath(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int flags = 0; // Flags for globpath.
    bool error = false;

    // Return a string, or a list if the optional third argument is non-zero.
    rettv->v_type = kNvarString;

    if(argvars[2].v_type != kNvarUnknown)
    {
        // When the optional second argument is non-zero, don't remove matches
        // for 'wildignore' and don't put matches for 'suffixes' at the end.
        if(tv_get_number_chk(&argvars[2], &error))
        {
            flags |= WILD_KEEP_ALL;
        }

        if(argvars[3].v_type != kNvarUnknown)
        {
            if(tv_get_number_chk(&argvars[3], &error))
            {
                rettv->v_type = kNvarList;
                rettv->vval.v_list = NULL;
            }

            if(argvars[4].v_type != kNvarUnknown
               && tv_get_number_chk(&argvars[4], &error))
            {
                flags |= WILD_ALLLINKS;
            }
        }
    }

    char buf1[NUMBUFLEN];
    const char *const file = tv_get_string_buf_chk(&argvars[1], buf1);

    if(file != NULL && !error)
    {
        garray_st ga;
        ga_init(&ga, (int)sizeof(uchar_kt *), 10);

        globpath((uchar_kt *)tv_get_string(&argvars[0]),
                (uchar_kt *)file,
                &ga,
                flags);

        if(rettv->v_type == kNvarString)
        {
            rettv->vval.v_string = ga_concat_strings_sep(&ga, "\n");
        }
        else
        {
            tv_list_alloc_ret(rettv);

            for(int i = 0; i < ga.ga_len; i++)
            {
                tv_list_append_string(rettv->vval.v_list,
                                      ((const char **)(ga.ga_data))[i],
                                      -1);
            }
        }

        ga_clear_strings(&ga);
    }
    else
    {
        rettv->vval.v_string = NULL;
    }
}

/// NvimL: glob2regpat() function
static void f_glob2regpat(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const pat = tv_get_string_chk(&argvars[0]); // NULL on type error
    rettv->v_type = kNvarString;

    rettv->vval.v_string =
        ((pat == NULL)
         ? NULL : file_pat_to_reg_pat((uchar_kt *)pat, NULL, NULL, false));
}

/// nvim: has() function
static void f_has(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    // the basic check list
    static const char *const has_list[] =
    {
    // platform: unix/linux, windows, macos
    #ifdef HOST_OS_LINUX
        "os_linux",
    #endif

    #ifdef HOST_OS_WINDOWS
        "os_windows",
    #endif

    #ifdef HOST_OS_MACOS
        "os_macos",
    #endif

    // arch: arch 32
    #ifdef HOST_OS_ARCH_32
        "os_arch_32",

        #ifdef HOST_OS_LINUX
        "os_linux32",
        #endif

        #if defined(HOST_OS_WINDOWS)
        "os_windows32",
        #endif

        #ifdef HOST_OS_MACOS
        "os_macos32",
        #endif
    #endif

    // arch: arch 64
    #ifdef HOST_OS_ARCH_64
        "os_arch_64",

        #ifdef HOST_OS_LINUX
        "os_linux64",
        #endif

        #if defined(HOST_OS_WINDOWS)
        "os_windows64",
        #endif

        #ifdef HOST_OS_MACOS
        "os_macos64",
        #endif
    #endif

    // common staff
        "arabic",
        "autocmd",
        "browsefilter",
        "byte_offset",
        "cindent",
        "cmdline_compl",
        "cmdline_hist",
        "comments",
        "conceal",
        "cscope",
        "cursorbind",
        "cursorshape",
        "dialog_con",
        "diff",
        "digraphs",
        "eval",
        "ex_extra",
        "extra_search",
        "farsi",
        "file_in_path",
        "filterpipe",
        "find_in_path",
        "float",
        "folding",
        "fname_case",
    #ifdef HOST_OS_LINUX
        "fork",
    #endif
        "gettext",
    #if defined(HAVE_HDR_ICONV_H) && defined(USE_ICONV)
        "iconv",
    #endif
        "insert_expand",
        "jumplist",
        "keymap",
        "lambda",
        "langmap",
        "libcall",
        "linebreak",
        "lispindent",
        "listcmds",
        "localmap",
        "menu",
        "mksession",
        "modify_fname",
        "mouse",
        "multi_byte",
        "multi_lang",
    #ifdef NVIM_LANGUAGE_DEBUG_ENABLE
        "nviml_debug",
    #else
        "nviml_release",
    #endif
        "packages",
        "path_extra",
        "persistent_undo",
        "postscript",
        "printer",
        "profile",
        "reltime",
        "quickfix",
        "rightleft",
        "scrollbind",
        "showcmd",
        "cmdline_info",
        "shada",
        "signs",
        "smartindent",
        "startuptime",
        "statusline",
        "spell",
        "syntax",
        "tablineat",
        "tag_binary",
        "tag_old_static",
        "termguicolors",
        "termresponse",
        "textobjects",
        "timers",
        "title",
        "user_commands",
        "vertsplit",
        "virtualedit",
        "visual",
        "visualextra",
        "vreplace",
        "wildignore",
        "wildmenu",
        "windows",
        "winaltkeys",
        "writebackup",
    };

    bool status = false;
    const char *const name = tv_get_string(&argvars[0]);

    for(size_t i = 0; i < ARRAY_SIZE(has_list); i++)
    {
        if(ustricmp(name, has_list[i]) == 0)
        {
            status = true;
            break;
        }
    }

    if(!status)
    {
        // check patch
        if(ustrnicmp(name, "patch", 5) == 0)
        {
            if(name[5] == ':') // patch:123456
            {
                status = has_nvim_patch(atoi(name + 5));
            }
            else if(name[5] == '-') // patch-0.2@123456
            {
                if(ascii_isdigit(name[6]) // nvim version: major
                   && name[7] == '.'
                   && ascii_isdigit(name[8]) // nvim version: minor
                   && name[9] == '@'
                   && strlen(name) >= 11)
                {
                    int nvim_major = atoi(name + 6);
                    int nvim_minor = atoi(name + 8);

                    status = (nvim_major < NVIM_VERSION_MAJOR
                              || (nvim_major == NVIM_VERSION_MAJOR
                                  && (nvim_minor < NVIM_VERSION_MINOR
                                      || (nvim_minor == NVIM_VERSION_MINOR
                                          && has_nvim_patch(atoi(name + 10))))));
                }
            }
        }
        else if(ustrnicmp(name, "nvim@", 5) == 0)
        {
            status = has_nvim_version(name + 5); // nvim@x.y.z
        }
        else if(ustrnicmp(name, "gkide@", 6) == 0)
        {
            status = has_gkide_version(name + 6); // gkide@x.y.z
        }
        else if(ustricmp(name, "nvim_starting") == 0)
        {
            status = (runtime_status != kRS_Normal);
        }
#if defined(USE_ICONV) && defined(DYNAMIC_ICONV)
        else if(ustricmp(name, "iconv") == 0)
        {
            status = iconv_enabled(false);
        }
#endif
        else if(ustricmp(name, "syntax_items") == 0)
        {
            status = syntax_present(curwin);
#ifdef HOST_OS_LINUX
        }
        else if(ustricmp(name, "unnamedplus") == 0)
        {
            status = eval_has_provider("clipboard");
#endif
        }
    }

    if(!status && eval_has_provider(name))
    {
        status = true;
    }

    rettv->vval.v_number = status;
}

/// NvimL: has_key() function
static void f_has_key(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[0].v_type != kNvarDict)
    {
        EMSG(_(e_dictreq));
        return;
    }

    if(argvars[0].vval.v_dict == NULL)
    {
        return;
    }

    rettv->vval.v_number =
        tv_dict_find(argvars[0].vval.v_dict, tv_get_string(&argvars[1]), -1) != NULL;
}

/// `haslocaldir([{win}[, {tab}]])` function
///
/// Returns `1` if the scope object has a local directory, `0` otherwise. If a
/// scope object is not specified the current one is implied. This function
/// share a lot of code with `f_getcwd`.
///
/// @pre  The arguments must be of type number.
/// @pre  There may not be more than two arguments.
/// @pre  An argument may not be -1 if preceding arguments are not all -1.
///
/// @post  The return value will be either the number `1` or `0`.
static void f_haslocaldir(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    // Possible scope of working directory to return.
    CdScope scope = kCdScopeInvalid;

    // Numbers of the scope objects (window, tab) we want the working directory
    // of. A `-1` means to skip this scope, a `0` means the current object.
    int scope_number[] =
    {
        [kCdScopeWindow] = 0, // Number of window to look at.
        [kCdScopeTab   ] = 0, // Number of tab to look at.
    };

    tabpage_st *tp  = curtab; // The tabpage to look at.
    win_st *win = curwin; // The window to look at.

    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    // Pre-conditions and scope extraction together
    for(int i = MIN_CD_SCOPE; i < MAX_CD_SCOPE; i++)
    {
        if(argvars[i].v_type == kNvarUnknown)
        {
            break;
        }

        if(argvars[i].v_type != kNvarNumber)
        {
            EMSG(_(e_invarg));
            return;
        }

        scope_number[i] = argvars[i].vval.v_number;

        if(scope_number[i] < -1)
        {
            EMSG(_(e_invarg));
            return;
        }

        // Use the narrowest scope the user requested
        if(scope_number[i] >= 0 && scope == kCdScopeInvalid)
        {
            // The scope is the current iteration step.
            scope = i;
        }
        else if(scope_number[i] < 0)
        {
            scope = i + 1;
        }
    }

    // If the user didn't specify anything, default to window scope
    if(scope == kCdScopeInvalid)
    {
        scope = MIN_CD_SCOPE;
    }

    // Find the tabpage by number
    if(scope_number[kCdScopeTab] > 0)
    {
        tp = find_tabpage(scope_number[kCdScopeTab]);

        if(!tp)
        {
            EMSG(_("E5000: Cannot find tab number."));
            return;
        }
    }

    // Find the window in `tp` by number, `NULL` if none.
    if(scope_number[kCdScopeWindow] >= 0)
    {
        if(scope_number[kCdScopeTab] < 0)
        {
            EMSG(_("E5001: Higher scope cannot be -1 if lower scope is >= 0."));
            return;
        }

        if(scope_number[kCdScopeWindow] > 0)
        {
            win = find_win_by_nr(&argvars[0], tp);

            if(!win)
            {
                EMSG(_("E5002: Cannot find window number."));
                return;
            }
        }
    }

    switch(scope)
    {
        case kCdScopeWindow:
            assert(win);
            rettv->vval.v_number = win->w_localdir ? 1 : 0;
            break;

        case kCdScopeTab:
            assert(tp);
            rettv->vval.v_number = tp->tp_localdir ? 1 : 0;
            break;

        case kCdScopeGlobal:
            // The global scope never has a local directory
            rettv->vval.v_number = 0;
            break;

        case kCdScopeInvalid:
            // We should never get here
            assert(false);
    }
}

/// "hasmapto()" function
static void f_hasmapto(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *mode;
    const char *const name = tv_get_string(&argvars[0]);
    bool abbr = false;
    char buf[NUMBUFLEN];

    if(argvars[1].v_type == kNvarUnknown)
    {
        mode = "nvo";
    }
    else
    {
        mode = tv_get_string_buf(&argvars[1], buf);

        if(argvars[2].v_type != kNvarUnknown)
        {
            abbr = tv_get_number(&argvars[2]);
        }
    }

    if(map_to_exists(name, mode, abbr))
    {
        rettv->vval.v_number = true;
    }
    else
    {
        rettv->vval.v_number = false;
    }
}

/// "histadd()" function
static void f_histadd(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    history_type_et histype;
    rettv->vval.v_number = false;

    if(check_restricted() || check_secure())
    {
        return;
    }

    const char *str = tv_get_string_chk(&argvars[0]); // NULL on type error
    histype = str != NULL
              ? get_histtype(str, strlen(str), false) : kHistInvalid;

    if(histype != kHistInvalid)
    {
        char buf[NUMBUFLEN];
        str = tv_get_string_buf(&argvars[1], buf);

        if(*str != NUL)
        {
            init_history();
            add_to_history(histype, (uchar_kt *)str, false, NUL);
            rettv->vval.v_number = true;
            return;
        }
    }
}

/// "histdel()" function
static void f_histdel(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int n;
    const char *const str = tv_get_string_chk(&argvars[0]); // NULL on type error

    if(str == NULL)
    {
        n = 0;
    }
    else if(argvars[1].v_type == kNvarUnknown)
    {
        // only one argument: clear entire history
        n = clr_history(get_histtype(str, strlen(str), false));
    }
    else if(argvars[1].v_type == kNvarNumber)
    {
        // index given: remove that entry
        n = del_history_idx(get_histtype(str, strlen(str), false),
                            (int)tv_get_number(&argvars[1]));
    }
    else
    {
        // string given: remove all matching entries
        char buf[NUMBUFLEN];
        n = del_history_entry(get_histtype(str, strlen(str), false),
                              (uchar_kt *)tv_get_string_buf(&argvars[1], buf));
    }

    rettv->vval.v_number = n;
}

/// "histget()" function
static void f_histget(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int idx;
    history_type_et type;
    // NULL on type error
    const char *const str = tv_get_string_chk(&argvars[0]);

    if(str == NULL)
    {
        rettv->vval.v_string = NULL;
    }
    else
    {
        type = get_histtype(str, strlen(str), false);

        if(argvars[1].v_type == kNvarUnknown)
        {
            idx = get_history_idx(type);
        }
        else
        {
            idx = (int)tv_get_number_chk(&argvars[1], NULL);
        }

        // -1 on type error
        rettv->vval.v_string = ustrdup(get_history_entry(type, idx));
    }

    rettv->v_type = kNvarString;
}

/// "histnr()" function
static void f_histnr(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int i;
    const char *const history = tv_get_string_chk(&argvars[0]);

    i = history == NULL
        ? kHistCmd - 1 : get_histtype(history, strlen(history), false);

    if(i != kHistInvalid)
    {
        i = get_history_idx(i);
    }
    else
    {
        i = -1;
    }

    rettv->vval.v_number = i;
}

/// "highlightID(name)" function
static void f_hlID(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number =
        syn_name2id((const uchar_kt *)tv_get_string(&argvars[0]));
}

/// "highlight_exists()" function
static void f_hlexists(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number =
        highlight_exists((const uchar_kt *)tv_get_string(&argvars[0]));
}

/// "hostname()" function
static void f_hostname(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char hostname[256];
    os_get_hostname(hostname, 256);
    rettv->v_type = kNvarString;
    rettv->vval.v_string = ustrdup((uchar_kt *)hostname);
}

/// iconv() function
static void f_iconv(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    vimconv_st vimconv;
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;
    const char *const str = tv_get_string(&argvars[0]);
    char buf1[NUMBUFLEN];

    uchar_kt *const from =
        enc_canonize(enc_skip((uchar_kt *)tv_get_string_buf(&argvars[1], buf1)));

    char buf2[NUMBUFLEN];

    uchar_kt *const to =
        enc_canonize(enc_skip((uchar_kt *)tv_get_string_buf(&argvars[2], buf2)));

    vimconv.vc_type = CONV_NONE;
    convert_setup(&vimconv, from, to);

    // If the encodings are equal, no conversion needed.
    if(vimconv.vc_type == CONV_NONE)
    {
        rettv->vval.v_string = (uchar_kt *)xstrdup(str);
    }
    else
    {
        rettv->vval.v_string = string_convert(&vimconv, (uchar_kt *)str, NULL);
    }

    convert_setup(&vimconv, NULL, NULL);
    xfree(from);
    xfree(to);
}

/// "indent()" function
static void f_indent(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const linenum_kt lnum = tv_get_lnum(argvars);

    if(lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
        rettv->vval.v_number = get_indent_lnum(lnum);
    }
    else
    {
        rettv->vval.v_number = -1;
    }
}

/// "index()" function
static void f_index(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    list_st *l;
    listitem_st  *item;
    long idx = 0;
    int ic = FALSE;
    rettv->vval.v_number = -1;

    if(argvars[0].v_type != kNvarList)
    {
        EMSG(_(e_listreq));
        return;
    }

    l = argvars[0].vval.v_list;

    if(l != NULL)
    {
        item = l->lv_first;

        if(argvars[2].v_type != kNvarUnknown)
        {
            bool error = false;

            // Start at specified item. Use the cached index that tv_list_find()
            // sets, so that a negative number also works.
            item = tv_list_find(l, tv_get_number_chk(&argvars[2], &error));
            idx = l->lv_idx;

            if(argvars[3].v_type != kNvarUnknown)
            {
                ic = tv_get_number_chk(&argvars[3], &error);
            }

            if(error)
            {
                item = NULL;
            }
        }

        for(; item != NULL; item = item->li_next, ++idx)
        {
            if(tv_equal(&item->li_tv, &argvars[1], ic, FALSE))
            {
                rettv->vval.v_number = idx;
                break;
            }
        }
    }
}

static int inputsecret_flag = 0;

/// This function is used by f_input() and f_inputdialog() functions. The third
/// argument to f_input() specifies the type of completion to use at the
/// prompt. The third argument to f_inputdialog() specifies the value to return
/// when the user cancels the prompt.
void get_user_input(const typval_st *const argvars,
                    typval_st *const rettv,
                    const bool inputdialog)
FUNC_ATTR_NONNULL_ALL
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    const char *prompt = "";
    const char *defstr = "";
    const char *cancelreturn = NULL;
    const char *xp_name = NULL;
    char prompt_buf[NUMBUFLEN];
    char defstr_buf[NUMBUFLEN];
    char cancelreturn_buf[NUMBUFLEN];
    char xp_name_buf[NUMBUFLEN];

    if(argvars[0].v_type == kNvarDict)
    {
        if(argvars[1].v_type != kNvarUnknown)
        {
            emsgf(_("E5050: {opts} must be the only argument"));
            return;
        }

        const dict_st *const dict = argvars[0].vval.v_dict;
        prompt = tv_dict_get_string_buf_chk(dict, S_LEN("prompt"), prompt_buf, "");

        if(prompt == NULL)
        {
            return;
        }

        defstr = tv_dict_get_string_buf_chk(dict, S_LEN("default"), defstr_buf, "");

        if(defstr == NULL)
        {
            return;
        }

        char def[1] = { 0 };
        cancelreturn = tv_dict_get_string_buf_chk(dict,
                                                  S_LEN("cancelreturn"),
                                                  cancelreturn_buf, def);

        if(cancelreturn == NULL) // error
        {
            return;
        }

        if(*cancelreturn == NUL)
        {
            cancelreturn = NULL;
        }

        xp_name = tv_dict_get_string_buf_chk(dict,
                                             S_LEN("completion"),
                                             xp_name_buf, def);

        if(xp_name == NULL) // error
        {
            return;
        }

        if(xp_name == def) // default to NULL
        {
            xp_name = NULL;
        }
    }
    else
    {
        prompt = tv_get_string_buf_chk(&argvars[0], prompt_buf);

        if(prompt == NULL)
        {
            return;
        }

        if(argvars[1].v_type != kNvarUnknown)
        {
            defstr = tv_get_string_buf_chk(&argvars[1], defstr_buf);

            if(defstr == NULL)
            {
                return;
            }

            if(argvars[2].v_type != kNvarUnknown)
            {
                const char *const arg2 =
                    tv_get_string_buf_chk(&argvars[2], cancelreturn_buf);

                if(arg2 == NULL)
                {
                    return;
                }

                if(inputdialog)
                {
                    cancelreturn = arg2;
                }
                else
                {
                    xp_name = arg2;
                }
            }
        }
    }

    int xp_type = EXPAND_NOTHING;
    char *xp_arg = NULL;

    if(xp_name != NULL)
    {
        // input() with a third argument: completion
        const int xp_namelen = (int)strlen(xp_name);
        uint32_t argt;

        if(parse_compl_arg((uchar_kt *)xp_name,
                           xp_namelen,
                           &xp_type,
                           &argt,
                           (uchar_kt **)&xp_arg) == FAIL)
        {
            return;
        }
    }

    int cmd_silent_save = cmd_silent;

    cmd_silent = false; // Want to see the prompt.

    // Only the part of the message after the last NL is considered as
    // prompt for the command line.
    const char *p = strrchr(prompt, '\n');

    if(p == NULL)
    {
        p = prompt;
    }
    else
    {
        p++;
        msg_start();
        msg_clr_eos();
        msg_puts_attr_len(prompt, p - prompt, echo_attr);
        msg_didout = false;
        msg_starthere();
    }

    cmdline_row = msg_row;

    stuffReadbuffSpec(defstr);

    int save_ex_normal_busy = ex_normal_busy;
    ex_normal_busy = 0;

    rettv->vval.v_string =
        getcmdline_prompt(inputsecret_flag ? NUL : '@',
                          (uchar_kt *)p,
                          echo_attr,
                          xp_type,
                          (uchar_kt *)xp_arg);

    ex_normal_busy = save_ex_normal_busy;

    if(rettv->vval.v_string == NULL && cancelreturn != NULL)
    {
        rettv->vval.v_string = (uchar_kt *)xstrdup(cancelreturn);
    }

    xfree(xp_arg);

    // Since the user typed this, no need to wait for return.
    need_wait_return = false;
    msg_didout = false;
    cmd_silent = cmd_silent_save;
}

/// "input()" function
///
/// Also handles inputsecret() when inputsecret is set.
static void f_input(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    get_user_input(argvars, rettv, FALSE);
}

/// "inputdialog()" function
static void f_inputdialog(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    get_user_input(argvars, rettv, TRUE);
}

/// "inputlist()" function
static void f_inputlist(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    listitem_st  *li;
    int selected;
    int mouse_used;

    if(argvars[0].v_type != kNvarList || argvars[0].vval.v_list == NULL)
    {
        EMSG2(_(e_listarg), "inputlist()");
        return;
    }

    msg_start();
    msg_row = Rows - 1; // for when 'cmdheight' > 1
    lines_left = Rows; // avoid more prompt
    msg_scroll = TRUE;
    msg_clr_eos();

    for(li = argvars[0].vval.v_list->lv_first; li != NULL; li = li->li_next)
    {
        msg_puts(tv_get_string(&li->li_tv));
        msg_putchar('\n');
    }

    // Ask for choice.
    selected = prompt_for_number(&mouse_used);

    if(mouse_used)
    {
        selected -= lines_left;
    }

    rettv->vval.v_number = selected;
}


static garray_st ga_userinput = {0, 0, sizeof(tahsave_st), 4, NULL};

/// "inputrestore()" function
static void f_inputrestore(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(!GA_EMPTY(&ga_userinput))
    {
        --ga_userinput.ga_len;
        restore_typeahead((tahsave_st *)(ga_userinput.ga_data) + ga_userinput.ga_len);
        // default return is zero == OK
    }
    else if(p_verbose > 1)
    {
        verb_msg((uchar_kt *)_("called inputrestore() more often than inputsave()"));
        rettv->vval.v_number = 1; // Failed
    }
}

/// "inputsave()" function
static void f_inputsave(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                        typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    // Add an entry to the stack of typeahead storage.
    tahsave_st *p = GA_APPEND_VIA_PTR(tahsave_st, &ga_userinput);
    save_typeahead(p);
}

/// "inputsecret()" function
static void f_inputsecret(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    cmdline_star++;
    inputsecret_flag++;
    f_input(argvars, rettv, NULL);
    cmdline_star--;
    inputsecret_flag--;
}

/// "insert()" function
static void f_insert(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    list_st *l;
    bool error = false;

    if(argvars[0].v_type != kNvarList)
    {
        EMSG2(_(e_listarg), "insert()");
    }
    else if((l = argvars[0].vval.v_list) != NULL
            && !tv_check_lock(l->lv_lock, N_("insert() argument"), TV_TRANSLATE))
    {
        long before = 0;

        if(argvars[2].v_type != kNvarUnknown)
        {
            before = tv_get_number_chk(&argvars[2], &error);
        }

        if(error)
        {
            // type error; errmsg already given
            return;
        }

        listitem_st *item = NULL;

        if(before != l->lv_len)
        {
            item = tv_list_find(l, before);

            if(item == NULL)
            {
                EMSGN(_(e_listidx), before);
                l = NULL;
            }
        }

        if(l != NULL)
        {
            tv_list_insert_tv(l, &argvars[1], item);
            tv_copy(&argvars[0], rettv);
        }
    }
}

/// "invert(expr)" function
static void f_invert(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = ~tv_get_number_chk(&argvars[0], NULL);
}

/// "isdirectory()" function
static void f_isdirectory(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = os_isdir((const uchar_kt *)tv_get_string(&argvars[0]));
}

/// "islocked()" function
static void f_islocked(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    leftval_st lv;
    dictitem_st  *di;
    rettv->vval.v_number = -1;

    const uchar_kt *const end = get_lval((uchar_kt *)tv_get_string(&argvars[0]),
                                       NULL,
                                       &lv, false, false,
                                       GLV_NO_AUTOLOAD|GLV_READ_ONLY,
                                       FNE_CHECK_START);

    if(end != NULL && lv.ll_name != NULL)
    {
        if(*end != NUL)
        {
            EMSG(_(e_trailing));
        }
        else
        {
            if(lv.ll_tv == NULL)
            {
                di = find_var((const char *)lv.ll_name, lv.ll_name_len, NULL, true);

                if(di != NULL)
                {
                    // Consider a variable locked when:
                    // 1. the variable itself is locked
                    // 2. the value of the variable is locked.
                    // 3. the List or Dict value is locked.
                    rettv->vval.v_number = ((di->di_flags & DI_FLAGS_LOCK)
                                            || tv_islocked(&di->di_tv));
                }
            }
            else if(lv.ll_range)
            {
                EMSG(_("E786: Range not allowed"));
            }
            else if(lv.ll_newkey != NULL)
            {
                EMSG2(_(e_dictkey), lv.ll_newkey);
            }
            else if(lv.ll_list != NULL)
            {
                // List item.
                rettv->vval.v_number = tv_islocked(&lv.ll_li->li_tv);
            }
            else
            {
                // Dictionary item.
                rettv->vval.v_number = tv_islocked(&lv.ll_di->di_tv);
            }
        }
    }

    clear_lval(&lv);
}


/// Turn a dictionary into a list
///
/// @param[in]  tv     Dictionary to convert. Is checked for actually being
///                    a dictionary, will give an error if not.
/// @param[out] rettv  Location where result will be saved.
/// @param[in]  what   What to save in rettv.
static void dict_list(typval_st *const tv,
                      typval_st *const rettv,
                      const dictlist_type_et what)
{
    if(tv->v_type != kNvarDict)
    {
        emsgf(_(e_dictreq));
        return;
    }

    if(tv->vval.v_dict == NULL)
    {
        return;
    }

    tv_list_alloc_ret(rettv);

    TV_DICT_ITER(tv->vval.v_dict, di, {
        listitem_st *const li = tv_list_item_alloc();
        tv_list_append(rettv->vval.v_list, li);

        switch(what)
        {
            case kDictListKeys:
            {
                li->li_tv.v_type = kNvarString;
                li->li_tv.v_lock = kNvlVarUnlocked;
                li->li_tv.vval.v_string = ustrdup(di->di_key);
                break;
            }

            case kDictListValues:
            {
                tv_copy(&di->di_tv, &li->li_tv);
                break;
            }

            case kDictListItems:
            {
                // items()
                list_st *const sub_l = tv_list_alloc();
                li->li_tv.v_type = kNvarList;
                li->li_tv.v_lock = kNvlVarUnlocked;
                li->li_tv.vval.v_list = sub_l;
                sub_l->lv_refcount++;

                listitem_st *sub_li = tv_list_item_alloc();
                tv_list_append(sub_l, sub_li);
                sub_li->li_tv.v_type = kNvarString;
                sub_li->li_tv.v_lock = kNvlVarUnlocked;
                sub_li->li_tv.vval.v_string = ustrdup(di->di_key);
                sub_li = tv_list_item_alloc();
                tv_list_append(sub_l, sub_li);
                tv_copy(&di->di_tv, &sub_li->li_tv);

                break;
            }
        }
    });
}

/// "id()" function
static void f_id(typval_st *argvars,
                 typval_st *rettv,
                 func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
FUNC_ATTR_NONNULL_ALL
{
    const int len = xvsnprintf(NULL, 0, "%p", dummy_ap, argvars);

    rettv->v_type = kNvarString;
    rettv->vval.v_string = xmalloc(len + 1);

    xvsnprintf((char *)rettv->vval.v_string,
               len + 1, "%p", dummy_ap, argvars);
}

/// "items(dict)" function
static void f_items(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    dict_list(argvars, rettv, 2);
}

/// "jobclose(id[, stream])" function
static void f_jobclose(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarNumber
       || (argvars[1].v_type != kNvarString && argvars[1].v_type != kNvarUnknown))
    {
        EMSG(_(e_invarg));
        return;
    }

    terminal_jobdata_st *data = find_job(argvars[0].vval.v_number);

    if(!data)
    {
        EMSG(_(e_invjob));
        return;
    }

    process_st *proc = (process_st *)&data->proc;

    if(argvars[1].v_type == kNvarString)
    {
        char *stream = (char *)argvars[1].vval.v_string;

        if(!strcmp(stream, "stdin"))
        {
            if(data->rpc)
            {
                EMSG(_("Invalid stream on rpc job, use jobclose(id, 'rpc')"));
            }
            else
            {
                process_close_in(proc);
            }
        }
        else if(!strcmp(stream, "stdout"))
        {
            if(data->rpc)
            {
                EMSG(_("Invalid stream on rpc job, use jobclose(id, 'rpc')"));
            }
            else
            {
                process_close_out(proc);
            }
        }
        else if(!strcmp(stream, "stderr"))
        {
            process_close_err(proc);
        }
        else if(!strcmp(stream, "rpc"))
        {
            if(data->rpc)
            {
                channel_close(data->id);
            }
            else
            {
                EMSG(_("Invalid job stream: Not an rpc job"));
            }
        }
        else
        {
            EMSG2(_("Invalid job stream \"%s\""), stream);
        }
    }
    else
    {
        if(data->rpc)
        {
            channel_close(data->id);
            process_close_err(proc);
        }
        else
        {
            process_close_streams(proc);

            if(proc->type == kProcessTypePty)
            {
                pty_process_close_master(&data->proc.pty);
            }
        }
    }
}

/// "jobpid(id)" function
static void f_jobpid(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarNumber)
    {
        EMSG(_(e_invarg));
        return;
    }

    terminal_jobdata_st *data = find_job(argvars[0].vval.v_number);

    if(!data)
    {
        EMSG(_(e_invjob));
        return;
    }

    process_st *proc = (process_st *)&data->proc;
    rettv->vval.v_number = proc->pid;
}

/// "jobsend()" function
static void f_jobsend(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarNumber || argvars[1].v_type == kNvarUnknown)
    {
        // First argument is the job id and
        // second is the string or list to write to the job's stdin
        EMSG(_(e_invarg));
        return;
    }

    terminal_jobdata_st *data = find_job(argvars[0].vval.v_number);

    if(!data)
    {
        EMSG(_(e_invjob));
        return;
    }

    if(((process_st *)&data->proc)->in->closed)
    {
        EMSG(_("Can't send data to the job: stdin is closed"));
        return;
    }

    if(data->rpc)
    {
        EMSG(_("Can't send raw data to rpc channel"));
        return;
    }

    ptrdiff_t input_len = 0;
    char *input = save_tv_as_string(&argvars[1], &input_len, false);

    if(!input)
    {
        // Either the error has been handled
        // by save_tv_as_string(), or there is no input to send.
        return;
    }

    wbuffer_st *buf = wstream_new_buffer(input, input_len, 1, xfree);
    rettv->vval.v_number = wstream_write(data->proc.uv.process.in, buf);
}

/// "jobresize(job, width, height)" function
static void f_jobresize(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarNumber
       || argvars[1].v_type != kNvarNumber
       || argvars[2].v_type != kNvarNumber)
    {
        // job id, width, height
        EMSG(_(e_invarg));
        return;
    }

    terminal_jobdata_st *data = find_job(argvars[0].vval.v_number);

    if(!data)
    {
        EMSG(_(e_invjob));
        return;
    }

    if(data->proc.uv.process.type != kProcessTypePty)
    {
        EMSG(_(e_jobnotpty));
        return;
    }

    pty_process_resize(&data->proc.pty,
                       argvars[1].vval.v_number,
                       argvars[2].vval.v_number);

    rettv->vval.v_number = 1;
}

static char **tv_to_argv(typval_st *cmd_tv, const char **cmd, bool *executable)
{
    if(cmd_tv->v_type == kNvarString)
    {
        const char *cmd_str = tv_get_string(cmd_tv);

        if(cmd)
        {
            *cmd = cmd_str;
        }

        return shell_build_argv(cmd_str, NULL);
    }

    if(cmd_tv->v_type != kNvarList)
    {
        EMSG2(_(e_invarg2), "expected String or List");
        return NULL;
    }

    list_st *argl = cmd_tv->vval.v_list;
    int argc = argl->lv_len;

    if(!argc)
    {
        EMSG(_(e_invarg)); // List must have at least one item.
        return NULL;
    }

    assert(argl->lv_first);
    const char *exe = tv_get_string_chk(&argl->lv_first->li_tv);

    if(!exe || !os_can_exe((const uchar_kt *)exe, NULL, true))
    {
        if(exe && executable)
        {
            *executable = false;
        }

        return NULL;
    }

    if(cmd)
    {
        *cmd = exe;
    }

    // Build the argument vector
    int i = 0;
    char **argv = xcalloc(argc + 1, sizeof(char *));

    for(listitem_st *arg = argl->lv_first; arg != NULL; arg = arg->li_next)
    {
        const char *a = tv_get_string_chk(&arg->li_tv);

        if(!a)
        {
            // Did emsg in tv_get_string_chk; just deallocate argv.
            shell_free_argv(argv);
            return NULL;
        }

        argv[i++] = xstrdup(a);
    }

    return argv;
}

/// "jobstart()" function
static void f_jobstart(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    bool executable = true;
    char **argv = tv_to_argv(&argvars[0], NULL, &executable);

    if(!argv)
    {
        rettv->vval.v_number = executable ? 0 : -1;
        return; // Did error message in tv_to_argv.
    }

    if(argvars[1].v_type != kNvarDict && argvars[1].v_type != kNvarUnknown)
    {
        // Wrong argument types
        EMSG2(_(e_invarg2), "expected dictionary");
        shell_free_argv(argv);
        return;
    }

    bool rpc = false;
    bool pty = false;
    char *cwd = NULL;
    bool detach = false;
    dict_st *job_opts = NULL;
    callback_st on_stdout = CALLBACK_NONE;
    callback_st on_stderr = CALLBACK_NONE;
    callback_st on_exit = CALLBACK_NONE;


    if(argvars[1].v_type == kNvarDict)
    {
        job_opts = argvars[1].vval.v_dict;
        detach = tv_dict_get_number(job_opts, "detach") != 0;
        rpc = tv_dict_get_number(job_opts, "rpc") != 0;
        pty = tv_dict_get_number(job_opts, "pty") != 0;

        if(pty && rpc)
        {
            EMSG2(_(e_invarg2), "job cannot have both 'pty' and 'rpc' options set");
            shell_free_argv(argv);
            return;
        }

        char *new_cwd = tv_dict_get_string(job_opts, "cwd", false);

        if(new_cwd && strlen(new_cwd) > 0)
        {
            cwd = new_cwd;

            // The new cwd must be a directory.
            if(!os_isdir((uchar_kt *)cwd))
            {
                EMSG2(_(e_invarg2), "expected valid directory");
                shell_free_argv(argv);
                return;
            }
        }

        if(!common_job_callbacks(job_opts, &on_stdout, &on_stderr, &on_exit))
        {
            shell_free_argv(argv);
            return;
        }
    }

    terminal_jobdata_st *data = common_job_init(argv,
                                            on_stdout,
                                            on_stderr,
                                            on_exit,
                                            pty,
                                            rpc,
                                            detach,
                                            cwd);

    process_st *proc = (process_st *)&data->proc;

    if(pty)
    {
        uint16_t width = (uint16_t)tv_dict_get_number(job_opts, "width");

        if(width > 0)
        {
            data->proc.pty.width = width;
        }

        uint16_t height = (uint16_t)tv_dict_get_number(job_opts, "height");

        if(height > 0)
        {
            data->proc.pty.height = height;
        }

        char *term = tv_dict_get_string(job_opts, "TERM", true);

        if(term)
        {
            data->proc.pty.term_name = term;
        }
    }

    if(!rpc && on_stdout.type == kCallbackNone)
    {
        proc->out = NULL;
    }

    if(on_stderr.type == kCallbackNone)
    {
        proc->err = NULL;
    }

    common_job_start(data, rettv);
}

/// "jobstop()" function
static void f_jobstop(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarNumber)
    {
        // Only argument is the job id
        EMSG(_(e_invarg));
        return;
    }

    terminal_jobdata_st *data = find_job(argvars[0].vval.v_number);

    if(!data)
    {
        EMSG(_(e_invjob));
        return;
    }

    process_stop((process_st *)&data->proc);
    data->stopped = true;
    rettv->vval.v_number = 1;
}

/// "jobwait(ids[, timeout])" function
static void f_jobwait(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarList
       || (argvars[1].v_type != kNvarNumber && argvars[1].v_type != kNvarUnknown))
    {
        EMSG(_(e_invarg));
        return;
    }

    list_st *args = argvars[0].vval.v_list;
    list_st *rv = tv_list_alloc();
    ui_busy_start();
    multiqueue_st *waiting_jobs = multiqueue_new_parent(loop_on_put, &main_loop);

    // For each item in the input list append an integer to the output list. -3
    // is used to represent an invalid job id, -2 is for a interrupted job and
    // -1 for jobs that were skipped or timed out.
    for(listitem_st *arg = args->lv_first; arg != NULL; arg = arg->li_next)
    {
        terminal_jobdata_st *data = NULL;

        if(arg->li_tv.v_type != kNvarNumber
           || !(data = find_job(arg->li_tv.vval.v_number)))
        {
            tv_list_append_number(rv, -3);
        }
        else
        {
            // append the list item and set the status pointer so
            // we'll collect the status code when the job exits
            tv_list_append_number(rv, -1);
            data->status_ptr = &rv->lv_last->li_tv.vval.v_number;

            // Process any pending events for the job because we'll
            // temporarily replace the parent queue
            multiqueue_process_events(data->events);
            multiqueue_replace_parent(data->events, waiting_jobs);
        }
    }

    int remaining = -1;
    uint64_t before = 0;

    if(argvars[1].v_type == kNvarNumber
       && argvars[1].vval.v_number >= 0)
    {
        remaining = argvars[1].vval.v_number;
        before = os_hrtime();
    }

    for(listitem_st *arg = args->lv_first; arg != NULL; arg = arg->li_next)
    {
        terminal_jobdata_st *data = NULL;

        if(remaining == 0)
        {
            // timed out
            break;
        }

        if(arg->li_tv.v_type != kNvarNumber
           || !(data = find_job(arg->li_tv.vval.v_number)))
        {
            continue;
        }

        int status = process_wait((process_st *)&data->proc,
                                  remaining,
                                  waiting_jobs);

        if(status < 0)
        {
            // interrupted or timed out, skip remaining jobs.
            if(status == -2)
            {
                // set the status so the user can distinguish
                // between interrupted and skipped/timeout jobs.
                *data->status_ptr = -2;
            }

            break;
        }

        if(remaining > 0)
        {
            uint64_t now = os_hrtime();
            remaining -= (int) ((now - before) / 1000000);
            before = now;

            if(remaining <= 0)
            {
                break;
            }
        }
    }

    for(listitem_st *arg = args->lv_first; arg != NULL; arg = arg->li_next)
    {
        terminal_jobdata_st *data = NULL;

        if(arg->li_tv.v_type != kNvarNumber
           || !(data = find_job(arg->li_tv.vval.v_number)))
        {
            continue;
        }

        // remove the status pointer because the list may be freed before the
        // job exits
        data->status_ptr = NULL;
    }

    // restore the parent queue for any jobs still alive
    for(listitem_st *arg = args->lv_first; arg != NULL; arg = arg->li_next)
    {
        terminal_jobdata_st *data = NULL;

        if(arg->li_tv.v_type != kNvarNumber
           || !(data = pmap_get(uint64_t)(jobs, arg->li_tv.vval.v_number)))
        {
            continue;
        }

        // restore the parent queue for the job
        multiqueue_process_events(data->events);
        multiqueue_replace_parent(data->events, main_loop.events);
    }

    multiqueue_free(waiting_jobs);
    ui_busy_stop();
    rv->lv_refcount++;
    rettv->v_type = kNvarList;
    rettv->vval.v_list = rv;
}

/// "join()" function
static void f_join(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[0].v_type != kNvarList)
    {
        EMSG(_(e_listreq));
        return;
    }

    if(argvars[0].vval.v_list == NULL)
    {
        return;
    }

    const char *const sep = (argvars[1].v_type == kNvarUnknown
                             ? " " : tv_get_string_chk(&argvars[1]));

    rettv->v_type = kNvarString;

    if(sep != NULL)
    {
        garray_st ga;

        ga_init(&ga, (int)sizeof(char), 80);
        tv_list_join(&ga, argvars[0].vval.v_list, sep);
        ga_append(&ga, NUL);
        rettv->vval.v_string = (uchar_kt *)ga.ga_data;
    }
    else
    {
        rettv->vval.v_string = NULL;
    }
}

/// json_decode() function
static void f_json_decode(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char numbuf[NUMBUFLEN];
    const char *s = NULL;
    char *tofree = NULL;
    size_t len;

    if(argvars[0].v_type == kNvarList)
    {
        if(!encode_vim_list_to_buf(argvars[0].vval.v_list, &len, &tofree))
        {
            EMSG(_("E474: Failed to convert list to string"));
            return;
        }

        s = tofree;

        if(s == NULL)
        {
            assert(len == 0);
            s = "";
        }
    }
    else
    {
        s = tv_get_string_buf_chk(&argvars[0], numbuf);

        if(s)
        {
            len = strlen(s);
        }
        else
        {
            return;
        }
    }

    if(json_decode_string(s, len, rettv) == FAIL)
    {
        emsgf(_("E474: Failed to parse %.*s"), (int) len, s);
        rettv->v_type = kNvarNumber;
        rettv->vval.v_number = 0;
    }

    assert(rettv->v_type != kNvarUnknown);
    xfree(tofree);
}

/// json_encode() function
static void f_json_encode(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = (uchar_kt *) encode_tv2json(&argvars[0], NULL);
}

/// "keys()" function
static void f_keys(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    dict_list(argvars, rettv, 0);
}

/// "last_buffer_nr()" function.
static void f_last_buffer_nr(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                             typval_st *rettv,
                             func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int n = 0;

    FOR_ALL_BUFFERS(buf)
    {
        if(n < buf->b_id)
        {
            n = buf->b_id;
        }
    }
    rettv->vval.v_number = n;
}

/// "len()" function
static void f_len(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    switch (argvars[0].v_type)
    {
        case kNvarString:
        case kNvarNumber:
        {
            rettv->vval.v_number =
                (number_kt)strlen(tv_get_string(&argvars[0]));
            break;
        }

        case kNvarList:
        {
            rettv->vval.v_number = tv_list_len(argvars[0].vval.v_list);
            break;
        }

        case kNvarDict:
        {
            rettv->vval.v_number = tv_dict_len(argvars[0].vval.v_dict);
            break;
        }

        case kNvarUnknown:
        case kNvarSpecial:
        case kNvarFloat:
        case kNvarPartial:
        case kNvarUfunc:
        {
            EMSG(_("E701: Invalid type for len()"));
            break;
        }
    }
}

static void libcall_common(typval_st *argvars, typval_st *rettv, int out_type)
{
    rettv->v_type = out_type;

    if(out_type != kNvarNumber)
    {
        rettv->vval.v_string = NULL;
    }

    if(check_restricted() || check_secure())
    {
        return;
    }

    // The first two args (libname and funcname) must be strings
    if(argvars[0].v_type != kNvarString || argvars[1].v_type != kNvarString)
    {
        return;
    }

    const char *libname = (char *) argvars[0].vval.v_string;
    const char *funcname = (char *) argvars[1].vval.v_string;
    int in_type = argvars[2].v_type;

    // input variables
    char *str_in = (in_type == kNvarString)
                   ? (char *) argvars[2].vval.v_string : NULL;

    int64_t int_in = argvars[2].vval.v_number;

    // output variables
    char **str_out = (out_type == kNvarString)
                     ? (char **) &rettv->vval.v_string : NULL;

    int64_t int_out = 0;

    bool success = os_libcall(libname,
                              funcname,
                              str_in,
                              int_in,
                              str_out,
                              &int_out);

    if(!success)
    {
        EMSG2(_(e_libcall), funcname);
        return;
    }

    if(out_type == kNvarNumber)
    {
        rettv->vval.v_number = (int) int_out;
    }
}

/// "libcall()" function
static void f_libcall(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    libcall_common(argvars, rettv, kNvarString);
}

/// "libcallnr()" function
static void f_libcallnr(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    libcall_common(argvars, rettv, kNvarNumber);
}

/// "line(string)" function
static void f_line(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    linenum_kt lnum = 0;
    apos_st *fp;
    int fnum;
    fp = var2fpos(&argvars[0], TRUE, &fnum);

    if(fp != NULL)
    {
        lnum = fp->lnum;
    }

    rettv->vval.v_number = lnum;
}

/// "line2byte(lnum)" function
static void f_line2byte(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const linenum_kt lnum = tv_get_lnum(argvars);

    if(lnum < 1 || lnum > curbuf->b_ml.ml_line_count + 1)
    {
        rettv->vval.v_number = -1;
    }
    else
    {
        rettv->vval.v_number = ml_find_line_or_offset(curbuf, lnum, NULL);
    }

    if(rettv->vval.v_number >= 0)
    {
        rettv->vval.v_number++;
    }
}

/// "lispindent(lnum)" function
static void f_lispindent(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const apos_st pos = curwin->w_cursor;
    const linenum_kt lnum = tv_get_lnum(argvars);

    if(lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
        curwin->w_cursor.lnum = lnum;
        rettv->vval.v_number = get_lisp_indent();
        curwin->w_cursor = pos;
    }
    else
    {
        rettv->vval.v_number = -1;
    }
}

/// "localtime()" function
static void f_localtime(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = (number_kt)time(NULL);
}


static void get_maparg(typval_st *argvars, typval_st *rettv, int exact)
{
    uchar_kt *keys_buf = NULL;
    uchar_kt *rhs;
    int mode;
    int abbr = FALSE;
    int get_dict = FALSE;
    map_abbr_st  *mp;
    int buffer_local;

    // Return empty string for failure.
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;
    uchar_kt *keys = (uchar_kt *)tv_get_string(&argvars[0]);

    if(*keys == NUL)
    {
        return;
    }

    char buf[NUMBUFLEN];
    const char *which;

    if(argvars[1].v_type != kNvarUnknown)
    {
        which = tv_get_string_buf_chk(&argvars[1], buf);

        if(argvars[2].v_type != kNvarUnknown)
        {
            abbr = tv_get_number(&argvars[2]);

            if(argvars[3].v_type != kNvarUnknown)
            {
                get_dict = tv_get_number(&argvars[3]);
            }
        }
    }
    else
    {
        which = "";
    }

    if(which == NULL)
    {
        return;
    }

    mode = get_map_mode((uchar_kt **)&which, 0);

    keys = replace_termcodes(keys, ustrlen(keys), &keys_buf,
                             true, true, false, CPO_TO_CPO_FLAGS);

    rhs = check_map(keys, mode, exact, false, abbr, &mp, &buffer_local);
    xfree(keys_buf);

    if(!get_dict)
    {
        // Return a string.
        if(rhs != NULL)
        {
            rettv->vval.v_string = str2special_save(rhs, FALSE);
        }
    }
    else
    {
        tv_dict_alloc_ret(rettv);

        if(rhs != NULL)
        {
            // Return a dictionary.
            mapblock_fill_dict(rettv->vval.v_dict, mp, buffer_local, true);
        }
    }
}

/// luaeval() function implementation
static void f_luaeval(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
FUNC_ATTR_NONNULL_ALL
{
    const char *const str = (const char *)tv_get_string_chk(&argvars[0]);

    if(str == NULL)
    {
        return;
    }

    executor_eval_lua(cstr_as_string((char *)str), &argvars[1], rettv);
}

/// Fill a dictionary with all applicable maparg() like dictionaries
///
/// @param  dict          The dictionary to be filled
/// @param  mp            The maphash that contains the mapping information
/// @param  buffer_value  The "buffer" value
/// @param  compatible    True for compatible with old maparg() dict
void mapblock_fill_dict(dict_st *const dict,
                        const map_abbr_st *const mp,
                        long buffer_value,
                        bool compatible)
FUNC_ATTR_NONNULL_ALL
{
    uchar_kt *lhs = str2special_save(mp->m_keys, true);
    char *const mapmode = map_mode_to_chars(mp->m_mode);
    number_kt noremap_value;

    if(compatible)
    {
        // Keep old compatible behavior
        // This is unable to determine whether a mapping is a <script> mapping
        noremap_value = !!mp->m_noremap;
    }
    else
    {
        // Distinguish between <script> mapping
        // If it's not a <script> mapping, check if it's a noremap
        noremap_value = mp->m_noremap == REMAP_SCRIPT ? 2 : !!mp->m_noremap;
    }

    tv_dict_add_str(dict, S_LEN("lhs"),     (const char *)lhs);
    tv_dict_add_str(dict, S_LEN("rhs"),     (const char *)mp->m_orig_str);
    tv_dict_add_nr(dict,  S_LEN("noremap"), noremap_value);
    tv_dict_add_nr(dict,  S_LEN("expr"),    mp->m_expr ? 1 : 0);
    tv_dict_add_nr(dict,  S_LEN("silent"),  mp->m_silent ? 1 : 0);
    tv_dict_add_nr(dict,  S_LEN("sid"),     (number_kt)mp->m_script_ID);
    tv_dict_add_nr(dict,  S_LEN("buffer"),  (number_kt)buffer_value);
    tv_dict_add_nr(dict,  S_LEN("nowait"),  mp->m_nowait ? 1 : 0);
    tv_dict_add_str(dict, S_LEN("mode"),    mapmode);

    xfree(lhs);
    xfree(mapmode);
}

/// "map()" function
static void f_map(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    filter_map(argvars, rettv, TRUE);
}

/// "maparg()" function
static void f_maparg(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    get_maparg(argvars, rettv, TRUE);
}

/// "mapcheck()" function
static void f_mapcheck(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    get_maparg(argvars, rettv, FALSE);
}


static void find_some_match(typval_st *argvars, typval_st *rettv, int type)
{
    uchar_kt *str = NULL;
    long len = 0;
    uchar_kt *expr = NULL;
    regmatch_st regmatch;
    uchar_kt *save_cpo;
    long start = 0;
    long nth = 1;
    columnum_kt startcol = 0;
    int match = 0;
    list_st *l = NULL;
    listitem_st *li = NULL;
    long idx = 0;
    uchar_kt *tofree = NULL;

    // Make 'cpoptions' empty, the 'l' flag should not be used here.
    save_cpo = p_cpo;
    p_cpo = (uchar_kt *)"";
    rettv->vval.v_number = -1;

    if(type == 3 || type == 4)
    {
        // type 3: return empty list when there are no matches.
        // type 4: return ["", -1, -1, -1]
        tv_list_alloc_ret(rettv);

        if(type == 4)
        {
            tv_list_append_string(rettv->vval.v_list, "", 0);
            tv_list_append_number(rettv->vval.v_list, -1);
            tv_list_append_number(rettv->vval.v_list, -1);
            tv_list_append_number(rettv->vval.v_list, -1);
        }
    }
    else if(type == 2)
    {
        rettv->v_type = kNvarString;
        rettv->vval.v_string = NULL;
    }

    if(argvars[0].v_type == kNvarList)
    {
        if((l = argvars[0].vval.v_list) == NULL)
        {
            goto theend;
        }

        li = l->lv_first;
    }
    else
    {
        expr = str = (uchar_kt *)tv_get_string(&argvars[0]);
        len = (long)ustrlen(str);
    }

    char patbuf[NUMBUFLEN];
    const char *const pat = tv_get_string_buf_chk(&argvars[1], patbuf);

    if(pat == NULL)
    {
        goto theend;
    }

    if(argvars[2].v_type != kNvarUnknown)
    {
        bool error = false;
        start = tv_get_number_chk(&argvars[2], &error);

        if(error)
        {
            goto theend;
        }

        if(l != NULL)
        {
            li = tv_list_find(l, start);

            if(li == NULL)
            {
                goto theend;
            }

            idx = l->lv_idx; // Use the cached index.
        }
        else
        {
            if(start < 0)
            {
                start = 0;
            }

            if(start > len)
            {
                goto theend;
            }

            // When "count" argument is there ignore matches before "start",
            // otherwise skip part of the string. Differs when pattern is "^"
            // or "\<".
            if(argvars[3].v_type != kNvarUnknown)
            {
                startcol = start;
            }
            else
            {
                str += start;
                len -= start;
            }
        }

        if(argvars[3].v_type != kNvarUnknown)
        {
            nth = tv_get_number_chk(&argvars[3], &error);
        }

        if(error)
        {
            goto theend;
        }
    }

    regmatch.regprog = regexp_compile((uchar_kt *)pat, RE_MAGIC + RE_STRING);

    if(regmatch.regprog != NULL)
    {
        regmatch.rm_ic = p_ic;

        for(;;)
        {
            if(l != NULL)
            {
                if(li == NULL)
                {
                    match = FALSE;
                    break;
                }

                xfree(tofree);
                tofree = expr = str = (uchar_kt *)encode_tv2echo(&li->li_tv, NULL);

                if(str == NULL)
                {
                    break;
                }
            }

            match = vim_regexec_nl(&regmatch, str, (columnum_kt)startcol);

            if(match && --nth <= 0)
            {
                break;
            }

            if(l == NULL && !match)
            {
                break;
            }

            // Advance to just after the match.
            if(l != NULL)
            {
                li = li->li_next;
                ++idx;
            }
            else
            {
                startcol = (columnum_kt)(regmatch.startp[0]
                                     + (*mb_ptr2len)(regmatch.startp[0]) - str);

                if(startcol > (columnum_kt)len || str + startcol <= regmatch.startp[0])
                {
                    match = FALSE;
                    break;
                }
            }
        }

        if(match)
        {
            if(type == 4)
            {
                listitem_st *li1 = rettv->vval.v_list->lv_first;
                listitem_st *li2 = li1->li_next;
                listitem_st *li3 = li2->li_next;
                listitem_st *li4 = li3->li_next;

                xfree(li1->li_tv.vval.v_string);

                int rd = (int)(regmatch.endp[0] - regmatch.startp[0]);

                li1->li_tv.vval.v_string = ustrndup(regmatch.startp[0], rd);
                li3->li_tv.vval.v_number = (number_kt)(regmatch.startp[0] - expr);
                li4->li_tv.vval.v_number = (number_kt)(regmatch.endp[0] - expr);

                if(l != NULL)
                {
                    li2->li_tv.vval.v_number = (number_kt)idx;
                }
            }
            else if(type == 3)
            {
                // return list with matched string and submatches
                for(int i = 0; i < NSUBEXP; ++i)
                {
                    if(regmatch.endp[i] == NULL)
                    {
                        tv_list_append_string(rettv->vval.v_list, NULL, 0);
                    }
                    else
                    {
                        tv_list_append_string(rettv->vval.v_list,
                                              (const char *)regmatch.startp[i],
                                              (regmatch.endp[i] - regmatch.startp[i]));
                    }
                }
            }
            else if(type == 2)
            {
                // Return matched string.
                if(l != NULL)
                {
                    tv_copy(&li->li_tv, rettv);
                }
                else
                {
                    rettv->vval.v_string =
                        (uchar_kt *)xmemdupz((const char *)regmatch.startp[0],
                                           (size_t)(regmatch.endp[0] - regmatch.startp[0]));
                }
            }
            else if(l != NULL)
            {
                rettv->vval.v_number = idx;
            }
            else
            {
                if(type != 0)
                {
                    rettv->vval.v_number = (number_kt)(regmatch.startp[0] - str);
                }
                else
                {
                    rettv->vval.v_number = (number_kt)(regmatch.endp[0] - str);
                }

                rettv->vval.v_number += (number_kt)(str - expr);
            }
        }

        vim_regfree(regmatch.regprog);
    }

    if(type == 4 && l == NULL)
    {
        // matchstrpos() without a list: drop the second item
        tv_list_item_remove(rettv->vval.v_list,
                            rettv->vval.v_list->lv_first->li_next);
    }

theend:

    xfree(tofree);
    p_cpo = save_cpo;
}

/// "match()" function
static void f_match(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    find_some_match(argvars, rettv, 1);
}

/// "matchadd()" function
static void f_matchadd(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char grpbuf[NUMBUFLEN];
    char patbuf[NUMBUFLEN];
    const char *const grp = tv_get_string_buf_chk(&argvars[0], grpbuf);
    const char *const pat = tv_get_string_buf_chk(&argvars[1], patbuf);
    int prio = 10;
    int id = -1;
    bool error = false;
    const char *conceal_char = NULL;
    rettv->vval.v_number = -1;

    if(grp == NULL || pat == NULL)
    {
        return;
    }

    if(argvars[2].v_type != kNvarUnknown)
    {
        prio = tv_get_number_chk(&argvars[2], &error);

        if(argvars[3].v_type != kNvarUnknown)
        {
            id = tv_get_number_chk(&argvars[3], &error);

            if(argvars[4].v_type != kNvarUnknown)
            {
                if(argvars[4].v_type != kNvarDict)
                {
                    EMSG(_(e_dictreq));
                    return;
                }

                dictitem_st *di =
                    tv_dict_find(argvars[4].vval.v_dict, S_LEN("conceal"));

                if(di != NULL)
                {
                    conceal_char = tv_get_string(&di->di_tv);
                }
            }
        }
    }

    if(error)
    {
        return;
    }

    if(id >= 1 && id <= 3)
    {
        EMSGN("E798: ID is reserved for \":match\": %" PRId64, id);
        return;
    }

    rettv->vval.v_number =
        match_add(curwin, grp, pat, prio, id, NULL, conceal_char);
}

static void f_matchaddpos(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = -1;
    char buf[NUMBUFLEN];
    const char *const group = tv_get_string_buf_chk(&argvars[0], buf);

    if(group == NULL)
    {
        return;
    }

    if(argvars[1].v_type != kNvarList)
    {
        EMSG2(_(e_listarg), "matchaddpos()");
        return;
    }

    list_st *l;
    l = argvars[1].vval.v_list;

    if(l == NULL)
    {
        return;
    }

    bool error = false;
    int prio = 10;
    int id = -1;
    const char *conceal_char = NULL;

    if(argvars[2].v_type != kNvarUnknown)
    {
        prio = tv_get_number_chk(&argvars[2], &error);

        if(argvars[3].v_type != kNvarUnknown)
        {
            id = tv_get_number_chk(&argvars[3], &error);

            if(argvars[4].v_type != kNvarUnknown)
            {
                if(argvars[4].v_type != kNvarDict)
                {
                    EMSG(_(e_dictreq));
                    return;
                }

                dictitem_st *di =
                    tv_dict_find(argvars[4].vval.v_dict, S_LEN("conceal"));

                if(di != NULL)
                {
                    conceal_char = tv_get_string(&di->di_tv);
                }
            }
        }
    }

    if(error == true)
    {
        return;
    }

    // id == 3 is ok because matchaddpos() is supposed to substitute :3match
    if(id == 1 || id == 2)
    {
        EMSGN("E798: ID is reserved for \"match\": %" PRId64, id);
        return;
    }

    rettv->vval.v_number = match_add(curwin, group, NULL, prio, id, l, conceal_char);
}

/// nvimL: matcharg() function
static void f_matcharg(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    tv_list_alloc_ret(rettv);
    int id = tv_get_number(&argvars[0]);

    if(id >= 1 && id <= 3)
    {
        matchitem_st *m;

        if((m = (matchitem_st *)get_match(curwin, id)) != NULL)
        {
            tv_list_append_string(rettv->vval.v_list,
                                  (const char *)syn_id2name(m->hlg_id), -1);

            tv_list_append_string(rettv->vval.v_list,
                                  (const char *)m->pattern, -1);
        }
        else
        {
            tv_list_append_string(rettv->vval.v_list, NULL, 0);
            tv_list_append_string(rettv->vval.v_list, NULL, 0);
        }
    }
}

/// "matchdelete()" function
static void f_matchdelete(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number =
        match_delete(curwin, (int)tv_get_number(&argvars[0]), true);
}

/// "matchend()" function
static void f_matchend(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    find_some_match(argvars, rettv, 0);
}

/// "matchlist()" function
static void f_matchlist(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    find_some_match(argvars, rettv, 3);
}

/// "matchstr()" function
static void f_matchstr(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    find_some_match(argvars, rettv, 2);
}

/// "matchstrpos()" function
static void f_matchstrpos(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    find_some_match(argvars, rettv, 4);
}

/// Get maximal/minimal number value in a list or dictionary
///
/// @param[in]  tv
/// List or dictionary to work with. If it contains something
/// that is not an integer number (or cannot be coerced to
/// it) error is given.
///
/// @param[out] rettv
/// Location where result will be saved. Only assigns
/// vval.v_number, type is not touched. Returns zero for
/// empty lists/dictionaries.
///
/// @param[in] domax
/// Determines whether maximal or minimal value is desired.
static void max_min(const typval_st *const tv,
                    typval_st *const rettv,
                    const bool domax)
FUNC_ATTR_NONNULL_ALL
{
    number_kt n = 0;
    bool error = false;

    if(tv->v_type == kNvarList)
    {
        const list_st *const l = tv->vval.v_list;

        if(tv_list_len(l) != 0)
        {
            n = tv_get_number_chk(&l->lv_first->li_tv, &error);

            for(const listitem_st *li = l->lv_first->li_next;
                li != NULL && !error;
                li = li->li_next)
            {
                const number_kt i = tv_get_number_chk(&li->li_tv, &error);

                if(domax ? i > n : i < n)
                {
                    n = i;
                }
            }
        }
    }
    else if(tv->v_type == kNvarDict)
    {
        if(tv->vval.v_dict != NULL)
        {
            bool first = true;

            TV_DICT_ITER(tv->vval.v_dict, di, {
                const number_kt i = tv_get_number_chk(&di->di_tv, &error);

                if(error)
                {
                    break;
                }
                if(first)
                {
                    n = i;
                    first = true;
                }
                else if(domax ? i > n : i < n)
                {
                    n = i;
                }
            });
        }
    }
    else
    {
        EMSG2(_(e_listdictarg), domax ? "max()" : "min()");
    }

    rettv->vval.v_number = error ? 0 : n;
}

/// "max()" function
static void f_max(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    max_min(argvars, rettv, TRUE);
}

/// "min()" function
static void f_min(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    max_min(argvars, rettv, FALSE);
}

/// "mkdir()" function
static void f_mkdir(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int prot = 0755;
    rettv->vval.v_number = FAIL;

    if(check_restricted() || check_secure())
    {
        return;
    }

    char buf[NUMBUFLEN];
    const char *const dir = tv_get_string_buf(&argvars[0], buf);

    if(*dir == NUL)
    {
        rettv->vval.v_number = FAIL;
    }
    else
    {
        if(*path_tail((uchar_kt *)dir) == NUL)
        {
            // Remove trailing slashes.
            *path_tail_with_sep((uchar_kt *)dir) = NUL;
        }

        if(argvars[1].v_type != kNvarUnknown)
        {
            if(argvars[2].v_type != kNvarUnknown)
            {
                prot = tv_get_number_chk(&argvars[2], NULL);
            }

            if(prot != -1 && strcmp(tv_get_string(&argvars[1]), "p") == 0)
            {
                char *failed_dir;
                int ret = os_mkdir_recurse(dir, prot, &failed_dir);

                if(ret != 0)
                {
                    EMSG3(_(e_mkdir), failed_dir, os_strerror(ret));
                    xfree(failed_dir);
                    rettv->vval.v_number = FAIL;
                    return;
                }
                else
                {
                    rettv->vval.v_number = OK;
                    return;
                }
            }
        }

        rettv->vval.v_number = prot == -1 ? FAIL : vim_mkdir_emsg(dir, prot);
    }
}

/// "mode()" function
static void f_mode(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char *mode = get_mode();

    // Clear out the minor mode when the argument
    // is not a non-zero number or non-empty string.
    if(!non_zero_arg(&argvars[0]))
    {
        mode[1] = NUL;
    }

    rettv->vval.v_string = (uchar_kt *)mode;
    rettv->v_type = kNvarString;
}

/// "msgpackdump()" function
static void f_msgpackdump(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
FUNC_ATTR_NONNULL_ALL
{
    if(argvars[0].v_type != kNvarList)
    {
        EMSG2(_(e_listarg), "msgpackdump()");
        return;
    }

    list_st *ret_list = tv_list_alloc_ret(rettv);
    const list_st *list = argvars[0].vval.v_list;

    if(list == NULL)
    {
        return;
    }

    msgpack_packer *lpacker = msgpack_packer_new(ret_list, &encode_list_write);
    const char *const msg = _("msgpackdump() argument, index %i");

    // Assume that translation will not take more then 4 times more space
    char msgbuf[sizeof("msgpackdump() argument, index ") * 4 + NUMBUFLEN];
    int idx = 0;

    for(listitem_st *li = list->lv_first; li != NULL; li = li->li_next)
    {
        xsnprintf(msgbuf, sizeof(msgbuf), (char *) msg, idx);
        idx++;

        if(encode_vim_to_msgpack(lpacker, &li->li_tv, msgbuf) == FAIL)
        {
            break;
        }
    }

    msgpack_packer_free(lpacker);
}

/// "msgpackparse" function
static void f_msgpackparse(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
FUNC_ATTR_NONNULL_ALL
{
    if(argvars[0].v_type != kNvarList)
    {
        EMSG2(_(e_listarg), "msgpackparse()");
        return;
    }

    list_st *ret_list = tv_list_alloc_ret(rettv);
    const list_st *list = argvars[0].vval.v_list;

    if(list == NULL || list->lv_first == NULL)
    {
        return;
    }

    if(list->lv_first->li_tv.v_type != kNvarString)
    {
        EMSG2(_(e_invarg2), "List item is not a string");
        return;
    }

    list_state_st lrstate = encode_init_lrstate(list);
    msgpack_unpacker *const unpacker = msgpack_unpacker_new(IOSIZE);

    if(unpacker == NULL)
    {
        EMSG(_(e_outofmem));
        return;
    }

    msgpack_unpacked unpacked;
    msgpack_unpacked_init(&unpacked);

    do
    {
        if(!msgpack_unpacker_reserve_buffer(unpacker, IOSIZE))
        {
            EMSG(_(e_outofmem));
            goto f_msgpackparse_exit;
        }

        size_t read_bytes;

        const int rlret =
            encode_read_from_list(&lrstate,
                                  msgpack_unpacker_buffer(unpacker),
                                  IOSIZE,
                                  &read_bytes);

        if(rlret == FAIL)
        {
            EMSG2(_(e_invarg2), "List item is not a string");
            goto f_msgpackparse_exit;
        }

        msgpack_unpacker_buffer_consumed(unpacker, read_bytes);

        if(read_bytes == 0)
        {
            break;
        }

        while(unpacker->off < unpacker->used)
        {
            const msgpack_unpack_return result =
                msgpack_unpacker_next(unpacker, &unpacked);

            if(result == MSGPACK_UNPACK_PARSE_ERROR)
            {
                EMSG2(_(e_invarg2), "Failed to parse msgpack string");
                goto f_msgpackparse_exit;
            }

            if(result == MSGPACK_UNPACK_NOMEM_ERROR)
            {
                EMSG(_(e_outofmem));
                goto f_msgpackparse_exit;
            }

            if(result == MSGPACK_UNPACK_SUCCESS)
            {
                listitem_st *li = tv_list_item_alloc();
                li->li_tv.v_type = kNvarUnknown;
                tv_list_append(ret_list, li);

                if(msgpack_to_vim(unpacked.data, &li->li_tv) == FAIL)
                {
                    EMSG2(_(e_invarg2), "Failed to convert msgpack string");
                    goto f_msgpackparse_exit;
                }
            }

            if(result == MSGPACK_UNPACK_CONTINUE)
            {
                if(rlret == OK)
                {
                    EMSG2(_(e_invarg2), "Incomplete msgpack string");
                }

                break;
            }
        }

        if(rlret == OK)
        {
            break;
        }
    } while(true);

f_msgpackparse_exit:

    msgpack_unpacked_destroy(&unpacked);
    msgpack_unpacker_free(unpacker);

    return;
}

/// "nextnonblank()" function
static void f_nextnonblank(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    linenum_kt lnum;

    for(lnum = tv_get_lnum(argvars);; lnum++)
    {
        if(lnum < 0 || lnum > curbuf->b_ml.ml_line_count)
        {
            lnum = 0;
            break;
        }

        if(*skipwhite(ml_get(lnum)) != NUL)
        {
            break;
        }
    }

    rettv->vval.v_number = lnum;
}

/// "nr2char()" function
static void f_nr2char(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[1].v_type != kNvarUnknown)
    {
        if(!tv_check_num(&argvars[1]))
        {
            return;
        }
    }

    bool error = false;
    const number_kt num = tv_get_number_chk(&argvars[0], &error);

    if(error)
    {
        return;
    }

    if(num < 0)
    {
        emsgf(_("E5070: Character number must not be less than zero"));
        return;
    }

    if(num > INT_MAX)
    {
        emsgf(_("E5071: Character number must not be "
                "greater than INT_MAX (%i)"), INT_MAX);
        return;
    }

    char buf[MB_MAXBYTES];
    const int len = utf_char2bytes((int)num, (uchar_kt *)buf);

    rettv->v_type = kNvarString;
    rettv->vval.v_string = xmemdupz(buf, (size_t)len);
}

/// "or(expr, expr)" function
static void f_or(typval_st *argvars,
                 typval_st *rettv,
                 func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = tv_get_number_chk(&argvars[0], NULL)
                           | tv_get_number_chk(&argvars[1], NULL);
}

/// "pathshorten()" function
static void f_pathshorten(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    const char *const s = tv_get_string_chk(&argvars[0]);

    if(!s)
    {
        return;
    }

    rettv->vval.v_string = shorten_dir((uchar_kt *)xstrdup(s));
}

/// "pow()" function
static void f_pow(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    float_kt fx;
    float_kt fy;
    rettv->v_type = kNvarFloat;

    if(tv_get_float_chk(argvars, &fx) && tv_get_float_chk(&argvars[1], &fy))
    {
        rettv->vval.v_float = pow(fx, fy);
    }
    else
    {
        rettv->vval.v_float = 0.0;
    }
}

/// "prevnonblank()" function
static void f_prevnonblank(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    linenum_kt lnum = tv_get_lnum(argvars);

    if(lnum < 1 || lnum > curbuf->b_ml.ml_line_count)
    {
        lnum = 0;
    }
    else
    {
        while(lnum >= 1 && *skipwhite(ml_get(lnum)) == NUL)
        {
            lnum--;
        }
    }

    rettv->vval.v_number = lnum;
}

/// "printf()" function
static void f_printf(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    {
        int len;
        int saved_did_emsg = did_emsg;

        // Get the required length, allocate the buffer and do it for real.
        did_emsg = false;
        char buf[NUMBUFLEN];
        const char *fmt = tv_get_string_buf(&argvars[0], buf);
        len = xvsnprintf(NULL, 0, fmt, dummy_ap, argvars + 1);

        if(!did_emsg)
        {
            char *s = xmalloc(len + 1);
            rettv->vval.v_string = (uchar_kt *)s;
            (void)xvsnprintf(s, len + 1, fmt, dummy_ap, argvars + 1);
        }

        did_emsg |= saved_did_emsg;
    }
}

/// "pumvisible()" function
static void f_pumvisible(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(pum_visible())
    {
        rettv->vval.v_number = 1;
    }
}

/// "pyeval()" function
static void f_pyeval(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    script_host_eval("python", argvars, rettv);
}

/// "py3eval()" function
static void f_py3eval(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    script_host_eval("python3", argvars, rettv);
}

/// "range()" function
static void f_range(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    number_kt start;
    number_kt end;
    number_kt stride = 1;
    long i;
    bool error = false;
    start = tv_get_number_chk(&argvars[0], &error);

    if(argvars[1].v_type == kNvarUnknown)
    {
        end = start - 1;
        start = 0;
    }
    else
    {
        end = tv_get_number_chk(&argvars[1], &error);

        if(argvars[2].v_type != kNvarUnknown)
        {
            stride = tv_get_number_chk(&argvars[2], &error);
        }
    }

    if(error)
    {
        return; // Type error; errmsg already given.
    }

    if(stride == 0)
    {
        emsgf(_("E726: Stride is zero"));
    }
    else if(stride > 0 ? end + 1 < start : end - 1 > start)
    {
        emsgf(_("E727: Start past end"));
    }
    else
    {
        tv_list_alloc_ret(rettv);

        for(i = start; stride > 0 ? i <= end : i >= end; i += stride)
        {
            tv_list_append_number(rettv->vval.v_list, (number_kt)i);
        }
    }
}

/// "readfile()" function
static void f_readfile(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    bool binary = false;
    FILE *fd;
    uchar_kt buf[(IOSIZE/256)*256]; // rounded to avoid odd + 1
    int io_size = sizeof(buf);
    int readlen; // size of last fread()
    uchar_kt *prev = NULL; // previously read bytes, if any
    long prevlen  = 0; // length of data in prev
    long prevsize = 0; // size of prev buffer
    long maxline  = MAXLNUM;
    long cnt = 0;
    uchar_kt *p; // position in buf
    uchar_kt *start; // start of current line

    if(argvars[1].v_type != kNvarUnknown)
    {
        if(strcmp(tv_get_string(&argvars[1]), "b") == 0)
        {
            binary = true;
        }

        if(argvars[2].v_type != kNvarUnknown)
        {
            maxline = tv_get_number(&argvars[2]);
        }
    }

    tv_list_alloc_ret(rettv);

    // Always open the file in binary mode, library functions
    // have a mind of their own about CR-LF conversion.
    const char *const fname = tv_get_string(&argvars[0]);

    if(*fname == NUL || (fd = mch_fopen(fname, READBIN)) == NULL)
    {
        EMSG2(_(e_notopen), *fname == NUL ? _("<empty>") : fname);
        return;
    }

    while(cnt < maxline || maxline < 0)
    {
        readlen = (int)fread(buf, 1, io_size, fd);

        // This for loop processes what was read, but is also entered at end
        // of file so that either:
        // - an incomplete line gets written
        // - a "binary" file gets an empty line at the end if it ends in a newline.
        for(p = buf, start = buf;
            p < buf + readlen || (readlen <= 0 && (prevlen > 0 || binary));
            ++p)
        {
            if(*p == '\n' || readlen <= 0)
            {
                listitem_st *li;
                uchar_kt *s  = NULL;
                size_t len = p - start;

                // Finished a line.  Remove CRs before NL.
                if(readlen > 0 && !binary)
                {
                    while(len > 0 && start[len - 1] == '\r')
                    {
                        --len;
                    }

                    // removal may cross back to the "prev" string
                    if(len == 0)
                    {
                        while(prevlen > 0 && prev[prevlen - 1] == '\r')
                        {
                            --prevlen;
                        }
                    }
                }

                if(prevlen == 0)
                {
                    assert(len < INT_MAX);
                    s = ustrndup(start, (int)len);
                }
                else
                {
                    // Change "prev" buffer to be the right size. This way
                    // the bytes are only copied once, and very long lines are
                    // allocated only once.
                    s = xrealloc(prev, prevlen + len + 1);
                    memcpy(s + prevlen, start, len);
                    s[prevlen + len] = NUL;
                    prev = NULL; // the list will own the string
                    prevlen = prevsize = 0;
                }

                li = tv_list_item_alloc();
                li->li_tv.v_type = kNvarString;
                li->li_tv.v_lock = 0;
                li->li_tv.vval.v_string = s;
                tv_list_append(rettv->vval.v_list, li);
                start = p + 1; // step over newline

                if((++cnt >= maxline && maxline >= 0) || readlen <= 0)
                {
                    break;
                }
            }
            else if(*p == NUL)
            {
                *p = '\n';
            }
            // Check for utf8 "bom"; U+FEFF is encoded as EF BB BF. Do this
            // when finding the BF and check the previous two bytes.
            else if(*p == 0xbf && !binary)
            {
                // Find the two bytes before the 0xbf.If p is at buf, or buf
                // + 1, these may be in the "prev" string.
                uchar_kt back1 = p >= buf + 1
                               ? p[-1] : prevlen >= 1
                               ? prev[prevlen - 1] : NUL;

                uchar_kt back2 = p >= buf + 2
                               ? p[-2] : p == buf + 1 && prevlen >= 1
                               ? prev[prevlen - 1] : prevlen >= 2
                               ? prev[prevlen - 2] : NUL;

                if(back2 == 0xef && back1 == 0xbb)
                {
                    uchar_kt *dest = p - 2;

                    // Usually a BOM is at the beginning of a file, and so at
                    // the beginning of a line; then we can just step over it.
                    if(start == dest)
                    {
                        start = p + 1;
                    }
                    else
                    {
                        // have to shuffle buf to close gap
                        int adjust_prevlen = 0;

                        if(dest < buf)
                        {
                            adjust_prevlen = (int)(buf - dest);
                            dest = buf; // adjust_prevlen must be 1 or 2.
                        }

                        if(readlen > p - buf + 1)
                        {
                            memmove(dest, p + 1, readlen - (p - buf) - 1);
                        }

                        readlen -= 3 - adjust_prevlen;
                        prevlen -= adjust_prevlen;
                        p = dest - 1;
                    }
                }
            }
        }

        if((cnt >= maxline && maxline >= 0) || readlen <= 0)
        {
            break;
        }

        if(start < p)
        {
            // There's part of a line in buf, store it in "prev".
            if(p - start + prevlen >= prevsize)
            {
                // A common use case is ordinary text files and "prev" gets a
                // fragment of a line, so the first allocation is made
                // small, to avoid repeatedly 'allocing' large and
                // 'reallocing' small.
                if(prevsize == 0)
                {
                    prevsize = (long)(p - start);
                }
                else
                {
                    long grow50pc = (prevsize * 3) / 2;
                    long growmin  = (long)((p - start) * 2 + prevlen);
                    prevsize = grow50pc > growmin ? grow50pc : growmin;
                }

                prev = xrealloc(prev, prevsize);
            }

            // Add the line part to end of "prev".
            memmove(prev + prevlen, start, p - start);
            prevlen += (long)(p - start);
        }
    }

    // For a negative line count use only the lines
    // at the end of the file, free the rest.
    if(maxline < 0)
    {
        while(cnt > -maxline)
        {
            tv_list_item_remove(rettv->vval.v_list,
                                rettv->vval.v_list->lv_first);
            cnt--;
        }
    }

    xfree(prev);
    fclose(fd);
}


/// convert a List to proftime_kt
///
/// @param arg
/// The input list, must be of type kNvarList and have exactly 2 items
///
/// @param[out] tm
/// The proftime_kt representation of @b arg
///
/// @return OK In case of success, FAIL in case of error
static int list2proftime(typval_st *arg, proftime_kt *tm)
FUNC_ATTR_NONNULL_ALL
{
    if(arg->v_type != kNvarList
       || arg->vval.v_list == NULL
       || arg->vval.v_list->lv_len != 2)
    {
        return FAIL;
    }

    bool error = false;
    number_kt n1 = tv_list_find_nr(arg->vval.v_list, 0L, &error);
    number_kt n2 = tv_list_find_nr(arg->vval.v_list, 1L, &error);

    if(error)
    {
        return FAIL;
    }

    // in f_reltime() we split up the 64-bit proftime_kt into two 32-bit
    // values, now we combine them again.
    union
    {
        struct { number_kt low, high; } split;
        proftime_kt prof;
    } u = { .split.high = n1, .split.low = n2 };

    *tm = u.prof;

    return OK;
}

/// return an item that represents a time value
///
/// @param[out]
/// rettv Without an argument it returns the current time. With
/// one argument it returns the time passed since the argument.
/// With two arguments it returns the time passed between the two arguments.
static void f_reltime(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    proftime_kt res;
    proftime_kt start;

    if(argvars[0].v_type == kNvarUnknown)
    {
        // no arguments: get current time.
        res = profile_start();
    }
    else if(argvars[1].v_type == kNvarUnknown)
    {
        if(list2proftime(&argvars[0], &res) == FAIL)
        {
            return;
        }

        res = profile_end(res);
    }
    else
    {
        // two arguments: compute the difference.
        if(list2proftime(&argvars[0], &start) == FAIL
           || list2proftime(&argvars[1], &res) == FAIL)
        {
            return;
        }

        res = profile_sub(res, start);
    }

    // we have to store the 64-bit proftime_kt inside of a list of int's
    // (number_kt is defined as int). For all our supported platforms, int's
    // are at least 32-bits wide. So we'll use two 32-bit values to store it.
    union
    {
        struct
        {
            number_kt low, high;
        } split;
        proftime_kt prof;
    } u = { .prof = res };

    // statically assert that the union type conv will provide the correct
    // results, if number_kt or proftime_kt change, the union cast will need
    // to be revised.
    STATIC_ASSERT(sizeof(u.prof) == sizeof(u) && sizeof(u.split) == sizeof(u),
                  "type punning will produce incorrect results on this platform");

    tv_list_alloc_ret(rettv);
    tv_list_append_number(rettv->vval.v_list, u.split.high);
    tv_list_append_number(rettv->vval.v_list, u.split.low);
}

/// return a string that represents the value of {time}
///
/// @return
/// The string representation of the argument, the format is the
/// number of seconds followed by a dot, followed by the number of microseconds.
static void f_reltimestr(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
FUNC_ATTR_NONNULL_ALL
{
    proftime_kt tm;

    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    if(list2proftime(&argvars[0], &tm) == OK)
    {
        rettv->vval.v_string = (uchar_kt *)xstrdup(profile_msg(tm));
    }
}

/// "remove()" function
static void f_remove(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    long idx;
    long end;
    dict_st *d;
    list_st *l;
    listitem_st *li;
    dictitem_st *di;
    listitem_st *item;
    listitem_st *item2;
    const char *const arg_errmsg = N_("remove() argument");

    if(argvars[0].v_type == kNvarDict)
    {
        if(argvars[2].v_type != kNvarUnknown)
        {
            EMSG2(_(e_toomanyarg), "remove()");
        }
        else if((d = argvars[0].vval.v_dict) != NULL
                && !tv_check_lock(d->dv_lock, arg_errmsg, TV_TRANSLATE))
        {
            const char *key = tv_get_string_chk(&argvars[1]);

            if(key != NULL)
            {
                di = tv_dict_find(d, key, -1);

                if(di == NULL)
                {
                    EMSG2(_(e_dictkey), key);
                }
                else if(!var_check_fixed(di->di_flags, arg_errmsg, TV_TRANSLATE)
                        && !var_check_ro(di->di_flags, arg_errmsg, TV_TRANSLATE))
                {
                    *rettv = di->di_tv;
                    di->di_tv = TV_INITIAL_VALUE;
                    tv_dict_item_remove(d, di);

                    if(tv_dict_is_watched(d))
                    {
                        tv_dict_watcher_notify(d, key, NULL, rettv);
                    }
                }
            }
        }
    }
    else if(argvars[0].v_type != kNvarList)
    {
        EMSG2(_(e_listdictarg), "remove()");
    }
    else if((l = argvars[0].vval.v_list) != NULL
            && !tv_check_lock(l->lv_lock, arg_errmsg, TV_TRANSLATE))
    {
        bool error = false;
        idx = tv_get_number_chk(&argvars[1], &error);

        if(error)
        {
            // Type error: do nothing, errmsg already given.
        }
        else if((item = tv_list_find(l, idx)) == NULL)
        {
            EMSGN(_(e_listidx), idx);
        }
        else
        {
            if(argvars[2].v_type == kNvarUnknown)
            {
                // Remove one item, return its value.
                tv_list_remove_items(l, item, item);
                *rettv = item->li_tv;
                xfree(item);
            }
            else
            {
                // Remove range of items, return list with values.
                end = tv_get_number_chk(&argvars[2], &error);

                if(error)
                {
                    // Type error: do nothing.
                }
                else if((item2 = tv_list_find(l, end)) == NULL)
                {
                    EMSGN(_(e_listidx), end);
                }
                else
                {
                    int cnt = 0;

                    for(li = item; li != NULL; li = li->li_next)
                    {
                        ++cnt;

                        if(li == item2)
                        {
                            break;
                        }
                    }

                    if(li == NULL) // Didn't find "item2" after "item".
                    {
                        emsgf(_(e_invrange));
                    }
                    else
                    {
                        tv_list_remove_items(l, item, item2);
                        l = tv_list_alloc_ret(rettv);
                        l->lv_first = item;
                        l->lv_last = item2;
                        item->li_prev = NULL;
                        item2->li_next = NULL;
                        l->lv_len = cnt;
                    }
                }
            }
        }
    }
}

/// "rename({from}, {to})" function
static void f_rename(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(check_restricted() || check_secure())
    {
        rettv->vval.v_number = -1;
    }
    else
    {
        char buf[NUMBUFLEN];

        rettv->vval.v_number =
            vim_rename((const uchar_kt *)tv_get_string(&argvars[0]),
                       (const uchar_kt *)tv_get_string_buf(&argvars[1], buf));
    }
}

/// "repeat()" function
static void f_repeat(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    number_kt n = tv_get_number(&argvars[1]);

    if(argvars[0].v_type == kNvarList)
    {
        tv_list_alloc_ret(rettv);

        while(n-- > 0)
        {
            tv_list_extend(rettv->vval.v_list, argvars[0].vval.v_list, NULL);
        }
    }
    else
    {
        rettv->v_type = kNvarString;
        rettv->vval.v_string = NULL;

        if(n <= 0)
        {
            return;
        }

        const char *const p = tv_get_string(&argvars[0]);
        const size_t slen = strlen(p);

        if(slen == 0)
        {
            return;
        }

        const size_t len = slen * n;

        // Detect overflow.
        if(len / n != slen)
        {
            return;
        }

        char *const r = xmallocz(len);

        for(number_kt i = 0; i < n; i++)
        {
            memmove(r + i * slen, p, slen);
        }

        rettv->vval.v_string = (uchar_kt *)r;
    }
}

/// "resolve()" function
static void f_resolve(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    const char *fname = tv_get_string(&argvars[0]);

#ifdef HOST_OS_WINDOWS
    char *const v = os_resolve_shortcut(fname);
    rettv->vval.v_string = (uchar_kt *)(v == NULL ? xstrdup(fname) : v);
#else
    #ifdef HAVE_FUN_READLINK
    {
        bool is_relative_to_current = false;
        bool has_trailing_pathsep = false;
        int limit = 100;
        char *p = xstrdup(fname);

        if(p[0] == '.'
           && (vim_ispathsep(p[1]) || (p[1] == '.' && (vim_ispathsep(p[2])))))
        {
            is_relative_to_current = true;
        }

        ptrdiff_t len = (ptrdiff_t)strlen(p);

        if(len > 0 && after_pathsep(p, p + len))
        {
            has_trailing_pathsep = true;
            p[len - 1] = NUL; // The trailing slash breaks readlink().
        }

        char *q = (char *)path_next_component(p);
        char *remain = NULL;

        if(*q != NUL)
        {
            // Separate the first path component in "p", and keep the
            // remainder (beginning with the path separator).
            remain = xstrdup(q - 1);
            q[-1] = NUL;
        }

        char *const buf = xmallocz(MAXPATHL);
        char *cpy;

        for(;;)
        {
            for(;;)
            {
                len = readlink(p, buf, MAXPATHL);

                if(len <= 0)
                {
                    break;
                }

                buf[len] = NUL;

                if(limit-- == 0)
                {
                    xfree(p);
                    xfree(remain);
                    EMSG(_("E655: Too many symbolic links (cycle?)"));
                    rettv->vval.v_string = NULL;
                    xfree(buf);
                    return;
                }

                // Ensure that the result will have a trailing
                // path separator if the argument has one.
                if(remain == NULL && has_trailing_pathsep)
                {
                    add_pathsep(buf);
                }

                // Separate the first path component in the
                // link value and concatenate the remainders.
                q = (char *)path_next_component(vim_ispathsep(*buf) ? buf + 1 : buf);

                if(*q != NUL)
                {
                    cpy = remain;
                    remain = (remain
                              ? xstrdup_concat(q - 1, remain)
                              : xstrdup(q - 1));

                    xfree(cpy);
                    q[-1] = NUL;
                }

                q = (char *)path_tail((uchar_kt *)p);

                if(q > p && *q == NUL)
                {
                    // Ignore trailing path separator.
                    q[-1] = NUL;
                    q = (char *)path_tail((uchar_kt *)p);
                }

                if(q > p && !path_is_absolute_path((const uchar_kt *)buf))
                {
                    // Symlink is relative to directory of argument. Replace the
                    // symlink with the resolved name in the same directory.
                    const size_t p_len = strlen(p);
                    const size_t buf_len = strlen(buf);
                    p = xrealloc(p, p_len + buf_len + 1);
                    memcpy(path_tail((uchar_kt *)p), buf, buf_len + 1);
                }
                else
                {
                    xfree(p);
                    p = xstrdup(buf);
                }
            }

            if(remain == NULL)
            {
                break;
            }

            // Append the first path component of "remain" to "p".
            q = (char *)path_next_component(remain + 1);
            len = q - remain - (*q != NUL);
            const size_t p_len = strlen(p);
            cpy = xmallocz(p_len + len);
            memcpy(cpy, p, p_len + 1);
            xstrncat(cpy + p_len, remain, len + 1);
            xfree(p);
            p = cpy;

            // Shorten "remain".
            if(*q != NUL)
            {
                xstrmove(remain, q - 1);
            }
            else
            {
                xfree(remain);
                remain = NULL;
            }
        }

        // If the result is a relative path name, make it explicitly
        // relative to the current directory if and only if the argument
        // had this form.
        if(!vim_ispathsep(*p))
        {
            if(is_relative_to_current
               && *p != NUL
               && !(p[0] == '.'
                    && (p[1] == NUL
                        || vim_ispathsep(p[1])
                        || (p[1] == '.'
                            && (p[2] == NUL || vim_ispathsep(p[2]))))))
            {
                // Prepend "./"
                cpy = xstrdup_concat("./", p);

                xfree(p);
                p = cpy;
            }
            else if(!is_relative_to_current)
            {
                // Strip leading "./"
                q = p;

                while(q[0] == '.' && vim_ispathsep(q[1]))
                {
                    q += 2;
                }

                if(q > p)
                {
                    xstrmove(p, p + 2);
                }
            }
        }

        // Ensure that the result will have no trailing path separator
        // if the argument had none.  But keep "/" or "//"
        if(!has_trailing_pathsep)
        {
            q = p + strlen(p);

            if(after_pathsep(p, q))
            {
                *path_tail_with_sep((uchar_kt *)p) = NUL;
            }
        }

        rettv->vval.v_string = (uchar_kt *)p;
        xfree(buf);
    }
    #else
    rettv->vval.v_string = (uchar_kt *)xstrdup(p);
    #endif
#endif
    simplify_filename(rettv->vval.v_string);
}

/// "reverse({list})" function
static void f_reverse(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    list_st *l;

    if(argvars[0].v_type != kNvarList)
    {
        EMSG2(_(e_listarg), "reverse()");
    }
    else if((l = argvars[0].vval.v_list) != NULL
            && !tv_check_lock(l->lv_lock, N_("reverse() argument"), TV_TRANSLATE))
    {
        listitem_st *li = l->lv_last;
        l->lv_first = l->lv_last = NULL;
        l->lv_len = 0;

        while(li != NULL)
        {
            listitem_st *const ni = li->li_prev;
            tv_list_append(l, li);
            li = ni;
        }

        rettv->vval.v_list = l;
        rettv->v_type = kNvarList;
        l->lv_refcount++;
        l->lv_idx = l->lv_len - l->lv_idx - 1;
    }
}

#define SP_NOMOVE       0x01    ///< don't move cursor
#define SP_REPEAT       0x02    ///< repeat to find outer pair
#define SP_RETCOUNT     0x04    ///< return matchcount
#define SP_SETPCMARK    0x08    ///< set previous context mark
#define SP_START        0x10    ///< accept match at start position
#define SP_SUBPAT       0x20    ///< return nr of matching sub-pattern
#define SP_END          0x40    ///< leave cursor at end of match
#define SP_COLUMN       0x80    ///< start at cursor column

/// Get flags for a search function.
/// Possibly sets "p_ws".
/// Returns BACKWARD, FORWARD or zero (for an error).
static int get_search_arg(typval_st *varp, int *flagsp)
{
    int dir = FORWARD;
    int mask;

    if(varp->v_type != kNvarUnknown)
    {
        char nbuf[NUMBUFLEN];
        const char *flags = tv_get_string_buf_chk(varp, nbuf);

        if(flags == NULL)
        {
            return 0; // Type error; errmsg already given.
        }

        while(*flags != NUL)
        {
            switch(*flags)
            {
                case 'b':
                    dir = BACKWARD;
                    break;

                case 'w':
                    p_ws = true;
                    break;

                case 'W':
                    p_ws = false;
                    break;

                default:
                {
                    mask = 0;

                    if(flagsp != NULL)
                    {
                        switch(*flags)
                        {
                            case 'c':
                                mask = SP_START;
                                break;

                            case 'e':
                                mask = SP_END;
                                break;

                            case 'm':
                                mask = SP_RETCOUNT;
                                break;

                            case 'n':
                                mask = SP_NOMOVE;
                                break;

                            case 'p':
                                mask = SP_SUBPAT;
                                break;

                            case 'r':
                                mask = SP_REPEAT;
                                break;

                            case 's':
                                mask = SP_SETPCMARK;
                                break;

                            case 'z':
                                mask = SP_COLUMN;
                                break;
                        }
                    }

                    if(mask == 0)
                    {
                        emsgf(_(e_invarg2), flags);
                        dir = 0;
                    }
                    else
                    {
                        *flagsp |= mask;
                    }
                }
            }

            if(dir == 0)
            {
                break;
            }

            flags++;
        }
    }

    return dir;
}

/// Shared by search() and searchpos() functions.
static int search_cmn(typval_st *argvars, apos_st *match_pos, int *flagsp)
{
    int flags;
    apos_st pos;
    apos_st save_cursor;
    bool save_p_ws = p_ws;
    int dir;
    int retval = 0; // default: FAIL
    long lnum_stop = 0;
    proftime_kt tm;
    long time_limit = 0;
    int options = SEARCH_KEEP;
    int subpatnum;
    const char *const pat = tv_get_string(&argvars[0]);
    dir = get_search_arg(&argvars[1], flagsp); // May set p_ws.

    if(dir == 0)
    {
        goto theend;
    }

    flags = *flagsp;

    if(flags & SP_START)
    {
        options |= SEARCH_START;
    }

    if(flags & SP_END)
    {
        options |= SEARCH_END;
    }

    if(flags & SP_COLUMN)
    {
        options |= SEARCH_COL;
    }

    // Optional arguments: line number to stop searching and timeout.
    if(argvars[1].v_type != kNvarUnknown && argvars[2].v_type != kNvarUnknown)
    {
        lnum_stop = tv_get_number_chk(&argvars[2], NULL);

        if(lnum_stop < 0)
        {
            goto theend;
        }

        if(argvars[3].v_type != kNvarUnknown)
        {
            time_limit = tv_get_number_chk(&argvars[3], NULL);

            if(time_limit < 0)
            {
                goto theend;
            }
        }
    }

    // Set the time limit, if there is one.
    tm = profile_setlimit(time_limit);

    // This function does not accept SP_REPEAT and SP_RETCOUNT flags.
    // Check to make sure only those flags are set.
    // Also, Only the SP_NOMOVE or the SP_SETPCMARK flag can be set. Both
    // flags cannot be set. Check for that condition also.
    if(((flags & (SP_REPEAT | SP_RETCOUNT)) != 0)
       || ((flags & SP_NOMOVE) && (flags & SP_SETPCMARK)))
    {
        EMSG2(_(e_invarg2), tv_get_string(&argvars[1]));
        goto theend;
    }

    pos = save_cursor = curwin->w_cursor;

    subpatnum = searchit(curwin,
                         curbuf,
                         &pos,
                         dir,
                         (uchar_kt *)pat,
                         1,
                         options,
                         RE_SEARCH,
                         (linenum_kt)lnum_stop,
                         &tm);

    if(subpatnum != FAIL)
    {
        if(flags & SP_SUBPAT)
        {
            retval = subpatnum;
        }
        else
        {
            retval = pos.lnum;
        }

        if(flags & SP_SETPCMARK)
        {
            setpcmark();
        }

        curwin->w_cursor = pos;

        if(match_pos != NULL)
        {
            // Store the match cursor position
            match_pos->lnum = pos.lnum;
            match_pos->col = pos.col + 1;
        }

        // "/$" will put the cursor after the end
        // of the line, may need to correct that here
        check_cursor();
    }

    // If 'n' flag is used: restore cursor position.
    if(flags & SP_NOMOVE)
    {
        curwin->w_cursor = save_cursor;
    }
    else
    {
        curwin->w_set_curswant = TRUE;
    }

theend:

    p_ws = save_p_ws;
    return retval;
}

/// "rpcnotify()" function
static void f_rpcnotify(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarNumber || argvars[0].vval.v_number < 0)
    {
        EMSG2(_(e_invarg2), "Channel id must be a positive integer");
        return;
    }

    if(argvars[1].v_type != kNvarString)
    {
        EMSG2(_(e_invarg2), "Event type must be a string");
        return;
    }

    Array args = ARRAY_DICT_INIT;

    for(typval_st *tv = argvars + 2; tv->v_type != kNvarUnknown; tv++)
    {
        ADD(args, nvim_to_object(tv));
    }

    if(!channel_send_event((uint64_t)argvars[0].vval.v_number,
                           tv_get_string(&argvars[1]),
                           args))
    {
        EMSG2(_(e_invarg2), "Channel doesn't exist");
        return;
    }

    rettv->vval.v_number = 1;
}

/// "rpcrequest()" function
static void f_rpcrequest(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;
    const int l_provider_call_nesting = provider_call_nesting;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarNumber || argvars[0].vval.v_number <= 0)
    {
        EMSG2(_(e_invarg2), "Channel id must be a positive integer");
        return;
    }

    if(argvars[1].v_type != kNvarString)
    {
        EMSG2(_(e_invarg2), "Method name must be a string");
        return;
    }

    Array args = ARRAY_DICT_INIT;

    for(typval_st *tv = argvars + 2; tv->v_type != kNvarUnknown; tv++)
    {
        ADD(args, nvim_to_object(tv));
    }

    script_id_kt save_current_SID;
    uint8_t *save_sourcing_name, *save_autocmd_fname, *save_autocmd_match;
    linenum_kt save_sourcing_lnum;
    int save_autocmd_fname_full, save_autocmd_bufnr;
    void *save_funccalp;

    if(l_provider_call_nesting)
    {
        // If this is called from a provider function,
        // restore the scope information of the caller.
        save_current_SID = current_SID;
        save_sourcing_name = sourcing_name;
        save_sourcing_lnum = sourcing_lnum;
        save_autocmd_fname = autocmd_fname;
        save_autocmd_match = autocmd_match;
        save_autocmd_fname_full = autocmd_fname_full;
        save_autocmd_bufnr = autocmd_bufnr;
        save_funccalp = save_funccal();

        //
        current_SID = provider_caller_scope.SID;
        sourcing_name = provider_caller_scope.sourcing_name;
        sourcing_lnum = provider_caller_scope.sourcing_lnum;
        autocmd_fname = provider_caller_scope.autocmd_fname;
        autocmd_match = provider_caller_scope.autocmd_match;
        autocmd_fname_full = provider_caller_scope.autocmd_fname_full;
        autocmd_bufnr = provider_caller_scope.autocmd_bufnr;
        restore_funccal(provider_caller_scope.funccalp);
    }

    error_st err = ERROR_INIT;
    Object result = channel_send_call((uint64_t)argvars[0].vval.v_number,
                                      tv_get_string(&argvars[1]),
                                      args,
                                      &err);

    if(l_provider_call_nesting)
    {
        current_SID = save_current_SID;
        sourcing_name = save_sourcing_name;
        sourcing_lnum = save_sourcing_lnum;
        autocmd_fname = save_autocmd_fname;
        autocmd_match = save_autocmd_match;
        autocmd_fname_full = save_autocmd_fname_full;
        autocmd_bufnr = save_autocmd_bufnr;
        restore_funccal(save_funccalp);
    }

    if(ERROR_SET(&err))
    {
        nvim_errmsg_writeln(cstr_as_string(err.msg));
        goto end;
    }

    if(!object_to_vim(result, rettv, &err))
    {
        EMSG2(_("Error converting the call result: %s"), err.msg);
    }

end:

    api_free_object(result);
    api_clear_error(&err);
}

/// "rpcstart()" function (DEPRECATED)
static void f_rpcstart(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarString
       || (argvars[1].v_type != kNvarList && argvars[1].v_type != kNvarUnknown))
    {
        // Wrong argument types
        EMSG(_(e_invarg));
        return;
    }

    list_st *args = NULL;
    int argsl = 0;

    if(argvars[1].v_type == kNvarList)
    {
        args = argvars[1].vval.v_list;
        argsl = args->lv_len;

        // Assert that all list items are strings
        for(listitem_st *arg = args->lv_first; arg != NULL; arg = arg->li_next)
        {
            if(arg->li_tv.v_type != kNvarString)
            {
                EMSG(_(e_invarg));
                return;
            }
        }
    }

    if(argvars[0].vval.v_string == NULL || argvars[0].vval.v_string[0] == NUL)
    {
        EMSG(_(e_api_spawn_failed));
        return;
    }

    // Allocate extra memory for the argument vector and the NULL pointer
    int argvl = argsl + 2;
    char **argv = xmalloc(sizeof(uchar_kt *) * argvl);

    // Copy program name
    argv[0] = xstrdup((char *)argvars[0].vval.v_string);
    int i = 1;

    // Copy arguments to the vector
    if(argsl > 0)
    {
        for(listitem_st *arg = args->lv_first; arg != NULL; arg = arg->li_next)
        {
            argv[i++] = xstrdup(tv_get_string(&arg->li_tv));
        }
    }

    // The last item of argv must be NULL
    argv[i] = NULL;
    terminal_jobdata_st *data = common_job_init(argv,
                                            CALLBACK_NONE,
                                            CALLBACK_NONE,
                                            CALLBACK_NONE,
                                            false,
                                            true,
                                            false,
                                            NULL);
    common_job_start(data, rettv);
}

/// "rpcstop()" function
static void f_rpcstop(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarNumber;
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarNumber)
    {
        // Wrong argument types
        EMSG(_(e_invarg));
        return;
    }

    // if called with a job, stop it, else closes the channel
    if(pmap_get(uint64_t)(jobs, argvars[0].vval.v_number))
    {
        f_jobstop(argvars, rettv, NULL);
    }
    else
    {
        rettv->vval.v_number = channel_close(argvars[0].vval.v_number);
    }
}

/// "screenattr()" function
static void f_screenattr(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int c;
    const int row = (int)tv_get_number_chk(&argvars[0], NULL) - 1;
    const int col = (int)tv_get_number_chk(&argvars[1], NULL) - 1;

    if(row < 0 || row >= screen_Rows || col < 0 || col >= screen_Columns)
    {
        c = -1;
    }
    else
    {
        c = ScreenAttrs[LineOffset[row] + col];
    }

    rettv->vval.v_number = c;
}

/// "screenchar()" function
static void f_screenchar(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int off;
    int c;
    const int row = tv_get_number_chk(&argvars[0], NULL) - 1;
    const int col = tv_get_number_chk(&argvars[1], NULL) - 1;

    if(row < 0 || row >= screen_Rows || col < 0 || col >= screen_Columns)
    {
        c = -1;
    }
    else
    {
        off = LineOffset[row] + col;

        if(ScreenLinesUC[off] != 0)
        {
            c = ScreenLinesUC[off];
        }
        else
        {
            c = ScreenLines[off];
        }
    }

    rettv->vval.v_number = c;
}

/// "screencol()" function
///
/// First column is 1 to be consistent with virtcol().
static void f_screencol(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = ui_current_col() + 1;
}

/// "screenrow()" function
static void f_screenrow(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = ui_current_row() + 1;
}

/// "search()" function
static void f_search(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int flags = 0;
    rettv->vval.v_number = search_cmn(argvars, NULL, &flags);
}

/// "searchdecl()" function
static void f_searchdecl(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int locally = 1;
    int thisblock = 0;
    bool error = false;
    rettv->vval.v_number = 1; // default: FAIL
    const char *const name = tv_get_string_chk(&argvars[0]);

    if(argvars[1].v_type != kNvarUnknown)
    {
        locally = tv_get_number_chk(&argvars[1], &error) == 0;

        if(!error && argvars[2].v_type != kNvarUnknown)
        {
            thisblock = tv_get_number_chk(&argvars[2], &error) != 0;
        }
    }

    if(!error && name != NULL)
    {
        rettv->vval.v_number = find_decl((uchar_kt *)name,
                                         strlen(name),
                                         locally,
                                         thisblock,
                                         SEARCH_KEEP) == FAIL;
    }
}

/// Used by searchpair() and searchpairpos()
static int searchpair_cmn(typval_st *argvars, apos_st *match_pos)
{
    bool save_p_ws = p_ws;
    int dir;
    int flags = 0;
    int retval = 0; // default: FAIL
    long lnum_stop = 0;
    long time_limit = 0;

    // Get the three pattern arguments: start, middle, end.
    char nbuf1[NUMBUFLEN];
    char nbuf2[NUMBUFLEN];
    char nbuf3[NUMBUFLEN];

    const char *spat = tv_get_string_chk(&argvars[0]);
    const char *mpat = tv_get_string_buf_chk(&argvars[1], nbuf1);
    const char *epat = tv_get_string_buf_chk(&argvars[2], nbuf2);

    if(spat == NULL || mpat == NULL || epat == NULL)
    {
        goto theend; // Type error.
    }

    // Handle the optional fourth argument: flags.
    dir = get_search_arg(&argvars[3], &flags); // may set p_ws.

    if(dir == 0)
    {
        goto theend;
    }

    // Don't accept SP_END or SP_SUBPAT.
    // Only one of the SP_NOMOVE or SP_SETPCMARK flags can be set.
    if((flags & (SP_END | SP_SUBPAT)) != 0
       || ((flags & SP_NOMOVE) && (flags & SP_SETPCMARK)))
    {
        EMSG2(_(e_invarg2), tv_get_string(&argvars[3]));
        goto theend;
    }

    // Using 'r' implies 'W', otherwise it doesn't work.
    if(flags & SP_REPEAT)
    {
        p_ws = false;
    }

    // Optional fifth argument: skip expression.
    const char *skip;

    if(argvars[3].v_type == kNvarUnknown
       || argvars[4].v_type == kNvarUnknown)
    {
        skip = "";
    }
    else
    {
        skip = tv_get_string_buf_chk(&argvars[4], nbuf3);

        if(skip == NULL)
        {
            goto theend; // Type error.
        }

        if(argvars[5].v_type != kNvarUnknown)
        {
            lnum_stop = tv_get_number_chk(&argvars[5], NULL);

            if(lnum_stop < 0)
            {
                goto theend;
            }

            if(argvars[6].v_type != kNvarUnknown)
            {
                time_limit = tv_get_number_chk(&argvars[6], NULL);

                if(time_limit < 0)
                {
                    goto theend;
                }
            }
        }
    }

    retval = do_searchpair((uchar_kt *)spat,
                           (uchar_kt *)mpat,
                           (uchar_kt *)epat,
                           dir,
                           (uchar_kt *)skip,
                           flags,
                           match_pos,
                           lnum_stop,
                           time_limit);
theend:

    p_ws = save_p_ws;
    return retval;
}

/// "searchpair()" function
static void f_searchpair(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = searchpair_cmn(argvars, NULL);
}

/// "searchpairpos()" function
static void f_searchpairpos(typval_st *argvars,
                            typval_st *rettv,
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    apos_st match_pos;
    int lnum = 0;
    int col = 0;
    tv_list_alloc_ret(rettv);

    if(searchpair_cmn(argvars, &match_pos) > 0)
    {
        lnum = match_pos.lnum;
        col = match_pos.col;
    }

    tv_list_append_number(rettv->vval.v_list, (number_kt)lnum);
    tv_list_append_number(rettv->vval.v_list, (number_kt)col);
}

/*
 * Search for a start/middle/end thing.
 * Used by searchpair(), see its documentation for the details.
 * Returns ,
 */
///
/// @param spat         start pattern
/// @param mpat         middle pattern
/// @param epat         end pattern
/// @param dir          BACKWARD or FORWARD
/// @param skip         skip expression
/// @param flags        SP_SETPCMARK and other SP_ values
/// @param match_pos
/// @param lnum_stop    stop at this line if not zero
/// @param time_limit   stop after this many msec
///
/// @return 0 or -1 for no match
long do_searchpair(uchar_kt *spat,
                   uchar_kt *mpat,
                   uchar_kt *epat,
                   int dir,
                   uchar_kt *skip,
                   int flags,
                   apos_st *match_pos,
                   linenum_kt lnum_stop,
                   long time_limit)
{
    uchar_kt *save_cpo;
    uchar_kt *pat = NULL;
    uchar_kt *pat2 = NULL;
    uchar_kt *pat3 = NULL;
    long retval = 0;
    apos_st pos;
    apos_st firstpos;
    apos_st foundpos;
    apos_st save_cursor;
    apos_st save_pos;
    int n;
    int r;
    int nest = 1;
    int options = SEARCH_KEEP;
    proftime_kt tm;

    // Make 'cpoptions' empty, the 'l' flag should not be used here.
    save_cpo = p_cpo;
    p_cpo = empty_option;

    // Set the time limit, if there is one.
    tm = profile_setlimit(time_limit);

    // Make two search patterns: start/end (pat2,
    // for in nested pairs) and start/middle/end (pat3, for the top pair).
    pat2 = xmalloc(ustrlen(spat) + ustrlen(epat) + 15);
    pat3 = xmalloc(ustrlen(spat) + ustrlen(mpat) + ustrlen(epat) + 23);

    sprintf((char *)pat2, "\\(%s\\m\\)\\|\\(%s\\m\\)", spat, epat);

    if(*mpat == NUL)
    {
        ustrcpy(pat3, pat2);
    }
    else
    {
        sprintf((char *)pat3,
                "\\(%s\\m\\)\\|\\(%s\\m\\)\\|\\(%s\\m\\)", spat, epat, mpat);
    }

    if(flags & SP_START)
    {
        options |= SEARCH_START;
    }

    save_cursor = curwin->w_cursor;
    pos = curwin->w_cursor;
    clearpos(&firstpos);
    clearpos(&foundpos);
    pat = pat3;

    for(;;)
    {
        n = searchit(curwin,
                     curbuf,
                     &pos,
                     dir,
                     pat,
                     1L,
                     options,
                     RE_SEARCH,
                     lnum_stop,
                     &tm);

        // didn't find it or found the first match again: FAIL
        if(n == FAIL || (firstpos.lnum != 0 && equalpos(pos, firstpos)))
        {
            break;
        }

        if(firstpos.lnum == 0)
        {
            firstpos = pos;
        }

        if(equalpos(pos, foundpos))
        {
            // Found the same position again. Can happen with a pattern that
            // has "\zs" at the end and searching backwards. Advance one
            // character and try again.
            if(dir == BACKWARD)
            {
                decl(&pos);
            }
            else
            {
                incl(&pos);
            }
        }

        foundpos = pos;

        // clear the start flag to avoid getting stuck here
        options &= ~SEARCH_START;

        // If the skip pattern matches, ignore this match. *
        if(*skip != NUL)
        {
            save_pos = curwin->w_cursor;
            curwin->w_cursor = pos;
            bool err;
            r = eval_to_bool(skip, &err, NULL, false);
            curwin->w_cursor = save_pos;

            if(err)
            {
                // Evaluating {skip} caused an error, break here.
                curwin->w_cursor = save_cursor;
                retval = -1;
                break;
            }

            if(r)
            {
                continue;
            }
        }

        if((dir == BACKWARD && n == 3) || (dir == FORWARD && n == 2))
        {
            // Found end when searching backwards or
            // start when searching forward: nested pair.
            ++nest;
            pat = pat2; // nested, don't search for middle
        }
        else
        {
            // Found end when searching forward or start when searching
            // backward: end of (nested) pair; or found middle in outer pair.
            if(--nest == 1)
            {
                pat = pat3; // outer level, search for middle
            }
        }

        if(nest == 0)
        {
            // Found the match: return matchcount or line number.
            if(flags & SP_RETCOUNT)
            {
                ++retval;
            }
            else
            {
                retval = pos.lnum;
            }

            if(flags & SP_SETPCMARK)
            {
                setpcmark();
            }

            curwin->w_cursor = pos;

            if(!(flags & SP_REPEAT))
            {
                break;
            }

            nest = 1; // search for next unmatched
        }
    }

    if(match_pos != NULL)
    {
        // Store the match cursor position
        match_pos->lnum = curwin->w_cursor.lnum;
        match_pos->col = curwin->w_cursor.col + 1;
    }

    // If 'n' flag is used or search failed: restore cursor position.
    if((flags & SP_NOMOVE) || retval == 0)
    {
        curwin->w_cursor = save_cursor;
    }

    xfree(pat2);
    xfree(pat3);

    if(p_cpo == empty_option)
    {
        p_cpo = save_cpo;
    }
    else
    {
        // Darn, evaluating the {skip}
        // expression changed the value.
        free_string_option(save_cpo);
    }

    return retval;
}

/// "searchpos()" function
static void f_searchpos(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    apos_st match_pos;
    int lnum = 0;
    int col = 0;
    int n;
    int flags = 0;
    tv_list_alloc_ret(rettv);
    n = search_cmn(argvars, &match_pos, &flags);

    if(n > 0)
    {
        lnum = match_pos.lnum;
        col = match_pos.col;
    }

    tv_list_append_number(rettv->vval.v_list, (number_kt)lnum);
    tv_list_append_number(rettv->vval.v_list, (number_kt)col);

    if(flags & SP_SUBPAT)
    {
        tv_list_append_number(rettv->vval.v_list, (number_kt)n);
    }
}

/// "serverlist()" function
static void f_serverlist(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    size_t n;
    char **addrs = server_address_list(&n);

    // Copy addrs into a linked list.
    list_st *l = tv_list_alloc_ret(rettv);

    for(size_t i = 0; i < n; i++)
    {
        listitem_st *li = tv_list_item_alloc();
        li->li_tv.v_type = kNvarString;
        li->li_tv.v_lock = 0;
        li->li_tv.vval.v_string = (uchar_kt *)addrs[i];
        tv_list_append(l, li);
    }

    xfree(addrs);
}

/// "serverstart()" function
static void f_serverstart(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL; // Address of the new server

    if(check_restricted() || check_secure())
    {
        return;
    }

    char *address;

    // If the user supplied an address, use it, otherwise use a temp.
    if(argvars[0].v_type != kNvarUnknown)
    {
        if(argvars[0].v_type != kNvarString)
        {
            EMSG(_(e_invarg));
            return;
        }
        else
        {
            address = xstrdup(tv_get_string(argvars));
        }
    }
    else
    {
        address = server_address_new();
    }

    int result = server_start(address);
    xfree(address);

    if(result != 0)
    {
        EMSG2("Failed to start server: %s",
              result > 0 ? "Unknonwn system error" : uv_strerror(result));
        return;
    }

    // Since it's possible server_start adjusted the given {address} (e.g.,
    // "localhost:" will now have a port), return the final value to the user.
    size_t n;
    char **addrs = server_address_list(&n);
    rettv->vval.v_string = (uchar_kt *)addrs[n - 1];
    n--;

    for(size_t i = 0; i < n; i++)
    {
        xfree(addrs[i]);
    }

    xfree(addrs);
}

/// "serverstop()" function
static void f_serverstop(typval_st *argvars,
                         typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarString)
    {
        EMSG(_(e_invarg));
        return;
    }

    if(argvars[0].vval.v_string)
    {
        server_stop((char *) argvars[0].vval.v_string);
    }
}

/// "setbufvar()" function
static void f_setbufvar(typval_st *argvars,
                        typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(check_restricted()
       || check_secure()
       || !tv_check_str_or_nr(&argvars[0]))
    {
        return;
    }

    const char *varname = tv_get_string_chk(&argvars[1]);
    filebuf_st *const buf = get_buf_tv(&argvars[0], false);
    typval_st *varp = &argvars[2];

    if(buf != NULL && varname != NULL)
    {
        if(*varname == '&')
        {
            long numval;
            bool error = false;
            save_autocmd_st aco;

            // set curbuf to be our buf, temporarily
            aucmd_prepbuf(&aco, buf);
            varname++;
            numval = tv_get_number_chk(varp, &error);
            char nbuf[NUMBUFLEN];
            const char *const strval = tv_get_string_buf_chk(varp, nbuf);

            if(!error && strval != NULL)
            {
                set_option_value(varname, numval, strval, kOptSetLocal);
            }

            // reset notion of buffer
            aucmd_restbuf(&aco);
        }
        else
        {
            filebuf_st *save_curbuf = curbuf;
            const size_t varname_len = ustrlen(varname);
            char *const bufvarname = xmalloc(varname_len + 3);

            curbuf = buf;
            memcpy(bufvarname, "b:", 2);
            memcpy(bufvarname + 2, varname, varname_len + 1);
            set_var(bufvarname, varname_len + 2, varp, true);
            xfree(bufvarname);
            curbuf = save_curbuf;
        }
    }
}

static void f_setcharsearch(typval_st *argvars,
                            typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    dict_st *d;
    dictitem_st *di;

    if(argvars[0].v_type != kNvarDict)
    {
        EMSG(_(e_dictreq));
        return;
    }

    if((d = argvars[0].vval.v_dict) != NULL)
    {
        uchar_kt *const csearch = (uchar_kt *)tv_dict_get_string(d, "char", false);

        if(csearch != NULL)
        {
            int pcc[MAX_MCO];
            int c = utfc_ptr2char(csearch, pcc);
            set_last_csearch(c, csearch, utfc_ptr2len(csearch));
        }

        di = tv_dict_find(d, S_LEN("forward"));

        if(di != NULL)
        {
            set_csearch_direction(tv_get_number(&di->di_tv) ? FORWARD : BACKWARD);
        }

        di = tv_dict_find(d, S_LEN("until"));

        if(di != NULL)
        {
            set_csearch_until(!!tv_get_number(&di->di_tv));
        }
    }
}

/// "setcmdpos()" function
static void f_setcmdpos(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const int pos = (int)tv_get_number(&argvars[0]) - 1;

    if(pos >= 0)
    {
        rettv->vval.v_number = set_cmdline_pos(pos);
    }
}


/// "setfperm({fname}, {mode})" function
static void f_setfperm(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = 0;
    const char *const fname = tv_get_string_chk(&argvars[0]);

    if(fname == NULL)
    {
        return;
    }

    char modebuf[NUMBUFLEN];
    const char *const mode_str = tv_get_string_buf_chk(&argvars[1], modebuf);

    if(mode_str == NULL)
    {
        return;
    }

    if(strlen(mode_str) != 9)
    {
        EMSG2(_(e_invarg2), mode_str);
        return;
    }

    int mask = 1;
    int mode = 0;

    for(int i = 8; i >= 0; i--)
    {
        if(mode_str[i] != '-')
        {
            mode |= mask;
        }

        mask = mask << 1;
    }

    rettv->vval.v_number = os_setperm(fname, mode) == OK;
}

/// "setline()" function
static void f_setline(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    list_st *l = NULL;
    listitem_st *li = NULL;
    long added = 0;
    linenum_kt lcount = curbuf->b_ml.ml_line_count;
    linenum_kt lnum = tv_get_lnum(&argvars[0]);
    const char *line = NULL;

    if(argvars[1].v_type == kNvarList)
    {
        l = argvars[1].vval.v_list;
        li = l->lv_first;
    }
    else
    {
        line = tv_get_string_chk(&argvars[1]);
    }

    // default result is zero == OK
    for(;;)
    {
        if(l != NULL)
        {
            // List argument, get next string.
            if(li == NULL)
            {
                break;
            }

            line = tv_get_string_chk(&li->li_tv);
            li = li->li_next;
        }

        rettv->vval.v_number = 1; // FAIL

        if(line == NULL || lnum < 1 || lnum > curbuf->b_ml.ml_line_count + 1)
        {
            break;
        }

        // When coming here from Insert mode, sync undo, so that this can be
        // undone separately from what was previously inserted.
        if(u_sync_once == 2)
        {
            u_sync_once = 1; // notify that u_sync() was called
            u_sync(TRUE);
        }

        if(lnum <= curbuf->b_ml.ml_line_count)
        {
            // Existing line, replace it.
            if(u_savesub(lnum) == OK
               && ml_replace(lnum, (uchar_kt *)line, true) == OK)
            {
                changed_bytes(lnum, 0);

                if(lnum == curwin->w_cursor.lnum)
                {
                    check_cursor_col();
                }

                rettv->vval.v_number = 0; // OK
            }
        }
        else if(added > 0 || u_save(lnum - 1, lnum) == OK)
        {
            // lnum is one past the last line, append the line.
            added++;

            if(ml_append(lnum - 1, (uchar_kt *)line, 0, false) == OK)
            {
                rettv->vval.v_number = 0; // OK
            }
        }

        if(l == NULL) // only one string argument
        {
            break;
        }

        ++lnum;
    }

    if(added > 0)
    {
        appended_lines_mark(lcount, added);
    }
}

/// Create quickfix/location list from VimL values
///
/// Used by `setqflist()` and `setloclist()` functions. Accepts invalid
/// list_arg, action_arg and what_arg arguments in which case errors out,
/// including kNvarUnknown parameters.
///
/// @param[in,out]  wp      Window to create location list for. May be NULL in
///                         which case quickfix list will be created.
/// @param[in]  list_arg    Quickfix list contents.
/// @param[in]  action_arg  Action to perform: append to an existing list,
///                         replace its content or create a new one.
/// @param[in]  title_arg   New list title. Defaults to caller function name.
/// @param[out]  rettv      Return value: 0 in case of success, -1 otherwise.
static void set_qf_ll_list(win_st *wp, typval_st *args, typval_st *rettv)
FUNC_ATTR_NONNULL_ARG(2, 3)
{
    static char *e_invact = N_("E927: Invalid action: '%s'");
    const char *title = NULL;
    int action = ' ';
    rettv->vval.v_number = -1;
    dict_st *d = NULL;
    typval_st *list_arg = &args[0];

    if(list_arg->v_type != kNvarList)
    {
        EMSG(_(e_listreq));
        return;
    }

    typval_st *action_arg = &args[1];

    if(action_arg->v_type == kNvarUnknown)
    {
        // Option argument was not given.
        goto skip_args;
    }
    else if(action_arg->v_type != kNvarString)
    {
        EMSG(_(e_stringreq));
        return;
    }

    const char *const act = tv_get_string_chk(action_arg);

    if((*act == 'a' || *act == 'r' || *act == ' ') && act[1] == NUL)
    {
        action = *act;
    }
    else
    {
        EMSG2(_(e_invact), act);
        return;
    }

    typval_st *title_arg = &args[2];

    if(title_arg->v_type == kNvarUnknown)
    {
        // Option argument was not given.
        goto skip_args;
    }
    else if(title_arg->v_type == kNvarString)
    {
        title = tv_get_string_chk(title_arg);

        if(!title)
        {
            // Type error.
            // Error already printed by tv_get_string_chk().
            return;
        }
    }
    else if(title_arg->v_type == kNvarDict)
    {
        d = title_arg->vval.v_dict;
    }
    else
    {
        emsgf(_(e_dictreq));
        return;
    }

skip_args:

    if(!title)
    {
        title = (wp ? "setloclist()" : "setqflist()");
    }

    list_st *l = list_arg->vval.v_list;

    if(l && set_errorlist(wp, l, action, (uchar_kt *)title, d) == OK)
    {
        rettv->vval.v_number = 0;
    }
}

/// "setloclist()" function
static void f_setloclist(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    win_st *win;
    rettv->vval.v_number = -1;
    win = find_win_by_nr(&argvars[0], NULL);

    if(win != NULL)
    {
        set_qf_ll_list(win, &argvars[1], rettv);
    }
}

/// "setmatches()" function
static void f_setmatches(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    list_st *l;
    listitem_st *li;
    dict_st *d;
    list_st *s = NULL;
    rettv->vval.v_number = -1;

    if(argvars[0].v_type != kNvarList)
    {
        EMSG(_(e_listreq));
        return;
    }

    if((l = argvars[0].vval.v_list) != NULL)
    {
        // To some extent make sure that we are dealing with a list from
        // "getmatches()".
        li = l->lv_first;

        while(li != NULL)
        {
            if(li->li_tv.v_type != kNvarDict
               || (d = li->li_tv.vval.v_dict) == NULL)
            {
                EMSG(_(e_invarg));
                return;
            }

            if(!(tv_dict_find(d, S_LEN("group")) != NULL
                 && (tv_dict_find(d, S_LEN("pattern")) != NULL
                     || tv_dict_find(d, S_LEN("pos1")) != NULL)
                 && tv_dict_find(d, S_LEN("priority")) != NULL
                 && tv_dict_find(d, S_LEN("id")) != NULL))
            {
                EMSG(_(e_invarg));
                return;
            }

            li = li->li_next;
        }

        clear_matches(curwin);
        li = l->lv_first;
        bool match_add_failed = false;

        while(li != NULL)
        {
            int i = 0;
            d = li->li_tv.vval.v_dict;
            dictitem_st *const di = tv_dict_find(d, S_LEN("pattern"));

            if(di == NULL)
            {
                if(s == NULL)
                {
                    s = tv_list_alloc();

                    if(s == NULL)
                    {
                        return;
                    }
                }

                // match from matchaddpos()
                for(i = 1; i < 9; i++)
                {
                    char buf[5];
                    snprintf(buf, sizeof(buf), "pos%d", i);
                    dictitem_st *const pos_di = tv_dict_find(d, buf, -1);

                    if(pos_di != NULL)
                    {
                        if(pos_di->di_tv.v_type != kNvarList)
                        {
                            return;
                        }

                        tv_list_append_tv(s, &pos_di->di_tv);
                        s->lv_refcount++;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            // Note: there are three number buffers involved:
            // - group_buf below.
            // - numbuf in tv_dict_get_string().
            // - mybuf in tv_get_string().
            //
            // If you change this code make sure that buffers will not get
            // accidentally reused.
            char group_buf[NUMBUFLEN];

            const char *const group =
                tv_dict_get_string_buf(d, "group", group_buf);

            const int priority = (int)tv_dict_get_number(d, "priority");
            const int id = (int)tv_dict_get_number(d, "id");
            dictitem_st *const conceal_di = tv_dict_find(d, S_LEN("conceal"));

            const char *const conceal =
                (conceal_di != NULL ? tv_get_string(&conceal_di->di_tv) : NULL);

            if(i == 0)
            {
                if(match_add(curwin,
                             group,
                             tv_dict_get_string(d, "pattern", false),
                             priority,
                             id,
                             NULL,
                             conceal) != id)
                {
                    match_add_failed = true;
                }
            }
            else
            {
                if(match_add(curwin, group, NULL, priority, id, s, conceal) != id)
                {
                    match_add_failed = true;
                }

                tv_list_unref(s);
                s = NULL;
            }

            li = li->li_next;
        }

        if(!match_add_failed)
        {
            rettv->vval.v_number = 0;
        }
    }
}

/// "setpos()" function
static void f_setpos(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    apos_st pos;
    int fnum;
    columnum_kt curswant = -1;
    rettv->vval.v_number = -1;
    const char *const name = tv_get_string_chk(argvars);

    if(name != NULL)
    {
        if(list2fpos(&argvars[1], &pos, &fnum, &curswant) == OK)
        {
            if(--pos.col < 0)
            {
                pos.col = 0;
            }

            if(name[0] == '.' && name[1] == NUL)
            {
                // set cursor
                if(fnum == curbuf->b_id)
                {
                    curwin->w_cursor = pos;

                    if(curswant >= 0)
                    {
                        curwin->w_curswant = curswant - 1;
                        curwin->w_set_curswant = false;
                    }

                    check_cursor();
                    rettv->vval.v_number = 0;
                }
                else
                {
                    EMSG(_(e_invarg));
                }
            }
            else if(name[0] == '\'' && name[1] != NUL && name[2] == NUL)
            {
                // set mark
                if(setmark_pos((uint8_t)name[1], &pos, fnum) == OK)
                {
                    rettv->vval.v_number = 0;
                }
            }
            else
            {
                EMSG(_(e_invarg));
            }
        }
    }
}

/// "setqflist()" function
static void f_setqflist(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    set_qf_ll_list(NULL, argvars, rettv);
}

/// "setreg()" function
static void f_setreg(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int regname;
    bool append = false;
    motion_type_et yank_type;
    long block_len;
    block_len = -1;
    yank_type = kMTUnknown;
    rettv->vval.v_number = 1; // FAIL is default.
    const char *const strregname = tv_get_string_chk(argvars);

    if(strregname == NULL)
    {
        return; // Type error; errmsg already given.
    }

    regname = (uint8_t)(*strregname);

    if(regname == 0 || regname == '@')
    {
        regname = '"';
    }

    if(argvars[2].v_type != kNvarUnknown)
    {
        const char *stropt = tv_get_string_chk(&argvars[2]);

        if(stropt == NULL)
        {
            return; // Type error.
        }

        for(; *stropt != NUL; stropt++)
        {
            switch(*stropt)
            {
                case 'a':
                case 'A': // append
                {
                    append = true;
                    break;
                }

                case 'v':
                case 'c': // character-wise selection
                {
                    yank_type = kMTCharWise;
                    break;
                }

                case 'V':
                case 'l': // line-wise selection
                {
                    yank_type = kMTLineWise;
                    break;
                }

                case 'b':
                case Ctrl_V: // block-wise selection
                {
                    yank_type = kMTBlockWise;

                    if(ascii_isdigit(stropt[1]))
                    {
                        stropt++;
                        block_len = getdigits_long((uchar_kt **)&stropt) - 1;
                        stropt--;
                    }

                    break;
                }
            }
        }
    }

    if(argvars[1].v_type == kNvarList)
    {
        list_st *ll = argvars[1].vval.v_list;

        // If the list is NULL handle like an empty list.
        int len = ll == NULL ? 0 : ll->lv_len;

        // First half: use for pointers to result lines;
        // second half: use for pointers to allocated copies.
        char **lstval = xmalloc(sizeof(char *) * ((len + 1) * 2));
        const char **curval = (const char **)lstval;
        char **allocval = lstval + len + 2;
        char **curallocval = allocval;

        for(listitem_st *li = ll == NULL ? NULL : ll->lv_first;
            li != NULL;
            li = li->li_next)
        {
            char buf[NUMBUFLEN];
            *curval = tv_get_string_buf_chk(&li->li_tv, buf);

            if(*curval == NULL)
            {
                goto free_lstval;
            }

            if(*curval == buf)
            {
                // Need to make a copy,
                // next tv_get_string_buf_chk() will overwrite the string.
                *curallocval = xstrdup(*curval);
                *curval = *curallocval;
                curallocval++;
            }

            curval++;
        }

        *curval++ = NULL;
        write_reg_contents_lst(regname,
                               (uchar_kt **)lstval,
                               append,
                               yank_type,
                               block_len);
free_lstval:

        while(curallocval > allocval)
        {
            xfree(*--curallocval);
        }

        xfree(lstval);
    }
    else
    {
        const char *strval = tv_get_string_chk(&argvars[1]);

        if(strval == NULL)
        {
            return;
        }

        write_reg_contents_ex(regname,
                              (const uchar_kt *)strval,
                              ustrlen(strval),
                              append,
                              yank_type,
                              block_len);
    }

    rettv->vval.v_number = 0;
}

/// "settabvar()" function
static void f_settabvar(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = 0;

    if(check_restricted() || check_secure())
    {
        return;
    }

    tabpage_st *const tp =
        find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));

    const char *const varname = tv_get_string_chk(&argvars[1]);
    typval_st *const varp = &argvars[2];

    if(varname != NULL && tp != NULL)
    {
        tabpage_st *const save_curtab = curtab;
        goto_tabpage_tp(tp, false, false);

        const size_t varname_len = strlen(varname);
        char *const tabvarname = xmalloc(varname_len + 3);

        memcpy(tabvarname, "t:", 2);
        memcpy(tabvarname + 2, varname, varname_len + 1);
        set_var(tabvarname, varname_len + 2, varp, true);
        xfree(tabvarname);

        // Restore current tabpage.
        if(valid_tabpage(save_curtab))
        {
            goto_tabpage_tp(save_curtab, false, false);
        }
    }
}

/// "settabwinvar()" function
static void f_settabwinvar(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    setwinvar(argvars, rettv, 1);
}

/// "setwinvar()" function
static void f_setwinvar(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    setwinvar(argvars, rettv, 0);
}

/// "setwinvar()" and "settabwinvar()" functions
static void setwinvar(typval_st *argvars,
                      typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                      int off)
{
    if(check_restricted() || check_secure())
    {
        return;
    }

    tabpage_st *tp = NULL;

    if(off == 1)
    {
        tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
    }
    else
    {
        tp = curtab;
    }

    win_st *const win = find_win_by_nr(&argvars[off], tp);
    const char *varname = tv_get_string_chk(&argvars[off + 1]);
    typval_st *varp = &argvars[off + 2];

    if(win != NULL && varname != NULL && varp != NULL)
    {
        win_st *save_curwin;
        tabpage_st *save_curtab;
        bool need_switch_win = tp != curtab || win != curwin;

        if(!need_switch_win
           || switch_win(&save_curwin, &save_curtab, win, tp, true) == OK)
        {
            if(*varname == '&')
            {
                long numval;
                bool error = false;
                varname++;
                numval = tv_get_number_chk(varp, &error);
                char nbuf[NUMBUFLEN];
                const char *const strval = tv_get_string_buf_chk(varp, nbuf);

                if(!error && strval != NULL)
                {
                    set_option_value(varname, numval, strval, kOptSetLocal);
                }
            }
            else
            {
                const size_t varname_len = strlen(varname);
                char *const winvarname = xmalloc(varname_len + 3);

                memcpy(winvarname, "w:", 2);
                memcpy(winvarname + 2, varname, varname_len + 1);
                set_var(winvarname, varname_len + 2, varp, true);
                xfree(winvarname);
            }
        }

        if(need_switch_win)
        {
            restore_win(save_curwin, save_curtab, true);
        }
    }
}

/// f_sha256 - sha256({string}) function
static void f_sha256(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *p = tv_get_string(&argvars[0]);
    const char *hash = sha256_bytes((const uint8_t *)p, strlen(p), NULL, 0);

    // make a copy of the hash (sha256_bytes returns a static buffer)
    rettv->vval.v_string = (uchar_kt *)xstrdup(hash);
    rettv->v_type = kNvarString;
}

/// "shellescape({string})" function
static void f_shellescape(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_string =
        ustrdup_escape_shell((const uchar_kt *)tv_get_string(&argvars[0]),
                                non_zero_arg(&argvars[1]),
                                true);
    rettv->v_type = kNvarString;
}

/// shiftwidth() function
static void f_shiftwidth(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = get_sw_value(curbuf);
}

/// "simplify()" function
static void f_simplify(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const p = tv_get_string(&argvars[0]);
    rettv->vval.v_string = (uchar_kt *)xstrdup(p);
    simplify_filename(rettv->vval.v_string); // Simplify in place.
    rettv->v_type = kNvarString;
}

/// "sockconnect()" function
static void f_sockconnect(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[0].v_type != kNvarString || argvars[1].v_type != kNvarString)
    {
        EMSG(_(e_invarg));
        return;
    }

    if(argvars[2].v_type != kNvarDict && argvars[2].v_type != kNvarUnknown)
    {
        // Wrong argument types
        EMSG2(_(e_invarg2), "expected dictionary");
        return;
    }

    const char *mode = tv_get_string(&argvars[0]);
    const char *address = tv_get_string(&argvars[1]);
    bool tcp;

    if(strcmp(mode, "tcp") == 0)
    {
        tcp = true;
    }
    else if(strcmp(mode, "pipe") == 0)
    {
        tcp = false;
    }
    else
    {
        EMSG2(_(e_invarg2), "invalid mode");
        return;
    }

    bool rpc = false;

    if(argvars[2].v_type == kNvarDict)
    {
        dict_st *opts = argvars[2].vval.v_dict;
        rpc = tv_dict_get_number(opts, "rpc") != 0;
    }

    if(!rpc)
    {
        EMSG2(_(e_invarg2), "rpc option must be true");
        return;
    }

    const char *error = NULL;
    uint64_t id = channel_connect(tcp, address, 50, &error);

    if(error)
    {
        EMSG2(_("connection failed: %s"), error);
    }

    rettv->vval.v_number = (number_kt)id;
    rettv->v_type = kNvarNumber;
}

/// struct used in the array that's given to qsort()
typedef struct sort_item_s
{
    listitem_st *item;
    int idx;
} sort_item_st;

/// struct storing information about current sort
typedef struct sortinfo_s
{
    int item_compare_ic;
    bool item_compare_numeric;
    bool item_compare_numbers;
    bool item_compare_float;
    const char *item_compare_func;
    partial_st *item_compare_partial;
    dict_st *item_compare_selfdict;
    bool item_compare_func_err;
} sortinfo_st;
static sortinfo_st *sortinfo = NULL;

#define ITEM_COMPARE_FAIL 999

/// Compare functions for f_sort() and f_uniq() below.
static int item_compare(const void *s1,
                        const void *s2,
                        bool keep_zero)
{
    int res;
    sort_item_st *const si1 = (sort_item_st *)s1;
    sort_item_st *const si2 = (sort_item_st *)s2;
    typval_st *const tv1 = &si1->item->li_tv;
    typval_st *const tv2 = &si2->item->li_tv;

    if(sortinfo->item_compare_numbers)
    {
        const long v1 = tv_get_number(tv1);
        const long v2 = tv_get_number(tv2);
        res = v1 == v2 ? 0 : v1 > v2 ? 1 : -1;
        goto item_compare_end;
    }

    if(sortinfo->item_compare_float)
    {
        const float_kt v1 = tv_get_float(tv1);
        const float_kt v2 = tv_get_float(tv2);
        res = v1 == v2 ? 0 : v1 > v2 ? 1 : -1;
        goto item_compare_end;
    }

    char *p1;
    char *p2;
    char *tofree1 = NULL;
    char *tofree2 = NULL;

    // encode_tv2string() puts quotes around a string and allocates memory.
    // Don't do that for string variables. Use a single quote when comparing
    // with a non-string to do what the docs promise.
    if(tv1->v_type == kNvarString)
    {
        if(tv2->v_type != kNvarString || sortinfo->item_compare_numeric)
        {
            p1 = "'";
        }
        else
        {
            p1 = (char *)tv1->vval.v_string;
        }
    }
    else
    {
        tofree1 = p1 = encode_tv2string(tv1, NULL);
    }

    if(tv2->v_type == kNvarString)
    {
        if(tv1->v_type != kNvarString || sortinfo->item_compare_numeric)
        {
            p2 = "'";
        }
        else
        {
            p2 = (char *)tv2->vval.v_string;
        }
    }
    else
    {
        tofree2 = p2 = encode_tv2string(tv2, NULL);
    }

    if(p1 == NULL)
    {
        p1 = "";
    }

    if(p2 == NULL)
    {
        p2 = "";
    }

    if(!sortinfo->item_compare_numeric)
    {
        if(sortinfo->item_compare_ic)
        {
            res = ustricmp(p1, p2);
        }
        else
        {
            res = ustrcmp(p1, p2);
        }
    }
    else
    {
        double n1, n2;
        n1 = strtod(p1, &p1);
        n2 = strtod(p2, &p2);
        res = n1 == n2 ? 0 : n1 > n2 ? 1 : -1;
    }

    xfree(tofree1);
    xfree(tofree2);

item_compare_end:

    // When the result would be zero, compare the item indexes.
    // Makes the sort stable.
    if(res == 0 && !keep_zero)
    {
        res = si1->idx > si2->idx ? 1 : -1;
    }

    return res;
}

static int item_compare_keeping_zero(const void *s1, const void *s2)
{
    return item_compare(s1, s2, true);
}

static int item_compare_not_keeping_zero(const void *s1, const void *s2)
{
    return item_compare(s1, s2, false);
}

static int item_compare2(const void *s1, const void *s2, bool keep_zero)
{
    sort_item_st *si1,*si2;
    int res;
    typval_st rettv;
    typval_st argv[3];
    int dummy;
    const char *func_name;
    partial_st *partial = sortinfo->item_compare_partial;

    // shortcut after failure in previous call; compare all items equal
    if(sortinfo->item_compare_func_err)
    {
        return 0;
    }

    si1 = (sort_item_st *)s1;
    si2 = (sort_item_st *)s2;

    if(partial == NULL)
    {
        func_name = sortinfo->item_compare_func;
    }
    else
    {
        func_name = (const char *)partial_name(partial);
    }

    // Copy the values. This is needed to be able to set v_lock to kNvlVarFixed
    // in the copy without changing the original list items.
    tv_copy(&si1->item->li_tv, &argv[0]);
    tv_copy(&si2->item->li_tv, &argv[1]);

    rettv.v_type = kNvarUnknown; // tv_clear() uses this

    res = call_func((const uchar_kt *)func_name,
                    (int)ustrlen(func_name),
                    &rettv,
                    2,
                    argv,
                    NULL,
                    0L,
                    0L,
                    &dummy,
                    true,
                    partial,
                    sortinfo->item_compare_selfdict);

    tv_clear(&argv[0]);
    tv_clear(&argv[1]);

    if(res == FAIL)
    {
        res = ITEM_COMPARE_FAIL;
    }
    else
    {
        res = tv_get_number_chk(&rettv, &sortinfo->item_compare_func_err);
    }

    if(sortinfo->item_compare_func_err)
    {
        res = ITEM_COMPARE_FAIL; // return value has wrong type
    }

    tv_clear(&rettv);

    // When the result would be zero, compare the pointers themselves.
    // Makes the sort stable.
    if(res == 0 && !keep_zero)
    {
        res = si1->idx > si2->idx ? 1 : -1;
    }

    return res;
}

static int item_compare2_keeping_zero(const void *s1, const void *s2)
{
    return item_compare2(s1, s2, true);
}

static int item_compare2_not_keeping_zero(const void *s1, const void *s2)
{
    return item_compare2(s1, s2, false);
}

/// "sort({list})" function
static void do_sort_uniq(typval_st *argvars, typval_st *rettv, bool sort)
{
    list_st *l;
    listitem_st *li;
    sort_item_st *ptrs;
    long len;
    long i;

    // Pointer to current info struct used in compare function.
    // Save and restore the current one for nested calls.
    sortinfo_st info;
    sortinfo_st *old_sortinfo = sortinfo;
    sortinfo = &info;

    const char *const arg_errmsg = (sort
                                    ? N_("sort() argument")
                                    : N_("uniq() argument"));

    if(argvars[0].v_type != kNvarList)
    {
        EMSG2(_(e_listarg), sort ? "sort()" : "uniq()");
    }
    else
    {
        l = argvars[0].vval.v_list;

        if(l == NULL
           || tv_check_lock(l->lv_lock, arg_errmsg, TV_TRANSLATE))
        {
            goto theend;
        }

        rettv->vval.v_list = l;
        rettv->v_type = kNvarList;
        ++l->lv_refcount;
        len = tv_list_len(l);

        if(len <= 1)
        {
            goto theend; // short list sorts pretty quickly
        }

        info.item_compare_ic = false;
        info.item_compare_numeric = false;
        info.item_compare_numbers = false;
        info.item_compare_float = false;
        info.item_compare_func = NULL;
        info.item_compare_partial = NULL;
        info.item_compare_selfdict = NULL;

        if(argvars[1].v_type != kNvarUnknown)
        {
            // optional second argument: {func}
            if(argvars[1].v_type == kNvarUfunc)
            {
                info.item_compare_func = (const char *)argvars[1].vval.v_string;
            }
            else if(argvars[1].v_type == kNvarPartial)
            {
                info.item_compare_partial = argvars[1].vval.v_partial;
            }
            else
            {
                bool error = false;
                i = tv_get_number_chk(&argvars[1], &error);

                if(error)
                {
                    goto theend; // type error; errmsg already given
                }

                if(i == 1)
                {
                    info.item_compare_ic = true;
                }
                else if(argvars[1].v_type != kNvarNumber)
                {
                    info.item_compare_func = tv_get_string(&argvars[1]);
                }
                else if(i != 0)
                {
                    EMSG(_(e_invarg));
                    goto theend;
                }

                if(info.item_compare_func != NULL)
                {
                    if(*info.item_compare_func == NUL)
                    {
                        // empty string means default sort
                        info.item_compare_func = NULL;
                    }
                    else if(strcmp(info.item_compare_func, "n") == 0)
                    {
                        info.item_compare_func = NULL;
                        info.item_compare_numeric = true;
                    }
                    else if(strcmp(info.item_compare_func, "N") == 0)
                    {
                        info.item_compare_func = NULL;
                        info.item_compare_numbers = true;
                    }
                    else if(strcmp(info.item_compare_func, "f") == 0)
                    {
                        info.item_compare_func = NULL;
                        info.item_compare_float = true;
                    }
                    else if(strcmp(info.item_compare_func, "i") == 0)
                    {
                        info.item_compare_func = NULL;
                        info.item_compare_ic = true;
                    }
                }
            }

            if(argvars[2].v_type != kNvarUnknown)
            {
                // optional third argument: {dict}
                if(argvars[2].v_type != kNvarDict)
                {
                    EMSG(_(e_dictreq));
                    goto theend;
                }

                info.item_compare_selfdict = argvars[2].vval.v_dict;
            }
        }

        // Make an array with each entry pointing to an item in the List.
        ptrs = xmalloc((size_t)(len * sizeof (sort_item_st)));
        i = 0;

        if(sort)
        {
            // sort(): ptrs will be the list to sort.
            for(li = l->lv_first; li != NULL; li = li->li_next)
            {
                ptrs[i].item = li;
                ptrs[i].idx = i;
                i++;
            }

            info.item_compare_func_err = false;

            // Test the compare function.
            if((info.item_compare_func != NULL || info.item_compare_partial != NULL)
               && item_compare2_not_keeping_zero(&ptrs[0], &ptrs[1]) == ITEM_COMPARE_FAIL)
            {
                EMSG(_("E702: Sort compare function failed"));
            }
            else
            {
                // Sort the array with item pointers.
                qsort(ptrs,
                      (size_t)len,
                      sizeof(sort_item_st),
                      (info.item_compare_func == NULL
                       && info.item_compare_partial == NULL
                       ? item_compare_not_keeping_zero
                       : item_compare2_not_keeping_zero));

                if(!info.item_compare_func_err)
                {
                    // Clear the list and append the items in the sorted order.
                    l->lv_first = NULL;
                    l->lv_last = NULL;
                    l->lv_idx_item = NULL;
                    l->lv_len = 0;

                    for(i = 0; i < len; i++)
                    {
                        tv_list_append(l, ptrs[i].item);
                    }
                }
            }
        }
        else
        {
            int(*item_compare_func_ptr)(const void *, const void *);
            // f_uniq(): ptrs will be a stack of items to remove.
            info.item_compare_func_err = false;

            if(info.item_compare_func != NULL
               || info.item_compare_partial != NULL)
            {
                item_compare_func_ptr = item_compare2_keeping_zero;
            }
            else
            {
                item_compare_func_ptr = item_compare_keeping_zero;
            }

            for(li = l->lv_first; li != NULL && li->li_next != NULL; li = li->li_next)
            {
                if(item_compare_func_ptr(&li, &li->li_next) == 0)
                {
                    ptrs[i++].item = li;
                }

                if(info.item_compare_func_err)
                {
                    EMSG(_("E882: Uniq compare function failed"));
                    break;
                }
            }

            if(!info.item_compare_func_err)
            {
                while(--i >= 0)
                {
                    assert(ptrs[i].item->li_next);
                    li = ptrs[i].item->li_next;
                    ptrs[i].item->li_next = li->li_next;

                    if(li->li_next != NULL)
                    {
                        li->li_next->li_prev = ptrs[i].item;
                    }
                    else
                    {
                        l->lv_last = ptrs[i].item;
                    }

                    tv_list_watch_fix(l, li);
                    tv_list_item_free(li);
                    l->lv_len--;
                }
            }
        }

        xfree(ptrs);
    }

theend:

    sortinfo = old_sortinfo;
}

/// "sort"({list})" function
static void f_sort(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    do_sort_uniq(argvars, rettv, true);
}

/// "uniq({list})" function
static void f_uniq(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    do_sort_uniq(argvars, rettv, false);
}

//
// "reltimefloat()" function
//
static void f_reltimefloat(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
FUNC_ATTR_NONNULL_ALL
{
    proftime_kt tm;

    rettv->v_type = kNvarFloat;
    rettv->vval.v_float = 0;

    if(list2proftime(&argvars[0], &tm) == OK)
    {
        rettv->vval.v_float = ((float_kt)tm) / 1000000000;
    }
}

/// "soundfold({word})" function
static void f_soundfold(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    const char *const s = tv_get_string(&argvars[0]);
    rettv->vval.v_string = (uchar_kt *)eval_soundfold(s);
}

/// "spellbadword()" function
static void f_spellbadword(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *word = "";
    hlf_et attr = HLF_COUNT;
    size_t len = 0;
    tv_list_alloc_ret(rettv);

    if(argvars[0].v_type == kNvarUnknown)
    {
        // Find the start and length of the badly spelled word.
        len = spell_move_to(curwin, FORWARD, true, true, &attr);

        if(len != 0)
        {
            word = (char *)get_cursor_pos_ptr();
        }
    }
    else if(curwin->w_o_curbuf.wo_spell && *curbuf->b_s.b_p_spl != NUL)
    {
        const char *str = tv_get_string_chk(&argvars[0]);
        int capcol = -1;

        if(str != NULL)
        {
            // Check the argument for spelling.
            while(*str != NUL)
            {
                len = spell_check(curwin, (uchar_kt *)str, &attr, &capcol, false);

                if(attr != HLF_COUNT)
                {
                    word = str;
                    break;
                }

                str += len;
            }
        }
    }

    assert(len <= INT_MAX);

    tv_list_append_string(rettv->vval.v_list, word, len);

    tv_list_append_string(rettv->vval.v_list,
                          (attr == HLF_SPB
                           ? "bad" : attr == HLF_SPR
                           ? "rare" : attr == HLF_SPL
                           ? "local" : attr == HLF_SPC
                           ? "caps" : NULL), -1);
}

/// "spellsuggest()" function
static void f_spellsuggest(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    bool typeerr = false;
    int maxcount;
    garray_st ga;
    listitem_st *li;
    bool need_capital = false;
    tv_list_alloc_ret(rettv);

    if(curwin->w_o_curbuf.wo_spell && *curwin->w_s->b_p_spl != NUL)
    {
        const char *const str = tv_get_string(&argvars[0]);

        if(argvars[1].v_type != kNvarUnknown)
        {
            maxcount = tv_get_number_chk(&argvars[1], &typeerr);

            if(maxcount <= 0)
            {
                return;
            }

            if(argvars[2].v_type != kNvarUnknown)
            {
                need_capital = tv_get_number_chk(&argvars[2], &typeerr);

                if(typeerr)
                {
                    return;
                }
            }
        }
        else
        {
            maxcount = 25;
        }

        spell_suggest_list(&ga, (uchar_kt *)str, maxcount, need_capital, false);

        for(int i = 0; i < ga.ga_len; i++)
        {
            char *p = ((char **)ga.ga_data)[i];
            li = tv_list_item_alloc();
            li->li_tv.v_type = kNvarString;
            li->li_tv.v_lock = 0;
            li->li_tv.vval.v_string = (uchar_kt *)p;
            tv_list_append(rettv->vval.v_list, li);
        }

        ga_clear(&ga);
    }
}

static void f_split(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    regmatch_st regmatch;
    uchar_kt *save_cpo;
    int match;
    columnum_kt col = 0;
    bool keepempty = false;
    bool typeerr = false;

    // Make 'cpoptions' empty, the 'l'
    // flag should not be used here.
    save_cpo = p_cpo;
    p_cpo = (uchar_kt *)"";
    const char *str = tv_get_string(&argvars[0]);
    const char *pat = NULL;
    char patbuf[NUMBUFLEN];

    if(argvars[1].v_type != kNvarUnknown)
    {
        pat = tv_get_string_buf_chk(&argvars[1], patbuf);

        if(pat == NULL)
        {
            typeerr = true;
        }

        if(argvars[2].v_type != kNvarUnknown)
        {
            keepempty = (bool)tv_get_number_chk(&argvars[2], &typeerr);
        }
    }

    if(pat == NULL || *pat == NUL)
    {
        pat = "[\\x01- ]\\+";
    }

    tv_list_alloc_ret(rettv);

    if(typeerr)
    {
        return;
    }

    regmatch.regprog = regexp_compile((uchar_kt *)pat, RE_MAGIC + RE_STRING);

    if(regmatch.regprog != NULL)
    {
        regmatch.rm_ic = FALSE;

        while(*str != NUL || keepempty)
        {
            if(*str == NUL)
            {
                match = false; // Empty item at the end.
            }
            else
            {
                match = vim_regexec_nl(&regmatch, (uchar_kt *)str, col);
            }

            const char *end;

            if(match)
            {
                end = (const char *)regmatch.startp[0];
            }
            else
            {
                end = str + strlen(str);
            }

            if(keepempty
               || end > str
               || (rettv->vval.v_list->lv_len > 0
                   && *str != NUL
                   && match
                   && end < (const char *)regmatch.endp[0]))
            {
                tv_list_append_string(rettv->vval.v_list, str, end - str);
            }

            if(!match)
            {
                break;
            }

            // Advance to just after the match.
            if(regmatch.endp[0] > (uchar_kt *)str)
            {
                col = 0;
            }
            else
            {
                // Don't get stuck at the same match.
                col = (*mb_ptr2len)(regmatch.endp[0]);
            }

            str = (const char *)regmatch.endp[0];
        }

        vim_regfree(regmatch.regprog);
    }

    p_cpo = save_cpo;
}

/// "str2float()" function
static void f_str2float(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    uchar_kt *p = skipwhite((const uchar_kt *)tv_get_string(&argvars[0]));

    if(*p == '+')
    {
        p = skipwhite(p + 1);
    }

    (void)string2float((char *)p, &rettv->vval.v_float);
    rettv->v_type = kNvarFloat;
}

/// "str2nr()" function
static void f_str2nr(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int base = 10;
    long n;
    int what;

    if(argvars[1].v_type != kNvarUnknown)
    {
        base = tv_get_number(&argvars[1]);

        if(base != 2 && base != 8 && base != 10 && base != 16)
        {
            EMSG(_(e_invarg));
            return;
        }
    }

    uchar_kt *p = skipwhite((const uchar_kt *)tv_get_string(&argvars[0]));

    if(*p == '+')
    {
        p = skipwhite(p + 1);
    }

    switch(base)
    {
        case 2:
        {
            what = kStrToNumBin | kStrToNumOne;
            break;
        }

        case 8:
        {
            what = kStrToNumOct | kStrToNumOne;
            break;
        }

        case 16:
        {
            what = kStrToNumHex | kStrToNumOne;
            break;
        }

        default:
        {
            what = 0;
        }
    }

    str_to_num(p, NULL, NULL, what, &n, NULL, 0);
    rettv->vval.v_number = n;
}

/// "strftime({format}[, {time}])" function
static void f_strftime(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    time_t seconds;
    rettv->v_type = kNvarString;
    char *p = (char *)tv_get_string(&argvars[0]);

    if(argvars[1].v_type == kNvarUnknown)
    {
        seconds = time(NULL);
    }
    else
    {
        seconds = (time_t)tv_get_number(&argvars[1]);
    }

    struct tm curtime;

    struct tm *curtime_ptr = os_localtime_r(&seconds, &curtime);

    // MSVC returns NULL for an invalid value of seconds.
    if(curtime_ptr == NULL)
    {
        rettv->vval.v_string = ustrdup((uchar_kt *)_("(Invalid)"));
    }
    else
    {
        vimconv_st conv;
        uchar_kt *enc;
        conv.vc_type = CONV_NONE;
        enc = enc_locale();
        convert_setup(&conv, p_enc, enc);

        if(conv.vc_type != CONV_NONE)
        {
            p = (char *)string_convert(&conv, (uchar_kt *)p, NULL);
        }

        char result_buf[256];

        if(p != NULL)
        {
            (void)strftime(result_buf, sizeof(result_buf), p, curtime_ptr);
        }
        else
        {
            result_buf[0] = NUL;
        }

        if(conv.vc_type != CONV_NONE)
        {
            xfree(p);
        }

        convert_setup(&conv, enc, p_enc);

        if(conv.vc_type != CONV_NONE)
        {
            rettv->vval.v_string =
                string_convert(&conv, (uchar_kt *)result_buf, NULL);
        }
        else
        {
            rettv->vval.v_string = (uchar_kt *)xstrdup(result_buf);
        }

        // Release conversion descriptors.
        convert_setup(&conv, NULL, NULL);
        xfree(enc);
    }
}

/// "strgetchar()" function
static void f_strgetchar(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = -1;
    const char *const str = tv_get_string_chk(&argvars[0]);

    if(str == NULL)
    {
        return;
    }

    bool error = false;
    number_kt charidx = tv_get_number_chk(&argvars[1], &error);

    if(error)
    {
        return;
    }

    const size_t len = ustrlen(str);
    size_t byteidx = 0;

    while(charidx >= 0 && byteidx < len)
    {
        if(charidx == 0)
        {
            rettv->vval.v_number = mb_ptr2char((const uchar_kt *)str + byteidx);
            break;
        }

        charidx--;
        byteidx += mb_cptr2len((const uchar_kt *)str + byteidx);
    }
}

/// "stridx()" function
static void f_stridx(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char buf[NUMBUFLEN];
    rettv->vval.v_number = -1;
    const char *const needle = tv_get_string_chk(&argvars[1]);
    const char *haystack = tv_get_string_buf_chk(&argvars[0], buf);
    const char *const haystack_start = haystack;

    if(needle == NULL || haystack == NULL)
    {
        return; // Type error; errmsg already given.
    }

    if(argvars[2].v_type != kNvarUnknown)
    {
        bool error = false;
        const ptrdiff_t start_idx =
                (ptrdiff_t)tv_get_number_chk(&argvars[2], &error);

        if(error || start_idx >= (ptrdiff_t)strlen(haystack))
        {
            return;
        }

        if(start_idx >= 0)
        {
            haystack += start_idx;
        }
    }

    const char *pos = strstr(haystack, needle);

    if(pos != NULL)
    {
        rettv->vval.v_number = (number_kt)(pos - haystack_start);
    }
}

/// "string()" function
static void f_string(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = (uchar_kt *) encode_tv2string(&argvars[0], NULL);
}

/// "strlen()" function
static void f_strlen(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = (number_kt)strlen(tv_get_string(&argvars[0]));
}

/// "strchars()" function
static void f_strchars(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *s = tv_get_string(&argvars[0]);
    int skipcc = 0;
    number_kt len = 0;
    int (*func_mb_ptr2char_adv)(const uchar_kt **pp);

    if(argvars[1].v_type != kNvarUnknown)
    {
        skipcc = tv_get_number_chk(&argvars[1], NULL);
    }

    if(skipcc < 0 || skipcc > 1)
    {
        EMSG(_(e_invarg));
    }
    else
    {
        func_mb_ptr2char_adv = skipcc ? mb_ptr2char_adv : mb_cptr2char_adv;

        while(*s != NUL)
        {
            func_mb_ptr2char_adv((const uchar_kt **)&s);
            len++;
        }

        rettv->vval.v_number = len;
    }
}

/// "strdisplaywidth()" function
static void f_strdisplaywidth(typval_st *argvars,
                              typval_st *rettv,
                              func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const s = tv_get_string(&argvars[0]);
    int col = 0;

    if(argvars[1].v_type != kNvarUnknown)
    {
        col = tv_get_number(&argvars[1]);
    }

    rettv->vval.v_number =
        (number_kt)(linetabsize_col(col, (uchar_kt *)s) - col);
}

/// "strwidth()" function
static void f_strwidth(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const s = tv_get_string(&argvars[0]);
    rettv->vval.v_number = (number_kt)mb_string2cells((const uchar_kt *)s);
}

/// "strcharpart()" function
static void f_strcharpart(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const p = tv_get_string(&argvars[0]);
    const size_t slen = ustrlen(p);
    int nbyte = 0;
    bool error = false;
    number_kt nchar = tv_get_number_chk(&argvars[1], &error);

    if(!error)
    {
        if(nchar > 0)
        {
            while(nchar > 0 && (size_t)nbyte < slen)
            {
                nbyte += mb_cptr2len((const uchar_kt *)p + nbyte);
                nchar--;
            }
        }
        else
        {
            nbyte = nchar;
        }
    }

    int len = 0;

    if(argvars[2].v_type != kNvarUnknown)
    {
        int charlen = tv_get_number(&argvars[2]);

        while(charlen > 0 && nbyte + len < (int)slen)
        {
            int off = nbyte + len;

            if(off < 0)
            {
                len += 1;
            }
            else
            {
                len += (size_t)mb_cptr2len((const uchar_kt *)p + off);
            }

            charlen--;
        }
    }
    else
    {
        len = slen - nbyte; // default: all bytes that are available.
    }

    // Only return the overlap between the specified
    // part and the actual string.
    if(nbyte < 0)
    {
        len += nbyte;
        nbyte = 0;
    }
    else if((size_t)nbyte > slen)
    {
        nbyte = slen;
    }

    if(len < 0)
    {
        len = 0;
    }
    else if(nbyte + len > (int)slen)
    {
        len = slen - nbyte;
    }

    rettv->v_type = kNvarString;
    rettv->vval.v_string = (uchar_kt *)xstrndup(p + nbyte, (size_t)len);
}

/// "strpart()" function
static void f_strpart(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    bool error = false;
    const char *const p = tv_get_string(&argvars[0]);
    const size_t slen = strlen(p);
    number_kt n = tv_get_number_chk(&argvars[1], &error);
    number_kt len;

    if(error)
    {
        len = 0;
    }
    else if(argvars[2].v_type != kNvarUnknown)
    {
        len = tv_get_number(&argvars[2]);
    }
    else
    {
        len = slen - n; // Default len: all bytes that are available.
    }

    // Only return the overlap between the
    // specified part and the actual string.
    if(n < 0)
    {
        len += n;
        n = 0;
    }
    else if(n > (number_kt)slen)
    {
        n = slen;
    }

    if(len < 0)
    {
        len = 0;
    }
    else if(n + len > (number_kt)slen)
    {
        len = slen - n;
    }

    rettv->v_type = kNvarString;
    rettv->vval.v_string = (uchar_kt *)xmemdupz(p + n, (size_t)len);
}

/// "strridx()" function
static void f_strridx(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char buf[NUMBUFLEN];
    const char *const needle = tv_get_string_chk(&argvars[1]);
    const char *const haystack = tv_get_string_buf_chk(&argvars[0], buf);
    rettv->vval.v_number = -1;

    if(needle == NULL || haystack == NULL)
    {
        return; // Type error; errmsg already given.
    }

    const size_t haystack_len = ustrlen(haystack);
    ptrdiff_t end_idx;

    if(argvars[2].v_type != kNvarUnknown)
    {
        // Third argument: upper limit for index.
        end_idx = (ptrdiff_t)tv_get_number_chk(&argvars[2], NULL);

        if(end_idx < 0)
        {
            return; // Can never find a match.
        }
    }
    else
    {
        end_idx = (ptrdiff_t)haystack_len;
    }

    const char *lastmatch = NULL;

    if(*needle == NUL)
    {
        // Empty string matches past the end.
        lastmatch = haystack + end_idx;
    }
    else
    {
        for(const char *rest = haystack; *rest != NUL; rest++)
        {
            rest = strstr(rest, needle);

            if(rest == NULL || rest > haystack + end_idx)
            {
                break;
            }

            lastmatch = rest;
        }
    }

    if(lastmatch == NULL)
    {
        rettv->vval.v_number = -1;
    }
    else
    {
        rettv->vval.v_number = (number_kt)(lastmatch - haystack);
    }
}

/// "strtrans()" function
static void f_strtrans(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = transstr((uchar_kt *)tv_get_string(&argvars[0]));
}

/// "submatch()" function
static void f_submatch(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    bool error = false;
    int no = (int)tv_get_number_chk(&argvars[0], &error);

    if(error)
    {
        return;
    }

    if(no < 0 || no >= NSUBEXP)
    {
        EMSGN(_("E935: invalid submatch number: %d"), no);
        return;
    }

    int retList = 0;

    if(argvars[1].v_type != kNvarUnknown)
    {
        retList = tv_get_number_chk(&argvars[1], &error);

        if(error)
        {
            return;
        }
    }

    if(retList == 0)
    {
        rettv->v_type = kNvarString;
        rettv->vval.v_string = reg_submatch(no);
    }
    else
    {
        rettv->v_type = kNvarList;
        rettv->vval.v_list = reg_submatch_list(no);
    }
}

/// "substitute()" function
static void f_substitute(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char patbuf[NUMBUFLEN];
    char subbuf[NUMBUFLEN];
    char flagsbuf[NUMBUFLEN];
    const char *const str = tv_get_string_chk(&argvars[0]);
    const char *const pat = tv_get_string_buf_chk(&argvars[1], patbuf);
    const char *sub = NULL;
    const char *const flg = tv_get_string_buf_chk(&argvars[3], flagsbuf);
    typval_st *expr = NULL;

    if(tv_is_func(argvars[2]))
    {
        expr = &argvars[2];
    }
    else
    {
        sub = tv_get_string_buf_chk(&argvars[2], subbuf);
    }

    rettv->v_type = kNvarString;

    if(str == NULL
       || pat == NULL
       || (sub == NULL && expr == NULL)
       || flg == NULL)
    {
        rettv->vval.v_string = NULL;
    }
    else
    {
        rettv->vval.v_string =
            do_string_sub((uchar_kt *)str,
                          (uchar_kt *)pat,
                          (uchar_kt *)sub,
                          expr,
                          (uchar_kt *)flg);
    }
}

/// "synID(lnum, col, trans)" function
static void f_synID(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    // -1 on type error (both)
    const linenum_kt lnum = tv_get_lnum(argvars);
    const columnum_kt col = (columnum_kt)tv_get_number(&argvars[1]) - 1;
    bool transerr = false;
    const int trans = tv_get_number_chk(&argvars[2], &transerr);
    int id = 0;

    if(!transerr
       && lnum >= 1
       && lnum <= curbuf->b_ml.ml_line_count
       && col >= 0
       && (size_t)col < ustrlen(ml_get(lnum)))
    {
        id = syn_get_id(curwin, lnum, col, trans, NULL, false);
    }

    rettv->vval.v_number = id;
}

/// "synIDattr(id, what [, mode])" function
static void f_synIDattr(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const int id = (int)tv_get_number(&argvars[0]);
    const char *const what = tv_get_string(&argvars[1]);
    int modec;

    if(argvars[2].v_type != kNvarUnknown)
    {
        char modebuf[NUMBUFLEN];
        const char *const mode = tv_get_string_buf(&argvars[2], modebuf);
        modec = TOLOWER_ASC(mode[0]);

        if(modec != 'c' && modec != 'g')
        {
            modec = 0; // Replace invalid with current.
        }
    }
    else if(ui_rgb_attached())
    {
        modec = 'g';
    }
    else
    {
        modec = 'c';
    }

    const char *p = NULL;

    switch(TOLOWER_ASC(what[0]))
    {
        case 'b':
        {
            if(TOLOWER_ASC(what[1]) == 'g') // bg[#]
            {
                p = highlight_color(id, what, modec);
            }
            else  // bold
            {
                p = highlight_has_attr(id, HL_BOLD, modec);
            }

            break;
        }

        case 'f': // fg[#] or font
        {
            p = highlight_color(id, what, modec);
            break;
        }

        case 'i':
        {
            if(TOLOWER_ASC(what[1]) == 'n') // inverse
            {
                p = highlight_has_attr(id, HL_INVERSE, modec);
            }
            else // italic
            {
                p = highlight_has_attr(id, HL_ITALIC, modec);
            }

            break;
        }

        case 'n': // name
        {
            p = get_highlight_name(NULL, id - 1);
            break;
        }

        case 'r': // reverse
        {
            p = highlight_has_attr(id, HL_INVERSE, modec);
            break;
        }

        case 's':
        {
            if(TOLOWER_ASC(what[1]) == 'p') // sp[#]
            {
                p = highlight_color(id, what, modec);
            }
            else // standout
            {
                p = highlight_has_attr(id, HL_STANDOUT, modec);
            }

            break;
        }

        case 'u':
        {
            if(ustrlen(what) <= 5 || TOLOWER_ASC(what[5]) != 'c') // underline
            {
                p = highlight_has_attr(id, HL_UNDERLINE, modec);
            }
            else // undercurl
            {
                p = highlight_has_attr(id, HL_UNDERCURL, modec);
            }

            break;
        }
    }

    rettv->v_type = kNvarString;
    rettv->vval.v_string = (uchar_kt *)(p == NULL ? p : xstrdup(p));
}

/// "synIDtrans(id)" function
static void f_synIDtrans(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int id = tv_get_number(&argvars[0]);

    if(id > 0)
    {
        id = syn_get_final_id(id);
    }
    else
    {
        id = 0;
    }

    rettv->vval.v_number = id;
}

/// "synconcealed(lnum, col)" function
static void f_synconcealed(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int syntax_flags = 0;
    int cchar;
    int matchid = 0;
    uchar_kt str[NUMBUFLEN];
    rettv->v_type = kNvarList;
    rettv->vval.v_list = NULL;

    // -1 on type error (both)
    const linenum_kt lnum = tv_get_lnum(argvars);
    const columnum_kt col = (columnum_kt)tv_get_number(&argvars[1]) - 1;
    memset(str, NUL, sizeof(str));
    tv_list_alloc_ret(rettv);

    if(lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count && col >= 0
       && (size_t)col <= ustrlen(ml_get(lnum)) && curwin->w_o_curbuf.wo_cole > 0)
    {
        (void)syn_get_id(curwin, lnum, col, false, NULL, false);
        syntax_flags = get_syntax_info(&matchid);

        // get the conceal character
        if((syntax_flags & HL_CONCEAL) && curwin->w_o_curbuf.wo_cole < 3)
        {
            cchar = syn_get_sub_char();

            if(cchar == NUL
               && curwin->w_o_curbuf.wo_cole == 1
               && lcs_conceal != NUL)
            {
                cchar = lcs_conceal;
            }

            if(cchar != NUL)
            {
                (*mb_char2bytes)(cchar, str);
            }
        }
    }

    tv_list_append_number(rettv->vval.v_list, (syntax_flags & HL_CONCEAL) != 0);

    // -1 to auto-determine strlen
    tv_list_append_string(rettv->vval.v_list, (const char *)str, -1);
    tv_list_append_number(rettv->vval.v_list, matchid);
}

/// "synstack(lnum, col)" function
static void f_synstack(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarList;
    rettv->vval.v_list = NULL;

    // -1 on type error (both)
    const linenum_kt lnum = tv_get_lnum(argvars);
    const columnum_kt col = (columnum_kt)tv_get_number(&argvars[1]) - 1;

    if(lnum >= 1
       && lnum <= curbuf->b_ml.ml_line_count
       && col >= 0
       && (size_t)col <= ustrlen(ml_get(lnum)))
    {
        tv_list_alloc_ret(rettv);
        (void)syn_get_id(curwin, lnum, col, false, NULL, true);
        int id;
        int i = 0;

        while((id = syn_get_stack_item(i++)) >= 0)
        {
            tv_list_append_number(rettv->vval.v_list, id);
        }
    }
}

static list_st *string_to_list(const char *str, size_t len, const bool keepempty)
{
    if(!keepempty && str[len - 1] == NL)
    {
        len--;
    }

    list_st *const list = tv_list_alloc();
    encode_list_write(list, str, len);

    return list;
}

static void get_system_output_as_rettv(typval_st *argvars,
                                       typval_st *rettv,
                                       bool retlist)
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    if(check_restricted() || check_secure())
    {
        return;
    }

    // get input to the shell command (if any), and its length
    ptrdiff_t input_len;
    char *input = save_tv_as_string(&argvars[1], &input_len, false);

    if(input_len < 0)
    {
        assert(input == NULL);
        return;
    }

    // get shell command to execute
    bool executable = true;
    char **argv = tv_to_argv(&argvars[0], NULL, &executable);

    if(!argv)
    {
        if(!executable)
        {
            set_vim_var_nr(VV_SHELL_ERROR, (long)-1);
        }

        xfree(input);
        return; // Already did emsg.
    }

    // execute the command
    size_t nread = 0;
    char *res = NULL;
    int status = os_system(argv, input, input_len, &res, &nread);
    xfree(input);
    set_vim_var_nr(VV_SHELL_ERROR, (long) status);

    if(res == NULL)
    {
        if(retlist)
        {
            // return an empty list when there's no output
            tv_list_alloc_ret(rettv);
        }
        else
        {
            rettv->vval.v_string = (uchar_kt *) xstrdup("");
        }

        return;
    }

    if(retlist)
    {
        int keepempty = 0;

        if(argvars[1].v_type != kNvarUnknown
           && argvars[2].v_type != kNvarUnknown)
        {
            keepempty = tv_get_number(&argvars[2]);
        }

        rettv->vval.v_list = string_to_list(res, nread, (bool)keepempty);
        rettv->vval.v_list->lv_refcount++;
        rettv->v_type = kNvarList;
        xfree(res);
    }
    else
    {
        // res may contain several NULs before the final terminating one.
        // Replace them with SOH (1) like in get_cmd_output() to avoid truncation.
        xmemchrsub(res, NUL, 1, nread);

#ifdef USE_CRNL
        // translate <CR><NL> into <NL>
        char *d = res;
        for(char *s = res; *s; ++s)
        {
            if(s[0] == CAR && s[1] == NL)
            {
                ++s;
            }

            *d++ = *s;
        }

        *d = NUL;
#endif
        rettv->vval.v_string = (uchar_kt *) res;
    }
}

/// f_system - the VimL system() function
static void f_system(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    get_system_output_as_rettv(argvars, rettv, false);
}

static void f_systemlist(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    get_system_output_as_rettv(argvars, rettv, true);
}

/// "tabpagebuflist()" function
static void f_tabpagebuflist(typval_st *argvars,
                             typval_st *rettv,
                             func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    win_st *wp = NULL;

    if(argvars[0].v_type == kNvarUnknown)
    {
        wp = firstwin;
    }
    else
    {
        tabpage_st *const tp = find_tabpage((int)tv_get_number(&argvars[0]));

        if(tp != NULL)
        {
            wp = (tp == curtab) ? firstwin : tp->tp_firstwin;
        }
    }

    if(wp != NULL)
    {
        tv_list_alloc_ret(rettv);

        while(wp != NULL)
        {
            tv_list_append_number(rettv->vval.v_list, wp->w_buffer->b_id);
            wp = wp->w_next;
        }
    }
}

/// "tabpagenr()" function
static void f_tabpagenr(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int nr = 1;

    if(argvars[0].v_type != kNvarUnknown)
    {
        const char *const arg = tv_get_string_chk(&argvars[0]);
        nr = 0;

        if(arg != NULL)
        {
            if(strcmp(arg, "$") == 0)
            {
                nr = tabpage_index(NULL) - 1;
            }
            else
            {
                EMSG2(_(e_invexpr2), arg);
            }
        }
    }
    else
    {
        nr = tabpage_index(curtab);
    }

    rettv->vval.v_number = nr;
}

/// Common code for tabpagewinnr() and winnr().
static int get_winnr(tabpage_st *tp, typval_st *argvar)
{
    win_st *twin;
    int nr = 1;
    win_st *wp;
    twin = (tp == curtab) ? curwin : tp->tp_curwin;

    if(argvar->v_type != kNvarUnknown)
    {
        const char *const arg = tv_get_string_chk(argvar);

        if(arg == NULL)
        {
            nr = 0; // Type error; errmsg already given.
        }
        else if(strcmp(arg, "$") == 0)
        {
            twin = (tp == curtab) ? lastwin : tp->tp_lastwin;
        }
        else if(strcmp(arg, "#") == 0)
        {
            twin = (tp == curtab) ? prevwin : tp->tp_prevwin;

            if(twin == NULL)
            {
                nr = 0;
            }
        }
        else
        {
            EMSG2(_(e_invexpr2), arg);
            nr = 0;
        }
    }

    if(nr > 0)
    {
        for(wp = (tp == curtab) ? firstwin : tp->tp_firstwin;
            wp != twin;
            wp = wp->w_next)
        {
            if(wp == NULL)
            {
                // didn't find it in this tabpage
                nr = 0;
                break;
            }

            ++nr;
        }
    }

    return nr;
}

/// "tabpagewinnr()" function
static void f_tabpagewinnr(typval_st *argvars,
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int nr = 1;
    tabpage_st *const tp = find_tabpage((int)tv_get_number(&argvars[0]));

    if(tp == NULL)
    {
        nr = 0;
    }
    else
    {
        nr = get_winnr(tp, &argvars[1]);
    }

    rettv->vval.v_number = nr;
}


/// "tagfiles()" function
static void f_tagfiles(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char *fname;
    tagname_st tn;
    tv_list_alloc_ret(rettv);
    fname = xmalloc(MAXPATHL);
    bool first = true;

    while(get_tagfname(&tn, first, (uchar_kt *)fname) == OK)
    {
        tv_list_append_string(rettv->vval.v_list, fname, -1);
        first = false;
    }

    tagname_free(&tn);
    xfree(fname);
}

/// "taglist()" function
static void f_taglist(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const char *const tag_pattern = tv_get_string(&argvars[0]);
    rettv->vval.v_number = false;

    if(*tag_pattern == NUL)
    {
        return;
    }

    const char *fname = NULL;

    if(argvars[1].v_type != kNvarUnknown)
    {
        fname = tv_get_string(&argvars[1]);
    }

    (void)get_tags(tv_list_alloc_ret(rettv),
                   (uchar_kt *)tag_pattern,
                   (uchar_kt *)fname);
}

/// "tempname()" function
static void f_tempname(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    rettv->vval.v_string = vim_tempname();
}

/// "termopen(cmd[, cwd])" function
static void f_termopen(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(check_restricted() || check_secure())
    {
        return;
    }

    if(curbuf->b_changed)
    {
        EMSG(_("Can only call this function in an unmodified buffer"));
        return;
    }

    const char *cmd;
    bool executable = true;
    char **argv = tv_to_argv(&argvars[0], &cmd, &executable);

    if(!argv)
    {
        rettv->vval.v_number = executable ? 0 : -1;
        return; // Did error message in tv_to_argv.
    }

    if(argvars[1].v_type != kNvarDict
       && argvars[1].v_type != kNvarUnknown)
    {
        // Wrong argument type
        EMSG2(_(e_invarg2), "expected dictionary");
        shell_free_argv(argv);
        return;
    }

    callback_st on_stdout = CALLBACK_NONE;
    callback_st on_stderr = CALLBACK_NONE;
    callback_st on_exit = CALLBACK_NONE;
    dict_st *job_opts = NULL;
    const char *cwd = ".";

    if(argvars[1].v_type == kNvarDict)
    {
        job_opts = argvars[1].vval.v_dict;
        const char *const new_cwd = tv_dict_get_string(job_opts, "cwd", false);

        if(new_cwd && *new_cwd != NUL)
        {
            cwd = new_cwd;

            // The new cwd must be a directory.
            if(!os_isdir((const uchar_kt *)cwd))
            {
                EMSG2(_(e_invarg2), "expected valid directory");
                shell_free_argv(argv);
                return;
            }
        }

        if(!common_job_callbacks(job_opts, &on_stdout, &on_stderr, &on_exit))
        {
            shell_free_argv(argv);
            return;
        }
    }

    terminal_jobdata_st *data = common_job_init(argv,
                                            on_stdout,
                                            on_stderr,
                                            on_exit,
                                            true,
                                            false,
                                            false,
                                            cwd);

    data->proc.pty.width = curwin->w_width;
    data->proc.pty.height = curwin->w_height;
    data->proc.pty.term_name = xstrdup("xterm-256color");

    if(!common_job_start(data, rettv))
    {
        return;
    }

    terminal_opt_st topts;

    topts.data = data;
    topts.width = curwin->w_width;
    topts.height = curwin->w_height;
    topts.write_cb = term_write;
    topts.resize_cb = term_resize;
    topts.close_cb = term_close;

    int pid = data->proc.pty.process.pid;
    char buf[1024];

    // format the title with the pid to conform with the term:// URI
    snprintf(buf, sizeof(buf), "term://%s//%d:%s", cwd, pid, cmd);

    // at this point the buffer has no terminal instance associated yet, so unset
    // the 'swapfile' option to ensure no swap file will be created
    curbuf->b_p_swf = false;
    (void)setfname(curbuf, (uchar_kt *)buf, NULL, true);

    // Save the job id and pid in b:terminal_job_{id,pid}
    error_st err = ERROR_INIT;

    dict_set_var(curbuf->b_vars,
                 cstr_as_string("terminal_job_id"),
                 INTEGER_OBJ(rettv->vval.v_number),
                 false,
                 false,
                 &err);

    api_clear_error(&err);

    dict_set_var(curbuf->b_vars,
                 cstr_as_string("terminal_job_pid"),
                 INTEGER_OBJ(pid),
                 false,
                 false,
                 &err);

    api_clear_error(&err);
    terminal_st *term = terminal_open(topts);
    data->term = term;
    data->refcount++;
    return;
}

/// "test_garbagecollect_now()" function
static void f_test_garbagecollect_now(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                                      typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    // This is dangerous, any Lists and Dicts used
    // internally may be freed while still in use.
    garbage_collect(true);
}

bool callback_from_typval(callback_st *const callback, typval_st *const arg)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(arg->v_type == kNvarPartial && arg->vval.v_partial != NULL)
    {
        callback->data.partial = arg->vval.v_partial;
        callback->data.partial->pt_refcount++;
        callback->type = kCallbackPartial;
    }
    else if(arg->v_type == kNvarUfunc || arg->v_type == kNvarString)
    {
        uchar_kt *name = arg->vval.v_string;
        func_ref(name);
        callback->data.funcref = ustrdup(name);
        callback->type = kCallbackFuncref;
    }
    else if(arg->v_type == kNvarNumber && arg->vval.v_number == 0)
    {
        callback->type = kCallbackNone;
    }
    else
    {
        EMSG(_("E921: Invalid callback argument"));
        return false;
    }

    return true;
}

/// Unref/free callback
void callback_free(callback_st *const callback)
FUNC_ATTR_NONNULL_ALL
{
    switch(callback->type)
    {
        case kCallbackFuncref:
        {
            func_unref(callback->data.funcref);
            xfree(callback->data.funcref);
            break;
        }

        case kCallbackPartial:
        {
            partial_unref(callback->data.partial);
            break;
        }

        case kCallbackNone:
        {
            break;
        }

        default:
        {
            abort();
        }
    }

    callback->type = kCallbackNone;
}

bool callback_call(callback_st *const callback,
                   const int argcount_in,
                   typval_st *const argvars_in,
                   typval_st *const rettv)
FUNC_ATTR_NONNULL_ALL
{
    partial_st *partial;
    uchar_kt *name;

    switch(callback->type)
    {
        case kCallbackFuncref:
            name = callback->data.funcref;
            partial = NULL;
            break;

        case kCallbackPartial:
            partial = callback->data.partial;
            name = partial_name(partial);
            break;

        case kCallbackNone:
            return false;
            break;

        default:
            abort();
    }

    int dummy;
    return call_func(name,
                     (int)ustrlen(name),
                     rettv,
                     argcount_in,
                     argvars_in,
                     NULL,
                     curwin->w_cursor.lnum,
                     curwin->w_cursor.lnum,
                     &dummy,
                     true,
                     partial,
                     NULL);
}

static bool set_ref_in_callback(callback_st *callback,
                                int copyID,
                                ht_stack_T **ht_stack,
                                list_stack_T **list_stack)
{
    typval_st tv;

    switch(callback->type)
    {
        case kCallbackFuncref:
        case kCallbackNone:
            break;

        case kCallbackPartial:
            tv.v_type = kNvarPartial;
            tv.vval.v_partial = callback->data.partial;
            return set_ref_in_item(&tv, copyID, ht_stack, list_stack);
            break;

        default:
            abort();
    }

    return false;
}

static void add_timer_info(typval_st *rettv, timer_st *timer)
{
    list_st *list = rettv->vval.v_list;
    dict_st *dict = tv_dict_alloc();

    tv_list_append_dict(list, dict);
    tv_dict_add_nr(dict, S_LEN("id"), timer->timer_id);
    tv_dict_add_nr(dict, S_LEN("time"), timer->timeout);
    tv_dict_add_nr(dict, S_LEN("paused"), timer->paused);

    tv_dict_add_nr(dict,
                   S_LEN("repeat"),
                   (timer->repeat_count < 0 ? -1 : timer->repeat_count));

    dictitem_st *di = tv_dict_item_alloc("callback");

    if(tv_dict_add(dict, di) == FAIL)
    {
        xfree(di);
        return;
    }

    if(timer->callback.type == kCallbackPartial)
    {
        di->di_tv.v_type = kNvarPartial;
        di->di_tv.vval.v_partial = timer->callback.data.partial;
        timer->callback.data.partial->pt_refcount++;
    }
    else if(timer->callback.type == kCallbackFuncref)
    {
        di->di_tv.v_type = kNvarUfunc;
        di->di_tv.vval.v_string = ustrdup(timer->callback.data.funcref);
    }

    di->di_tv.v_lock = 0;
}

static void add_timer_info_all(typval_st *rettv)
{
    timer_st *timer;

    map_foreach_value(timers, timer, {
        if(!timer->stopped)
        {
            add_timer_info(rettv, timer);
        }
    })
}

/// "timer_info([timer])" function
static void f_timer_info(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    tv_list_alloc_ret(rettv);

    if(argvars[0].v_type != kNvarUnknown)
    {
        if(argvars[0].v_type != kNvarNumber)
        {
            EMSG(_(e_number_exp));
            return;
        }

        timer_st *timer =
            pmap_get(uint64_t)(timers, tv_get_number(&argvars[0]));

        if(timer != NULL && !timer->stopped)
        {
            add_timer_info(rettv, timer);
        }
    }
    else
    {
        add_timer_info_all(rettv);
    }
}

/// "timer_pause(timer, paused)" function
static void f_timer_pause(typval_st *argvars,
                          typval_st *FUNC_ARGS_UNUSED_MATCH(unused),
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[0].v_type != kNvarNumber)
    {
        EMSG(_(e_number_exp));
        return;
    }

    int paused = (bool)tv_get_number(&argvars[1]);
    timer_st *timer = pmap_get(uint64_t)(timers, tv_get_number(&argvars[0]));

    if(timer != NULL)
    {
        timer->paused = paused;
    }
}

/// "timer_start(timeout, callback, opts)" function
static void f_timer_start(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    const long timeout = tv_get_number(&argvars[0]);
    timer_st *timer;
    int repeat = 1;
    dict_st *dict;
    rettv->vval.v_number = -1;

    if(argvars[2].v_type != kNvarUnknown)
    {
        if(argvars[2].v_type != kNvarDict
           || (dict = argvars[2].vval.v_dict) == NULL)
        {
            EMSG2(_(e_invarg2), tv_get_string(&argvars[2]));
            return;
        }

        dictitem_st *const di = tv_dict_find(dict, S_LEN("repeat"));

        if(di != NULL)
        {
            repeat = tv_get_number(&di->di_tv);

            if(repeat == 0)
            {
                repeat = 1;
            }
        }
    }

    callback_st callback;

    if(!callback_from_typval(&callback, &argvars[1]))
    {
        return;
    }

    timer = xmalloc(sizeof *timer);
    timer->refcount = 1;
    timer->stopped = false;
    timer->paused = false;
    timer->repeat_count = repeat;
    timer->timeout = timeout;
    timer->timer_id = last_timer_id++;
    timer->callback = callback;
    time_watcher_init(&main_loop, &timer->tw, timer);
    timer->tw.events = multiqueue_new_child(main_loop.events);

    // if main loop is blocked, don't queue up multiple events
    timer->tw.blockable = true;
    time_watcher_start(&timer->tw, timer_due_cb, timeout, timeout);

    pmap_put(uint64_t)(timers, timer->timer_id, timer);
    rettv->vval.v_number = timer->timer_id;
}


/// "timer_stop(timerid)" function
static void f_timer_stop(typval_st *argvars,
                         typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(argvars[0].v_type != kNvarNumber)
    {
        EMSG(_(e_number_exp));
        return;
    }

    timer_st *timer = pmap_get(uint64_t)(timers, tv_get_number(&argvars[0]));

    if(timer == NULL)
    {
        return;
    }

    timer_stop(timer);
}

static void f_timer_stopall(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                            typval_st *FUNC_ARGS_UNUSED_MATCH(unused),
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    timer_stop_all();
}

/// invoked on the main loop
static void timer_due_cb(time_watcher_st *FUNC_ARGS_UNUSED_MATCH(tw), void *data)
{
    timer_st *timer = (timer_st *)data;

    if(timer->stopped || timer->paused)
    {
        return;
    }

    timer->refcount++;

    // if repeat was negative repeat forever
    if(timer->repeat_count >= 0 && --timer->repeat_count == 0)
    {
        timer_stop(timer);
    }

    typval_st argv[2] = { TV_INITIAL_VALUE, TV_INITIAL_VALUE };

    argv[0].v_type = kNvarNumber;
    argv[0].vval.v_number = timer->timer_id;

    typval_st rettv = TV_INITIAL_VALUE;

    callback_call(&timer->callback, 1, argv, &rettv);
    tv_clear(&rettv);

    if(!timer->stopped && timer->timeout == 0)
    {
        // special case: timeout=0 means the callback will be
        // invoked again on the next event loop tick.
        // we don't use uv_idle_t to not spin the event loop
        // when the main loop is blocked.
        time_watcher_start(&timer->tw, timer_due_cb, 0, 0);
    }

    timer_decref(timer);
}

static void timer_stop(timer_st *timer)
{
    if(timer->stopped)
    {
        // avoid double free
        return;
    }

    timer->stopped = true;
    time_watcher_stop(&timer->tw);
    time_watcher_close(&timer->tw, timer_close_cb);
}

/// invoked on next event loop tick, so queue is empty
static void timer_close_cb(time_watcher_st *FUNC_ARGS_UNUSED_MATCH(tw), void *data)
{
    timer_st *timer = (timer_st *)data;
    multiqueue_free(timer->tw.events);
    callback_free(&timer->callback);
    pmap_del(uint64_t)(timers, timer->timer_id);
    timer_decref(timer);
}

static void timer_decref(timer_st *timer)
{
    if(--timer->refcount == 0)
    {
        xfree(timer);
    }
}

static void timer_stop_all(void)
{
    timer_st *timer;

    map_foreach_value(timers, timer, {
        timer_stop(timer);
    })
}

void timer_teardown(void)
{
    timer_stop_all();
}

/// "tolower(string)" function
static void f_tolower(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;

    rettv->vval.v_string =
        (uchar_kt *)xstrdup_case_convert(tv_get_string(&argvars[0]), false);
}

/// "toupper(string)" function
static void f_toupper(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;

    rettv->vval.v_string =
        (uchar_kt *)xstrdup_case_convert(tv_get_string(&argvars[0]), true);
}

/// "tr(string, fromstr, tostr)" function
static void f_tr(typval_st *argvars,
                 typval_st *rettv,
                 func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    char buf[NUMBUFLEN];
    char buf2[NUMBUFLEN];
    const char *in_str = tv_get_string(&argvars[0]);
    const char *fromstr = tv_get_string_buf_chk(&argvars[1], buf);
    const char *tostr = tv_get_string_buf_chk(&argvars[2], buf2);

    // Default return value: empty string.
    rettv->v_type = kNvarString;
    rettv->vval.v_string = NULL;

    if(fromstr == NULL || tostr == NULL)
    {
        return; // Type error; errmsg already given.
    }

    garray_st ga;
    ga_init(&ga, (int)sizeof(char), 80);

    // fromstr and tostr have to contain the same number of chars.
    bool first = true;

    while(*in_str != NUL)
    {
        const char *cpstr = in_str;
        const int inlen = (*mb_ptr2len)((const uchar_kt *)in_str);
        int cplen = inlen;
        int idx = 0;
        int fromlen;

        for(const char *p = fromstr; *p != NUL; p += fromlen)
        {
            fromlen = (*mb_ptr2len)((const uchar_kt *)p);

            if(fromlen == inlen && ustrncmp(in_str, p, inlen) == 0)
            {
                int tolen;

                for(p = tostr; *p != NUL; p += tolen)
                {
                    tolen = (*mb_ptr2len)((const uchar_kt *)p);

                    if(idx-- == 0)
                    {
                        cplen = tolen;
                        cpstr = (char *)p;
                        break;
                    }
                }

                if(*p == NUL)
                {
                    // tostr is shorter than fromstr.
                    goto error;
                }

                break;
            }

            idx++;
        }

        if(first && cpstr == in_str)
        {
            // Check that fromstr and tostr have the same number of
            // (multi-byte) characters. Done only once when a character
            // of in_str doesn't appear in fromstr.
            first = false;
            int tolen;

            for(const char *p = tostr; *p != NUL; p += tolen)
            {
                tolen = (*mb_ptr2len)((const uchar_kt *)p);
                idx--;
            }

            if(idx != 0)
            {
                goto error;
            }
        }

        ga_grow(&ga, cplen);
        memmove((char *)ga.ga_data + ga.ga_len, cpstr, (size_t)cplen);
        ga.ga_len += cplen;
        in_str += inlen;
    }

    // add a terminating NUL
    ga_append(&ga, NUL);
    rettv->vval.v_string = ga.ga_data;
    return;

error:

    EMSG2(_(e_invarg2), fromstr);
    ga_clear(&ga);
    return;
}

/// "type(expr)" function
static void f_type(typval_st *argvars,
                   typval_st *rettv,
                   func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int n = -1;

    switch(argvars[0].v_type)
    {
        case kNvarNumber:
            n = VAR_TYPE_NUMBER;
            break;

        case kNvarString:
            n = VAR_TYPE_STRING;
            break;

        case kNvarPartial:
        case kNvarUfunc:
            n = VAR_TYPE_FUNC;
            break;

        case kNvarList:
            n = VAR_TYPE_LIST;
            break;

        case kNvarDict:
            n = VAR_TYPE_DICT;
            break;

        case kNvarFloat:
            n = VAR_TYPE_FLOAT;
            break;

        case kNvarSpecial:
        {
            switch(argvars[0].vval.v_special)
            {
                case kSpecialVarTrue:
                case kSpecialVarFalse:
                {
                    n = VAR_TYPE_BOOL;
                    break;
                }

                case kSpecialVarNull:
                {
                    n = 7;
                    break;
                }
            }

            break;
        }

        case kNvarUnknown:
        {
            EMSG2(_(e_intern2), "f_type(UNKNOWN)");
            break;
        }
    }

    rettv->vval.v_number = n;
}

/// "undofile(name)" function
static void f_undofile(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->v_type = kNvarString;
    const char *const fname = tv_get_string(&argvars[0]);

    if(*fname == NUL)
    {
        // If there is no file name there will be no undo file.
        rettv->vval.v_string = NULL;
    }
    else
    {
        char *ffname = FullName_save(fname, false);

        if(ffname != NULL)
        {
            rettv->vval.v_string =
                (uchar_kt *)u_get_undo_file_name(ffname, false);
        }

        xfree(ffname);
    }
}

/// "undotree()" function
static void f_undotree(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    tv_dict_alloc_ret(rettv);
    dict_st *dict = rettv->vval.v_dict;
    list_st *list;

    tv_dict_add_nr(dict, S_LEN("synced"), (number_kt)curbuf->b_u_synced);
    tv_dict_add_nr(dict, S_LEN("seq_last"), (number_kt)curbuf->b_u_seq_last);
    tv_dict_add_nr(dict, S_LEN("save_last"), (number_kt)curbuf->b_u_save_nr_last);
    tv_dict_add_nr(dict, S_LEN("seq_cur"), (number_kt)curbuf->b_u_seq_cur);
    tv_dict_add_nr(dict, S_LEN("time_cur"), (number_kt)curbuf->b_u_time_cur);
    tv_dict_add_nr(dict, S_LEN("save_cur"), (number_kt)curbuf->b_u_save_nr_cur);

    list = tv_list_alloc();
    u_eval_tree(curbuf->b_u_oldhead, list);
    tv_dict_add_list(dict, S_LEN("entries"), list);
}

/// "values(dict)" function
static void f_values(typval_st *argvars,
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    dict_list(argvars, rettv, 1);
}

/// "virtcol(string)" function
static void f_virtcol(typval_st *argvars,
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    columnum_kt vcol = 0;
    apos_st *fp;
    int fnum = curbuf->b_id;
    fp = var2fpos(&argvars[0], FALSE, &fnum);

    if(fp != NULL && fp->lnum <= curbuf->b_ml.ml_line_count
       && fnum == curbuf->b_id)
    {
        getvvcol(curwin, fp, NULL, NULL, &vcol);
        ++vcol;
    }

    rettv->vval.v_number = vcol;
}

/// "visualmode()" function
static void f_visualmode(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    uchar_kt str[2];
    rettv->v_type = kNvarString;
    str[0] = curbuf->b_visual_mode_eval;
    str[1] = NUL;
    rettv->vval.v_string = ustrdup(str);

    // A non-zero number or non-empty string argument: reset mode.
    if(non_zero_arg(&argvars[0]))
    {
        curbuf->b_visual_mode_eval = NUL;
    }
}

/// "wildmenumode()" function
static void f_wildmenumode(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                           typval_st *rettv,
                           func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    if(wild_menu_showing)
    {
        rettv->vval.v_number = 1;
    }
}

/// "win_findbuf()" function
static void f_win_findbuf(typval_st *argvars,
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    tv_list_alloc_ret(rettv);
    win_findbuf(argvars, rettv->vval.v_list);
}

/// "win_getid()" function
static void f_win_getid(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = win_getid(argvars);
}

/// "win_gotoid()" function
static void f_win_gotoid(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = win_gotoid(argvars);
}

/// "win_id2tabwin()" function
static void f_win_id2tabwin(typval_st *argvars,
                            typval_st *rettv,
                            func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    tv_list_alloc_ret(rettv);
    win_id2tabwin(argvars, rettv->vval.v_list);
}

/// "win_id2win()" function
static void f_win_id2win(typval_st *argvars,
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = win_id2win(argvars);
}

/// "winbufnr(nr)" function
static void f_winbufnr(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    win_st *wp;
    wp = find_win_by_nr(&argvars[0], NULL);

    if(wp == NULL)
    {
        rettv->vval.v_number = -1;
    }
    else
    {
        rettv->vval.v_number = wp->w_buffer->b_id;
    }
}

/// "wincol()" function
static void f_wincol(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                     typval_st *rettv,
                     func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    validate_cursor();
    rettv->vval.v_number = curwin->w_wcol + 1;
}

/// "winheight(nr)" function
static void f_winheight(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    win_st *wp;
    wp = find_win_by_nr(&argvars[0], NULL);

    if(wp == NULL)
    {
        rettv->vval.v_number = -1;
    }
    else
    {
        rettv->vval.v_number = wp->w_height;
    }
}

/// "winline()" function
static void f_winline(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                      typval_st *rettv,
                      func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    validate_cursor();
    rettv->vval.v_number = curwin->w_wrow + 1;
}

/// "winnr()" function
static void f_winnr(typval_st *argvars,
                    typval_st *rettv,
                    func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int nr = 1;
    nr = get_winnr(curtab, &argvars[0]);
    rettv->vval.v_number = nr;
}

/// "winrestcmd()" function
static void f_winrestcmd(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                         typval_st *rettv,
                         func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    int winnr = 1;
    garray_st ga;
    uchar_kt buf[50];
    ga_init(&ga, (int)sizeof(char), 70);

    FOR_ALL_WINDOWS_IN_TAB(wp, curtab)
    {
        sprintf((char *)buf, "%dresize %d|", winnr, wp->w_height);
        ga_concat(&ga, buf);
        sprintf((char *)buf, "vert %dresize %d|", winnr, wp->w_width);
        ga_concat(&ga, buf);
        ++winnr;
    }

    ga_append(&ga, NUL);
    rettv->vval.v_string = ga.ga_data;
    rettv->v_type = kNvarString;
}

/// "winrestview()" function
static void f_winrestview(typval_st *argvars,
                          typval_st *FUNC_ARGS_UNUSED_MATCH(rettv),
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    dict_st *dict;

    if(argvars[0].v_type != kNvarDict
       || (dict = argvars[0].vval.v_dict) == NULL)
    {
        emsgf(_(e_invarg));
    }
    else
    {
        dictitem_st *di;

        if((di = tv_dict_find(dict, S_LEN("lnum"))) != NULL)
        {
            curwin->w_cursor.lnum = tv_get_number(&di->di_tv);
        }

        if((di = tv_dict_find(dict, S_LEN("col"))) != NULL)
        {
            curwin->w_cursor.col = tv_get_number(&di->di_tv);
        }

        if((di = tv_dict_find(dict, S_LEN("coladd"))) != NULL)
        {
            curwin->w_cursor.coladd = tv_get_number(&di->di_tv);
        }

        if((di = tv_dict_find(dict, S_LEN("curswant"))) != NULL)
        {
            curwin->w_curswant = tv_get_number(&di->di_tv);
            curwin->w_set_curswant = false;
        }

        if((di = tv_dict_find(dict, S_LEN("topline"))) != NULL)
        {
            set_topline(curwin, tv_get_number(&di->di_tv));
        }

        if((di = tv_dict_find(dict, S_LEN("topfill"))) != NULL)
        {
            curwin->w_topfill = tv_get_number(&di->di_tv);
        }

        if((di = tv_dict_find(dict, S_LEN("leftcol"))) != NULL)
        {
            curwin->w_leftcol = tv_get_number(&di->di_tv);
        }

        if((di = tv_dict_find(dict, S_LEN("skipcol"))) != NULL)
        {
            curwin->w_skipcol = tv_get_number(&di->di_tv);
        }

        check_cursor();
        win_new_height(curwin, curwin->w_height);
        win_new_width(curwin, curwin->w_width);
        changed_window_setting();

        if(curwin->w_topline <= 0)
        {
            curwin->w_topline = 1;
        }

        if(curwin->w_topline > curbuf->b_ml.ml_line_count)
        {
            curwin->w_topline = curbuf->b_ml.ml_line_count;
        }

        check_topfill(curwin, true);
    }
}

/// @b winsaveview() function
static void f_winsaveview(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                          typval_st *rettv,
                          func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    dict_st *dict;

    tv_dict_alloc_ret(rettv);
    dict = rettv->vval.v_dict;

    tv_dict_add_nr(dict, S_LEN("lnum"), (number_kt)curwin->w_cursor.lnum);
    tv_dict_add_nr(dict, S_LEN("col"), (number_kt)curwin->w_cursor.col);
    tv_dict_add_nr(dict, S_LEN("coladd"), (number_kt)curwin->w_cursor.coladd);

    update_curswant();

    tv_dict_add_nr(dict, S_LEN("curswant"), (number_kt)curwin->w_curswant);
    tv_dict_add_nr(dict, S_LEN("topline"), (number_kt)curwin->w_topline);
    tv_dict_add_nr(dict, S_LEN("topfill"), (number_kt)curwin->w_topfill);
    tv_dict_add_nr(dict, S_LEN("leftcol"), (number_kt)curwin->w_leftcol);
    tv_dict_add_nr(dict, S_LEN("skipcol"), (number_kt)curwin->w_skipcol);
}

/// Writes list of strings to file
///
/// @param  fp  File to write to.
/// @param[in]  list  List to write.
/// @param[in]  binary  Whether to write in binary mode.
///
/// @return true in case of success, false otherwise.
static bool write_list(filedesc_st *const fp,
                       const list_st *const list,
                       const bool binary)
{
    int error = 0;

    for(const listitem_st *li = list->lv_first; li != NULL; li = li->li_next)
    {
        const char *const s = tv_get_string_chk(&li->li_tv);

        if(s == NULL)
        {
            return false;
        }

        const char *hunk_start = s;

        for(const char *p = hunk_start;; p++)
        {
            if(*p == NUL || *p == NL)
            {
                if(p != hunk_start)
                {
                    const ptrdiff_t written =
                        file_write(fp,
                                   hunk_start,
                                   (size_t)(p - hunk_start));

                    if(written < 0)
                    {
                        error = (int)written;
                        goto write_list_error;
                    }
                }

                if(*p == NUL)
                {
                    break;
                }
                else
                {
                    hunk_start = p + 1;

                    const ptrdiff_t written =
                        file_write(fp, (char[]) { NUL }, 1);

                    if(written < 0)
                    {
                        error = (int)written;
                        break;
                    }
                }
            }
        }

        if(!binary || li->li_next != NULL)
        {
            const ptrdiff_t written = file_write(fp, "\n", 1);

            if(written < 0)
            {
                error = (int)written;
                goto write_list_error;
            }
        }
    }

    if((error = file_flush(fp)) != 0)
    {
        goto write_list_error;
    }

    return true;

write_list_error:

    emsgf(_("E80: Error while writing: %s"), os_strerror(error));
    return false;
}

/// Initializes a static list with 10 items.
void init_static_list(list10item_st *sl)
{
    list_st *l = &sl->sl_list;

    memset(sl, 0, sizeof(list10item_st));
    l->lv_first = &sl->sl_items[0];
    l->lv_last = &sl->sl_items[9];
    l->lv_refcount = DO_NOT_FREE_CNT;
    l->lv_lock = kNvlVarFixed;
    sl->sl_list.lv_len = 10;

    for(int i = 0; i < 10; i++)
    {
        listitem_st *li = &sl->sl_items[i];

        if(i == 0)
        {
            li->li_prev = NULL;
        }
        else
        {
            li->li_prev = li - 1;
        }

        if(i == 9)
        {
            li->li_next = NULL;
        }
        else
        {
            li->li_next = li + 1;
        }
    }
}

/// Saves a typval_st as a string.
///
/// For lists, replaces NLs with NUL and separates items with NLs.
///
/// @param[in] tv
/// A value to store as a string.
///
/// @param[out] len
/// The length of the resulting string or -1 on error.
///
/// @param[in] endnl
/// If true, the output will end in a newline (if a list).
///
/// @returns
/// an allocated string if @b tv represents a VimL string,
/// list, or number; NULL otherwise.
static char *save_tv_as_string(typval_st *tv, ptrdiff_t *const len, bool endnl)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
{
    if(tv->v_type == kNvarUnknown)
    {
        *len = 0;
        return NULL;
    }

    // For types other than list, let tv_get_string_buf_chk()
    // get the value or print an error.
    if(tv->v_type != kNvarList)
    {
        const char *ret = tv_get_string_chk(tv);

        if(ret && (*len = strlen(ret)))
        {
            return xmemdupz(ret, (size_t)(*len));
        }
        else
        {
            *len = -1;
            return NULL;
        }
    }

    // Pre-calculate the resulting length.
    *len = 0;
    list_st *list = tv->vval.v_list;

    for(listitem_st *li = list->lv_first; li != NULL; li = li->li_next)
    {
        *len += strlen(tv_get_string(&li->li_tv)) + 1;
    }

    if(*len == 0)
    {
        return NULL;
    }

    char *ret = xmalloc(*len + endnl);
    char *end = ret;

    for(listitem_st *li = list->lv_first; li != NULL; li = li->li_next)
    {
        for(const char *s = tv_get_string(&li->li_tv); *s != NUL; s++)
        {
            *end++ = (*s == '\n') ? NUL : *s;
        }

        if(endnl || li->li_next != NULL)
        {
            *end++ = '\n';
        }
    }

    *end = NUL;
    *len = end - ret;
    return ret;
}

/// "winwidth(nr)" function
static void f_winwidth(typval_st *argvars,
                       typval_st *rettv,
                       func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    win_st *wp;
    wp = find_win_by_nr(&argvars[0], NULL);

    if(wp == NULL)
    {
        rettv->vval.v_number = -1;
    }
    else
    {
        rettv->vval.v_number = wp->w_width;
    }
}

/// "wordcount()" function
static void f_wordcount(typval_st *FUNC_ARGS_UNUSED_MATCH(argvars),
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    tv_dict_alloc_ret(rettv);
    cursor_pos_info(rettv->vval.v_dict);
}

/// "writefile()" function
static void f_writefile(typval_st *argvars,
                        typval_st *rettv,
                        func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = 0; // Assuming success.

    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarList)
    {
        EMSG2(_(e_listarg), "writefile()");
        return;
    }

    if(argvars[0].vval.v_list == NULL)
    {
        return;
    }

    bool binary = false;
    bool append = false;
    bool do_fsync = !!p_fs;

    if(argvars[2].v_type != kNvarUnknown)
    {
        const char *const flags = tv_get_string_chk(&argvars[2]);

        if(flags == NULL)
        {
            return;
        }

        for(const char *p = flags; *p; p++)
        {
            switch(*p)
            {
                case 'b':
                {
                    binary = true;
                    break;
                }

                case 'a':
                {
                    append = true;
                    break;
                }

                case 's':
                {
                    do_fsync = true;
                    break;
                }

                case 'S':
                {
                    do_fsync = false;
                    break;
                }

                default:
                {
                    // Using %s, p and not %c, *p to
                    // preserve multibyte characters
                    emsgf(_("E5060: Unknown flag: %s"), p);
                    return;
                }
            }
        }
    }

    char buf[NUMBUFLEN];
    const char *const fname = tv_get_string_buf_chk(&argvars[1], buf);

    if(fname == NULL)
    {
        return;
    }

    filedesc_st fp;
    int error;
    rettv->vval.v_number = -1;

    if(*fname == NUL)
    {
        EMSG(_("E482: Can't open file with an empty name"));
    }
    else if((error = file_open(&fp,
                               fname,
                               ((append ? kFileAppend : kFileTruncate)
                                | kFileCreate), 0666)) != 0)
    {
        emsgf(_("E482: Can't open file %s for writing: %s"),
              fname, os_strerror(error));
    }
    else
    {
        if(write_list(&fp, argvars[0].vval.v_list, binary))
        {
            rettv->vval.v_number = 0;
        }

        if((error = file_close(&fp, do_fsync)) != 0)
        {
            emsgf(_("E80: Error when closing file %s: %s"),
                  fname, os_strerror(error));
        }
    }
}

/// "xor(expr, expr)" function
static void f_xor(typval_st *argvars,
                  typval_st *rettv,
                  func_ptr_ft FUNC_ARGS_UNUSED_MATCH(fptr))
{
    rettv->vval.v_number = tv_get_number_chk(&argvars[0], NULL)
                           ^ tv_get_number_chk(&argvars[1], NULL);
}


/// Translate a VimL object into a position
///
/// Accepts kNvarList and kNvarString objects.
/// Does not give an error for invalid type.
///
/// @param[in] tv           Object to translate.
/// @param[in] dollar_lnum  True when "$" is last line.
/// @param[out] ret_fnum    Set to fnum for marks.
///
/// @return Pointer to position or NULL in case of error (e.g. invalid type).
apos_st *var2fpos(const typval_st *const tv,
                const int dollar_lnum,
                int *const ret_fnum)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    static apos_st pos;
    apos_st *pp;

    // Argument can be [lnum, col, coladd].
    if(tv->v_type == kNvarList)
    {
        list_st *l;
        int len;
        bool error = false;
        listitem_st *li;
        l = tv->vval.v_list;

        if(l == NULL)
        {
            return NULL;
        }

        // Get the line number.
        pos.lnum = tv_list_find_nr(l, 0L, &error);

        if(error || pos.lnum <= 0 || pos.lnum > curbuf->b_ml.ml_line_count)
        {
            // Invalid line number.
            return NULL;
        }

        // Get the column number.
        pos.col = tv_list_find_nr(l, 1L, &error);

        if(error)
        {
            return NULL;
        }

        len = (long)ustrlen(ml_get(pos.lnum));
        // We accept "$" for the column number: last column.
        li = tv_list_find(l, 1L);

        if(li != NULL
           && li->li_tv.v_type == kNvarString
           && li->li_tv.vval.v_string != NULL
           && ustrcmp(li->li_tv.vval.v_string, "$") == 0)
        {
            pos.col = len + 1;
        }

        // Accept a position up to the NUL after the line.
        if(pos.col == 0 || (int)pos.col > len + 1)
        {
            // Invalid column number.
            return NULL;
        }

        pos.col--; // Get the virtual offset.  Defaults to zero.
        pos.coladd = tv_list_find_nr(l, 2L, &error);

        if(error)
        {
            pos.coladd = 0;
        }

        return &pos;
    }

    const char *const name = tv_get_string_chk(tv);

    if(name == NULL)
    {
        return NULL;
    }

    if(name[0] == '.') // Cursor.
    {
        return &curwin->w_cursor;
    }

    if(name[0] == 'v' && name[1] == NUL) // Visual start.
    {
        if(VIsual_active)
        {
            return &VIsual;
        }

        return &curwin->w_cursor;
    }

    if(name[0] == '\'') // Mark.
    {
        pp = getmark_buf_fnum(curbuf, (uint8_t)name[1], false, ret_fnum);

        if(pp == NULL || pp == (apos_st *)-1 || pp->lnum <= 0)
        {
            return NULL;
        }

        return pp;
    }

    pos.coladd = 0;

    if(name[0] == 'w' && dollar_lnum)
    {
        pos.col = 0;

        if(name[1] == '0')  // "w0": first visible line
        {
            update_topline();
            pos.lnum = curwin->w_topline;
            return &pos;
        }
        else if(name[1] == '$') // "w$": last visible line
        {
            validate_botline();
            pos.lnum = curwin->w_botline - 1;
            return &pos;
        }
    }
    else if(name[0] == '$') // last column or line
    {
        if(dollar_lnum)
        {
            pos.lnum = curbuf->b_ml.ml_line_count;
            pos.col = 0;
        }
        else
        {
            pos.lnum = curwin->w_cursor.lnum;
            pos.col = (columnum_kt)ustrlen(get_cursor_line_ptr());
        }

        return &pos;
    }

    return NULL;
}

/// Convert list in "arg" into a position and optional file number.
/// When "fnump" is NULL there is no file number, only 3 items.
/// Note that the column is passed on as-is, the caller may want to
/// decrement it to use 1 for the first column.
///
/// @return
/// FAIL when conversion is not possible,
/// doesn't check the position for validity.
static int list2fpos(typval_st *arg,
                     apos_st *posp,
                     int *fnump,
                     columnum_kt *curswantp)
{
    list_st *l = arg->vval.v_list;
    long i = 0;
    long n;

    // List must be: [fnum, lnum, col, coladd, curswant], where "fnum" is only
    // there when "fnump" isn't NULL; "coladd" and "curswant" are optional.
    if(arg->v_type != kNvarList
       || l == NULL
       || l->lv_len < (fnump == NULL ? 2 : 3)
       || l->lv_len > (fnump == NULL ? 4 : 5))
    {
        return FAIL;
    }

    if(fnump != NULL)
    {
        n = tv_list_find_nr(l, i++, NULL); // fnum

        if(n < 0)
        {
            return FAIL;
        }

        if(n == 0)
        {
            n = curbuf->b_id; // Current buffer.
        }

        *fnump = n;
    }

    n = tv_list_find_nr(l, i++, NULL); // lnum

    if(n < 0)
    {
        return FAIL;
    }

    posp->lnum = n;
    n = tv_list_find_nr(l, i++, NULL); // col

    if(n < 0)
    {
        return FAIL;
    }

    posp->col = n;
    n = tv_list_find_nr(l, i, NULL); // off

    if(n < 0)
    {
        posp->coladd = 0;
    }
    else
    {
        posp->coladd = n;
    }

    if(curswantp != NULL)
    {
        *curswantp = tv_list_find_nr(l, i + 1, NULL); // curswant
    }

    return OK;
}

/// Get the length of an environment variable name.
/// Advance "arg" to the first character after the name.
/// Return 0 for error.
static int get_env_len(const uchar_kt **arg)
{
    int len;
    const uchar_kt *p;

    for(p = *arg; is_id_char(*p); p++)
    { /* empty body */ }

    if(p == *arg) // No name found.
    {
        return 0;
    }

    len = (int)(p - *arg);
    *arg = p;
    return len;
}

/// Get the length of the name of a function or internal variable.
/// "arg" is advanced to the first non-white character after the name.
/// Return 0 if something is wrong.
static int get_id_len(const char **const arg)
{
    int len;
    // Find the end of the name.
    const char *p;

    for(p = *arg; eval_isnamec(*p); p++)
    {
        if(*p == ':')
        {
            // "s:" is start of "s:var", but "n:" is not and can be used in
            // slice "[n:]". Also "xx:" is not a namespace.
            len = (int)(p - *arg);

            if(len > 1
               || (len == 1 && ustrchr(namespace_char, **arg) == NULL))
            {
                break;
            }
        }
    }

    if(p == *arg) // no name found
    {
        return 0;
    }

    len = (int)(p - *arg);
    *arg = (const char *)skipwhite((const uchar_kt *)p);
    return len;
}

/// Get the length of the name of a variable or function.
/// Only the name is recognized, does not handle ".key" or "[idx]".
/// "arg" is advanced to the first non-white character after the name.
/// Return -1 if curly braces expansion failed.
/// Return 0 if something else is wrong.
/// If the name contains 'magic' {}'s, expand them and return the
/// expanded name in an allocated string via 'alias' - caller must free.
static int get_name_len(const char **const arg,
                        char **alias,
                        int evaluate,
                        int verbose)
{
    int len;
    *alias = NULL; // default to no alias

    if((*arg)[0] == (char)K_SPECIAL
       && (*arg)[1] == (char)KS_EXTRA
       && (*arg)[2] == (char)KE_SNR)
    {
        // Hard coded <SNR>, already translated.
        *arg += 3;
        return get_id_len(arg) + 3;
    }

    len = eval_fname_script(*arg);

    if(len > 0)
    {
        // literal "<SID>", "s:" or "<SNR>"
        *arg += len;
    }

    // Find the end of the name; check for {} construction.
    uchar_kt *expr_start;
    uchar_kt *expr_end;

    const char *p =
        (const char *)find_name_end((uchar_kt *)(*arg),
                                    (const uchar_kt **)&expr_start,
                                    (const uchar_kt **)&expr_end,
                                    len > 0 ? 0 : FNE_CHECK_START);

    if(expr_start != NULL)
    {
        if(!evaluate)
        {
            len += (int)(p - *arg);
            *arg = (const char *)skipwhite((const uchar_kt *)p);
            return len;
        }

        // Include any <SID> etc in the expanded string:
        // Thus the -len here.
        uchar_kt *temp_string = make_expanded_name((uchar_kt *)(*arg) - len,
                                                 expr_start,
                                                 expr_end,
                                                 (uchar_kt *)p);

        if(temp_string == NULL)
        {
            return -1;
        }

        *alias = (char *)temp_string;
        *arg = (const char *)skipwhite((const uchar_kt *)p);
        return (int)ustrlen(temp_string);
    }

    len += get_id_len(arg);

    if(len == 0 && verbose)
    {
        EMSG2(_(e_invexpr2), *arg);
    }

    return len;
}

/// Find the end of a variable or function name, taking care of magic braces.
/// If "expr_start" is not NULL then "expr_start" and "expr_end" are set to the
/// start and end of the first magic braces item.
/// "flags" can have FNE_INCL_BR and FNE_CHECK_START.
/// Return a pointer to just after the name.
/// Equal to "arg" if there is no valid name.
static const uchar_kt *find_name_end(const uchar_kt *arg,
                                   const uchar_kt **expr_start,
                                   const uchar_kt **expr_end,
                                   int flags)
{
    int mb_nest = 0;
    int br_nest = 0;
    int len;

    if(expr_start != NULL)
    {
        *expr_start = NULL;
        *expr_end = NULL;
    }

    // Quick check for valid starting character.
    if((flags & FNE_CHECK_START)
       && !eval_isnamec1(*arg) && *arg != '{')
    {
        return arg;
    }

    const uchar_kt *p;

    for(p = arg;
        *p != NUL
        && (eval_isnamec(*p)
            || *p == '{'
            || ((flags & FNE_INCL_BR) && (*p == '[' || *p == '.'))
            || mb_nest != 0
            || br_nest != 0);
        mb_ptr_adv(p))
    {
        if(*p == '\'')
        {
            // skip over 'string' to avoid counting [ and ] inside it.
            for(p = p + 1; *p != NUL && *p != '\''; mb_ptr_adv(p))
            { /* empty body */ }

            if(*p == NUL)
            {
                break;
            }
        }
        else if(*p == '"')
        {
            // skip over "str\"ing" to avoid counting [ and ] inside it.
            for(p = p + 1; *p != NUL && *p != '"'; mb_ptr_adv(p))
            {
                if(*p == '\\' && p[1] != NUL)
                {
                    ++p;
                }
            }

            if(*p == NUL)
            {
                break;
            }
        }
        else if(br_nest == 0 && mb_nest == 0 && *p == ':')
        {
            // "s:" is start of "s:var", but "n:" is not and can be used in
            // slice "[n:]".  Also "xx:" is not a namespace. But {ns}: is.
            len = (int)(p - arg);

            if((len > 1 && p[-1] != '}')
               || (len == 1 && ustrchr(namespace_char, *arg) == NULL))
            {
                break;
            }
        }

        if(mb_nest == 0)
        {
            if(*p == '[')
            {
                ++br_nest;
            }
            else if(*p == ']')
            {
                --br_nest;
            }
        }

        if(br_nest == 0)
        {
            if(*p == '{')
            {
                mb_nest++;

                if(expr_start != NULL && *expr_start == NULL)
                {
                    *expr_start = p;
                }
            }
            else if(*p == '}')
            {
                mb_nest--;

                if(expr_start != NULL && mb_nest == 0 && *expr_end == NULL)
                {
                    *expr_end = p;
                }
            }
        }
    }

    return p;
}

/// Expands out the 'magic' {}'s in a variable/function name.
/// Note that this can call itself recursively, to deal with
/// constructs like foo{bar}{baz}{bam}
/// The four pointer arguments point to "foo{expre}ss{ion}bar"
/// - "in_start"    ^
/// - "expr_start"  ^
/// - "expr_end"    ^
/// - "in_end"      ^
///
/// Returns a new allocated string, which the caller must free.
/// Returns NULL for failure.
static uchar_kt *make_expanded_name(const uchar_kt *in_start,
                                  uchar_kt *expr_start,
                                  uchar_kt *expr_end,
                                  uchar_kt *in_end)
{
    uchar_kt c1;
    uchar_kt *retval = NULL;
    uchar_kt *temp_result;
    uchar_kt *nextcmd = NULL;

    if(expr_end == NULL || in_end == NULL)
    {
        return NULL;
    }

    *expr_start = NUL;
    *expr_end = NUL;
    c1 = *in_end;
    *in_end = NUL;
    temp_result = eval_to_string(expr_start + 1, &nextcmd, FALSE);

    if(temp_result != NULL && nextcmd == NULL)
    {
        retval = xmalloc(ustrlen(temp_result)
                         + (expr_start - in_start)
                         + (in_end - expr_end) + 1);

        ustrcpy(retval, in_start);
        ustrcat(retval, temp_result);
        ustrcat(retval, expr_end + 1);
    }

    xfree(temp_result);
    *in_end = c1; // put char back for error messages
    *expr_start = '{';
    *expr_end = '}';

    if(retval != NULL)
    {
        temp_result = (uchar_kt *)find_name_end(retval,
                                              (const uchar_kt **)&expr_start,
                                              (const uchar_kt **)&expr_end,
                                              0);

        if(expr_start != NULL)
        {
            // Further expansion!
            temp_result = make_expanded_name(retval,
                                             expr_start,
                                             expr_end,
                                             temp_result);
            xfree(retval);
            retval = temp_result;
        }
    }

    return retval;
}

/// Return TRUE if character "c" can be used in a variable or function name.
/// Does not include '{' or '}' for magic braces.
static int eval_isnamec(int c)
{
    return ASCII_ISALNUM(c) || c == '_' || c == ':' || c == AUTOLOAD_CHAR;
}

/// Return TRUE if character "c" can be used as the first character in a
/// variable or function name (excluding '{' and '}').
static int eval_isnamec1(int c)
{
    return ASCII_ISALPHA(c) || c == '_';
}

/// Get number v: variable value.
long get_vim_var_nr(int idx)
FUNC_ATTR_PURE
{
    return vimvars[idx].vv_nr;
}

/// Get string v: variable value.
/// Uses a static buffer, can only be used once.
uchar_kt *get_vim_var_str(int idx)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_RETURN
{
    return (uchar_kt *)tv_get_string(&vimvars[idx].vv_tv);
}

/// Get List v: variable value.
/// Caller must take care of reference count when needed.
list_st *get_vim_var_list(int idx)
FUNC_ATTR_PURE
{
    return vimvars[idx].vv_list;
}

/// Get Dictionary v: variable value.
/// Caller must take care of reference count when needed.
dict_st *get_vim_var_dict(int idx)
FUNC_ATTR_PURE
{
    return vimvars[idx].vv_dict;
}

/// Set v:char to character "c".
void set_vim_var_char(int c)
{
    char buf[MB_MAXBYTES + 1];

    buf[(*mb_char2bytes)(c, (uchar_kt *) buf)] = NUL;
    set_vim_var_string(VV_CHAR, buf, -1);
}

/// Set v:count to "count" and v:count1 to "count1".
/// When "set_prevcount" is TRUE first set v:prevcount from v:count.
void set_vcount(long count, long count1, int set_prevcount)
{
    if(set_prevcount)
    {
        vimvars[VV_PREVCOUNT].vv_nr = vimvars[VV_COUNT].vv_nr;
    }

    vimvars[VV_COUNT].vv_nr = count;
    vimvars[VV_COUNT1].vv_nr = count1;
}

/// Set number v: variable to the given value
///
/// @param[in]  idx  Index of variable to set.
/// @param[in]  val  Value to set to.
void set_vim_var_nr(const VimVarIndex idx, const number_kt val)
{
    tv_clear(&vimvars[idx].vv_tv);
    vimvars[idx].vv_type = kNvarNumber;
    vimvars[idx].vv_nr = val;
}

/// Set special v: variable to the given value
///
/// @param[in]  idx  Index of variable to set.
/// @param[in]  val  Value to set to.
void set_vim_var_special(const VimVarIndex idx,
                         const nvlvar_special_value_et val)
{
    tv_clear(&vimvars[idx].vv_tv);
    vimvars[idx].vv_type = kNvarSpecial;
    vimvars[idx].vv_special = val;
}

/// Set string v: variable to the given string
///
/// @param[in]  idx
/// Index of variable to set.
///
/// @param[in]  val
/// Value to set to. Will be copied.
///
/// @param[in]  len
/// Legth of that value or -1 in which case strlen() will be used.
void set_vim_var_string(const VimVarIndex idx,
                        const char *const val,
                        const ptrdiff_t len)
{
    tv_clear(&vimvars[idx].vv_di.di_tv);
    vimvars[idx].vv_type = kNvarString;

    if(val == NULL)
    {
        vimvars[idx].vv_str = NULL;
    }
    else if(len == -1)
    {
        vimvars[idx].vv_str = (uchar_kt *) xstrdup(val);
    }
    else
    {
        vimvars[idx].vv_str = (uchar_kt *) xstrndup(val, (size_t) len);
    }
}

/// Set list v: variable to the given list
///
/// @param[in]  idx
/// Index of variable to set.
///
/// @param[in,out]  val
/// Value to set to. Reference count will be incremented.
void set_vim_var_list(const VimVarIndex idx, list_st *const val)
{
    tv_clear(&vimvars[idx].vv_di.di_tv);
    vimvars[idx].vv_type = kNvarList;
    vimvars[idx].vv_list = val;

    if(val != NULL)
    {
        val->lv_refcount++;
    }
}

/// Set Dictionary v: variable to the given dictionary
///
/// @param[in]  idx
/// Index of variable to set.
///
/// @param[in,out]  val
/// Value to set to. Reference count will be incremented.
/// Also keys of the dictionary will be made read-only.
void set_vim_var_dict(const VimVarIndex idx, dict_st *const val)
{
    tv_clear(&vimvars[idx].vv_di.di_tv);
    vimvars[idx].vv_type = kNvarDict;
    vimvars[idx].vv_dict = val;

    if(val != NULL)
    {
        val->dv_refcount++;
        // Set readonly
        tv_dict_set_keys_readonly(val);
    }
}

/// Set v:register if needed.
void set_reg_var(int c)
{
    char regname;

    if(c == 0 || c == ' ')
    {
        regname = '"';
    }
    else
    {
        regname = c;
    }

    // Avoid free/alloc when the value is already right.
    if(vimvars[VV_REG].vv_str == NULL || vimvars[VV_REG].vv_str[0] != c)
    {
        set_vim_var_string(VV_REG, &regname, 1);
    }
}

/// Get or set v:exception. If "oldval" == NULL, return the current value.
/// Otherwise, restore the value to "oldval" and return NULL.
/// Must always be called in pairs to save and restore v:exception!  Does not
/// take care of memory allocations.
uchar_kt *v_exception(uchar_kt *oldval)
{
    if(oldval == NULL)
    {
        return vimvars[VV_EXCEPTION].vv_str;
    }

    vimvars[VV_EXCEPTION].vv_str = oldval;
    return NULL;
}

/// Get or set v:throwpoint.  If "oldval" == NULL, return the current value.
/// Otherwise, restore the value to "oldval" and return NULL.
/// Must always be called in pairs to save and restore v:throwpoint! Does not
/// take care of memory allocations.
uchar_kt *v_throwpoint(uchar_kt *oldval)
{
    if(oldval == NULL)
    {
        return vimvars[VV_THROWPOINT].vv_str;
    }

    vimvars[VV_THROWPOINT].vv_str = oldval;
    return NULL;
}

/// Set v:cmdarg.
/// If "eap" != NULL, use "eap" to generate the value and return the old value.
/// If "oldarg" != NULL, restore the value to "oldarg" and return NULL.
///
uchar_kt *set_cmdarg(exargs_st *eap, uchar_kt *oldarg)
{
    uchar_kt *oldval;
    uchar_kt *newval;
    oldval = vimvars[VV_CMDARG].vv_str;

    if(eap == NULL)
    {
        xfree(oldval);
        vimvars[VV_CMDARG].vv_str = oldarg;
        return NULL;
    }

    size_t len = 0;

    if(eap->force_bin == FORCE_BIN)
    {
        len = 6;
    }
    else if(eap->force_bin == FORCE_NOBIN)
    {
        len = 8;
    }

    if(eap->read_edit)
    {
        len += 7;
    }

    if(eap->force_ff != 0)
    {
        len += ustrlen(eap->cmd + eap->force_ff) + 6;
    }

    if(eap->force_enc != 0)
    {
        len += ustrlen(eap->cmd + eap->force_enc) + 7;
    }

    if(eap->bad_char != 0)
    {
        len += 7 + 4; // " ++bad=" + "keep" or "drop"
    }

    newval = xmalloc(len + 1);

    if(eap->force_bin == FORCE_BIN)
    {
        sprintf((char *)newval, " ++bin");
    }
    else if(eap->force_bin == FORCE_NOBIN)
    {
        sprintf((char *)newval, " ++nobin");
    }
    else
    {
        *newval = NUL;
    }

    if(eap->read_edit)
    {
        ustrcat(newval, " ++edit");
    }

    if(eap->force_ff != 0)
    {
        sprintf((char *)newval + ustrlen(newval), " ++ff=%s",
                eap->cmd + eap->force_ff);
    }

    if(eap->force_enc != 0)
    {
        sprintf((char *)newval + ustrlen(newval), " ++enc=%s",
                eap->cmd + eap->force_enc);
    }

    if(eap->bad_char == BAD_KEEP)
    {
        ustrcpy(newval + ustrlen(newval), " ++bad=keep");
    }
    else if(eap->bad_char == BAD_DROP)
    {
        ustrcpy(newval + ustrlen(newval), " ++bad=drop");
    }
    else if(eap->bad_char != 0)
    {
        sprintf((char *)newval + ustrlen(newval), " ++bad=%c", eap->bad_char);
    }

    vimvars[VV_CMDARG].vv_str = newval;
    return oldval;
}

/// Get the value of internal variable "name".
///
/// @param name
/// @param len          length of @b name
/// @param rettv        NULL when only checking existence
/// @param dip          non-NULL when typval's dict item is needed
/// @param verbose      may give error message
/// @param no_autoload  do not use script autoloading
///
/// @return  Return OK or FAIL.
///
static int get_var_tv(const char *name,
                      int len,
                      typval_st *rettv,
                      dictitem_st **dip,
                      int verbose,
                      int no_autoload)
{
    int ret = OK;
    typval_st *tv = NULL;
    dictitem_st  *v;
    v = find_var(name, (size_t)len, NULL, no_autoload);

    if(v != NULL)
    {
        tv = &v->di_tv;

        if(dip != NULL)
        {
            *dip = v;
        }
    }

    if(tv == NULL)
    {
        if(rettv != NULL && verbose)
        {
            emsgf(_("E121: Undefined variable: %.*s"), len, name);
        }

        ret = FAIL;
    }
    else if(rettv != NULL)
    {
        tv_copy(tv, rettv);
    }

    return ret;
}

/// Check if variable "name[len]" is a local variable or an argument.
/// If so, "*eval_lavars_used" is set to TRUE.
static void check_vars(const char *name, size_t len)
{
    if(eval_lavars_used == NULL)
    {
        return;
    }

    const char *varname;
    hashtable_st *ht = find_var_ht(name, len, &varname);

    if(ht == get_funccal_local_ht() || ht == get_funccal_args_ht())
    {
        if(find_var(name, len, NULL, true) != NULL)
        {
            *eval_lavars_used = true;
        }
    }
}

/// Handle expr[expr], expr[expr:expr] subscript and .name lookup.
/// Also handle function call with Funcref variable: func(expr)
/// Can all be combined: dict.func(expr)[idx]['func'](expr)
///
/// @param arg
/// @param rettv
/// @param evaluate  do more than finding the end
/// @param verbose   give error messages
static int handle_subscript(const char **const arg,
                            typval_st *rettv,
                            int evaluate,
                            int verbose)
{
    int ret = OK;
    dict_st *selfdict = NULL;
    uchar_kt *s;
    int len;
    typval_st functv;

    while(ret == OK
          && (**arg == '['
              || (**arg == '.' && rettv->v_type == kNvarDict)
              || (**arg == '(' && (!evaluate || tv_is_func(*rettv))))
          && !ascii_iswhite(*(*arg - 1)))
    {
        if(**arg == '(')
        {
            partial_st *pt = NULL;

            // need to copy the funcref so that we can clear rettv
            if(evaluate)
            {
                functv = *rettv;
                rettv->v_type = kNvarUnknown;

                // Invoke the function.  Recursive!
                if(functv.v_type == kNvarPartial)
                {
                    pt = functv.vval.v_partial;
                    s = partial_name(pt);
                }
                else
                {
                    s = functv.vval.v_string;
                }
            }
            else
            {
                s = (uchar_kt *)"";
            }

            ret = get_func_tv(s,
                              (int)ustrlen(s),
                              rettv,
                              (uchar_kt **)arg,
                              curwin->w_cursor.lnum,
                              curwin->w_cursor.lnum,
                              &len,
                              evaluate,
                              pt,
                              selfdict);

            // Clear the funcref afterwards, so that deleting it while
            // evaluating the arguments is possible (see test55).
            if(evaluate)
            {
                tv_clear(&functv);
            }

            // Stop the expression evaluation when immediately aborting on
            // error, or when an interrupt occurred or an exception was thrown
            // but not caught.
            if(aborting())
            {
                if(ret == OK)
                {
                    tv_clear(rettv);
                }

                ret = FAIL;
            }

            tv_dict_unref(selfdict);
            selfdict = NULL;
        }
        else
        {
            // **arg == '[' || **arg == '.'
            tv_dict_unref(selfdict);

            if(rettv->v_type == kNvarDict)
            {
                selfdict = rettv->vval.v_dict;

                if(selfdict != NULL)
                {
                    ++selfdict->dv_refcount;
                }
            }
            else
            {
                selfdict = NULL;
            }

            if(eval_index((uchar_kt **)arg, rettv, evaluate, verbose) == FAIL)
            {
                tv_clear(rettv);
                ret = FAIL;
            }
        }
    }

    // Turn "dict.Func" into a partial for "Func" bound to "dict".
    if(selfdict != NULL && tv_is_func(*rettv))
    {
        set_selfdict(rettv, selfdict);
    }

    tv_dict_unref(selfdict);
    return ret;
}

void set_selfdict(typval_st *rettv, dict_st *selfdict)
{
    // Don't do this when "dict.Func" is already a partial that was bound
    // explicitly (pt_auto is false).
    if(rettv->v_type == kNvarPartial
       && !rettv->vval.v_partial->pt_auto
       && rettv->vval.v_partial->pt_dict != NULL)
    {
        return;
    }

    uchar_kt *fname;
    uchar_kt *tofree = NULL;
    ufunc_st *fp;
    uchar_kt fname_buf[FLEN_FIXED + 1];
    int error;

    if(rettv->v_type == kNvarPartial
       && rettv->vval.v_partial->pt_func != NULL)
    {
        fp = rettv->vval.v_partial->pt_func;
    }
    else
    {
        fname = rettv->v_type == kNvarUfunc
                || rettv->v_type == kNvarString
                   ? rettv->vval.v_string
                   : rettv->vval.v_partial->pt_name;

        // Translate "s:func" to the stored function name.
        fname = fname_trans_sid(fname, fname_buf, &tofree, &error);
        fp = find_func(fname);
        xfree(tofree);
    }

    // Turn "dict.Func" into a partial for "Func" with "dict".
    if(fp != NULL && (fp->uf_flags & FC_DICT))
    {
        partial_st *pt = (partial_st *)xcalloc(1, sizeof(partial_st));

        if(pt != NULL)
        {
            pt->pt_refcount = 1;
            pt->pt_dict = selfdict;
            (selfdict->dv_refcount)++;
            pt->pt_auto = true;

            if(rettv->v_type == kNvarUfunc || rettv->v_type == kNvarString)
            {
                // Just a function:
                // Take over the function name and use selfdict.
                pt->pt_name = rettv->vval.v_string;
            }
            else
            {
                int i;
                partial_st *ret_pt = rettv->vval.v_partial;

                // Partial: copy the function name, use selfdict and copy
                // args. Can't take over name or args, the partial might
                // be referenced elsewhere.
                if(ret_pt->pt_name != NULL)
                {
                    pt->pt_name = ustrdup(ret_pt->pt_name);
                    func_ref(pt->pt_name);
                }
                else
                {
                    pt->pt_func = ret_pt->pt_func;
                    func_ptr_ref(pt->pt_func);
                }

                if(ret_pt->pt_argc > 0)
                {
                    size_t arg_size = sizeof(typval_st) * ret_pt->pt_argc;
                    pt->pt_argv = (typval_st *)xmalloc(arg_size);

                    if(pt->pt_argv == NULL)
                    {
                        // out of memory: drop the arguments
                        pt->pt_argc = 0;
                    }
                    else
                    {
                        pt->pt_argc = ret_pt->pt_argc;

                        for(i = 0; i < pt->pt_argc; i++)
                        {
                            tv_copy(&ret_pt->pt_argv[i], &pt->pt_argv[i]);
                        }
                    }
                }

                partial_unref(ret_pt);
            }

            rettv->v_type = kNvarPartial;
            rettv->vval.v_partial = pt;
        }
    }
}

// Find variable "name" in the list of variables.
// Return a pointer to it if found, NULL if not found.
// Careful: "a:0" variables don't have a name.
// When "htp" is not NULL we are writing to the variable, set "htp" to the
// hashtable_st used.
static dictitem_st *find_var(const char *const name,
                            const size_t name_len,
                            hashtable_st **htp,
                            int no_autoload)
{
    const char *varname;
    hashtable_st *const ht = find_var_ht(name, name_len, &varname);

    if(htp != NULL)
    {
        *htp = ht;
    }

    if(ht == NULL)
    {
        return NULL;
    }

    dictitem_st *const ret =
        find_var_in_ht(ht,
                       *name,
                       varname,
                       name_len - (size_t)(varname - name),
                       no_autoload || htp != NULL);

    if(ret != NULL)
    {
        return ret;
    }

    // Search in parent scope for lambda
    return find_var_in_scoped_ht(name, name_len, no_autoload || htp != NULL);
}

/// Find variable in hashtab
///
/// @param[in]  ht           Hashtab to find variable in.
/// @param[in]  htname       Hashtab name (first character).
/// @param[in]  varname      Variable name.
/// @param[in]  varname_len  Variable name length.
/// @param[in]  no_autoload  If true then autoload scripts will not be sourced
///                          if autoload variable was not found.
///
/// @return
/// pointer to the dictionary item with the found
/// variable or NULL if it was not found.
static dictitem_st *find_var_in_ht(hashtable_st *const ht,
                                  int htname,
                                  const char *const varname,
                                  const size_t varname_len,
                                  int no_autoload)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    hashitem_st *hi;

    if(varname_len == 0)
    {
        // Must be something like "s:", otherwise "ht" would be NULL.
        switch(htname)
        {
            case 's':
                return (dictitem_st *)&SCRIPT_SV(current_SID)->sv_var;

            case 'g':
                return (dictitem_st *)&globvars_var;

            case 'v':
                return (dictitem_st *)&vimvars_var;

            case 'b':
                return (dictitem_st *)&curbuf->b_bufvar;

            case 'w':
                return (dictitem_st *)&curwin->w_winvar;

            case 't':
                return (dictitem_st *)&curtab->tp_winvar;

            case 'l':
                return (current_funccal == NULL
                        ? NULL : (dictitem_st *)&current_funccal->l_vars_var);

            case 'a':
                return (current_funccal == NULL
                        ? NULL : (dictitem_st *)&current_funccal->l_avars_var);
        }

        return NULL;
    }

    hi = hash_find_len(ht, varname, varname_len);

    if(HASHITEM_EMPTY(hi))
    {
        // For global variables we may try auto-loading the script. If it
        // worked find the variable again. Don't auto-load a script if it was
        // loaded already, otherwise it would be loaded every time when
        // checking if a function name is a Funcref variable.
        if(ht == &globvarht && !no_autoload)
        {
            // Note: script_autoload() may make "hi" invalid.
            // It must either be obtained again or not used.
            if(!script_autoload(varname, varname_len, false) || aborting())
            {
                return NULL;
            }

            hi = hash_find_len(ht, varname, varname_len);
        }

        if(HASHITEM_EMPTY(hi))
        {
            return NULL;
        }
    }

    return TV_DICT_HI2DI(hi);
}

// Get function call environment based on backtrace debug level
static func_call_st *get_funccal(void)
{
    func_call_st *funccal = current_funccal;

    if(debug_backtrace_level > 0)
    {
        for(int i = 0; i < debug_backtrace_level; i++)
        {
            func_call_st *temp_funccal = funccal->caller;

            if(temp_funccal)
            {
                funccal = temp_funccal;
            }
            else
            {
                // backtrace level overflow. reset to max
                debug_backtrace_level = i;
            }
        }
    }

    return funccal;
}

/// Return the hashtable used for argument in the current funccal.
/// Return NULL if there is no current funccal.
static hashtable_st *get_funccal_args_ht(void)
{
    if(current_funccal == NULL)
    {
        return NULL;
    }

    return &get_funccal()->l_avars.dv_hashtab;
}

/// Return the hashtable used for local variables in the current funccal.
/// Return NULL if there is no current funccal.
static hashtable_st *get_funccal_local_ht(void)
{
    if(current_funccal == NULL)
    {
        return NULL;
    }

    return &get_funccal()->l_vars.dv_hashtab;
}

/// Find the dict and hashtable used for a variable
///
/// @param[in]  name
/// Variable name, possibly with scope prefix.
///
/// @param[in]  name_len
/// Variable name length.
///
/// @param[out]  varname
/// Will be set to the start of the name without scope prefix.
///
/// @param[out]  d
/// Scope dictionary.
///
/// @return Scope hashtab, NULL if name is not valid.
static hashtable_st *find_var_ht_dict(const char *name,
                                   const size_t name_len,
                                   const char **varname,
                                   dict_st **d)
{
    hashitem_st *hi;
    *d = NULL;

    if(name_len == 0)
    {
        return NULL;
    }

    if(name_len == 1 || (name_len >= 2 && name[1] != ':'))
    {
        // name has implicit scope
        if(name[0] == ':' || name[0] == AUTOLOAD_CHAR)
        {
            // The name must not start with a colon or #.
            return NULL;
        }

        *varname = name;
        // "version" is "v:version" in all scopes
        hi = hash_find_len(&compat_hashtab, name, name_len);

        if(!HASHITEM_EMPTY(hi))
        {
            return &compat_hashtab;
        }

        if(current_funccal == NULL)
        {
            *d = &globvardict;
        }
        else
        {
            *d = &get_funccal()->l_vars; // l: variable
        }

        goto end;
    }

    *varname = name + 2;

    if(*name == 'g') // global variable
    {
        *d = &globvardict;
    }
    else if(name_len > 2
            && (memchr(name + 2, ':', name_len - 2) != NULL
                || memchr(name + 2, AUTOLOAD_CHAR, name_len - 2) != NULL))
    {
        // There must be no ':' or '#' in the
        // rest of the name if g: was not used
        return NULL;
    }

    if(*name == 'b') // buffer variable
    {
        *d = curbuf->b_vars;
    }
    else if(*name == 'w') // window variable
    {
        *d = curwin->w_vars;
    }
    else if(*name == 't') // tab page variable
    {
        *d = curtab->tp_vars;
    }
    else if(*name == 'v') // v: variable
    {
        *d = &vimvardict;
    }
    else if(*name == 'a' && current_funccal != NULL) // function argument
    {
        *d = &get_funccal()->l_avars;
    }
    else if(*name == 'l' && current_funccal != NULL) // local variable
    {
        *d = &get_funccal()->l_vars;
    }
    else if(*name == 's' // script variable
            && current_SID > 0 && current_SID <= ga_scripts.ga_len)
    {
        *d = &SCRIPT_SV(current_SID)->sv_dict;
    }

end:

    return *d ? &(*d)->dv_hashtab : NULL;
}

/// Find the hashtable used for a variable
///
/// @param[in]  name
/// Variable name, possibly with scope prefix.
///
/// @param[in]  name_len
/// Variable name length.
///
/// @param[out]  varname
/// Will be set to the start of the name without scope prefix.
///
/// @return
/// Scope hashtab, NULL if name is not valid.
static hashtable_st *find_var_ht(const char *name,
                              const size_t name_len,
                              const char **varname)
{
    dict_st *d;
    return find_var_ht_dict(name, name_len, varname, &d);
}

/// Get the string value of a (global/local) variable.
/// Note: see tv_get_string() for how long the pointer remains valid.
/// Returns NULL when it doesn't exist.
uchar_kt *get_var_value(const char *const name)
{
    dictitem_st  *v;
    v = find_var(name, strlen(name), NULL, false);

    if(v == NULL)
    {
        return NULL;
    }

    return (uchar_kt *)tv_get_string(&v->di_tv);
}

/// Allocate a new hashtab for a sourced script. It will be used while
/// sourcing this script and when executing functions defined in the script.
void new_script_vars(script_id_kt id)
{
    hashtable_st *ht;
    scriptvar_st *sv;
    ga_grow(&ga_scripts, (int)(id - ga_scripts.ga_len));

    {
        // Re-allocating ga_data means that an ht_array pointing to
        // ht_smallarray becomes invalid. We can recognize this: ht_mask is
        // at its init value. Also reset "v_dict", it's always the same.
        for(int i = 1; i <= ga_scripts.ga_len; ++i)
        {
            ht = &SCRIPT_VARS(i);

            if(ht->ht_mask == HT_INIT_SIZE - 1)
            {
                ht->ht_array = ht->ht_smallarray;
            }

            sv = SCRIPT_SV(i);
            sv->sv_var.di_tv.vval.v_dict = &sv->sv_dict;
        }

        while(ga_scripts.ga_len < id)
        {
            sv = SCRIPT_SV(ga_scripts.ga_len + 1) = xcalloc(1, sizeof(scriptvar_st));

            init_var_dict(&sv->sv_dict, &sv->sv_var, VAR_SCOPE);
            ++ga_scripts.ga_len;
        }
    }
}

/// Initialize dictionary @b dict as a scope and
/// set variable @b dict_var to point to it.
void init_var_dict(dict_st *dict, scope_dict_st *dict_var, int scope)
{
    hash_init(&dict->dv_hashtab);

    dict->dv_lock = kNvlVarUnlocked;
    dict->dv_scope = scope;
    dict->dv_refcount = DO_NOT_FREE_CNT;
    dict->dv_copyID = 0;
    dict_var->di_tv.vval.v_dict = dict;
    dict_var->di_tv.v_type = kNvarDict;
    dict_var->di_tv.v_lock = kNvlVarFixed;
    dict_var->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
    dict_var->di_key[0] = NUL;

    queue_init(&dict->watchers);
}

/// Unreference a dictionary initialized by init_var_dict().
void unref_var_dict(dict_st *dict)
{
    // Now the dict needs to be freed if no one else is
    // using it, go back to normal reference counting.
    dict->dv_refcount -= DO_NOT_FREE_CNT - 1;
    tv_dict_unref(dict);
}

/// Clean up a list of internal variables.
/// Frees all allocated variables and the value they contain.
/// Clears hashtab "ht", does not free it.
void vars_clear(hashtable_st *ht)
{
    vars_clear_ext(ht, TRUE);
}

/// Like vars_clear(), but only free the value if "free_val" is TRUE.
static void vars_clear_ext(hashtable_st *ht, int free_val)
{
    int todo;
    hashitem_st *hi;
    dictitem_st *v;
    hash_lock(ht);
    todo = (int)ht->ht_used;

    for(hi = ht->ht_array; todo > 0; ++hi)
    {
        if(!HASHITEM_EMPTY(hi))
        {
            --todo;
            // Free the variable. Don't remove it from the hashtab,
            // ht_array might change then.
            // hash_clear() takes care of it later.
            v = TV_DICT_HI2DI(hi);

            if(free_val)
            {
                tv_clear(&v->di_tv);
            }

            if(v->di_flags & DI_FLAGS_ALLOC)
            {
                xfree(v);
            }
        }
    }

    hash_clear(ht);
    ht->ht_used = 0;
}

/// Delete a variable from hashtab "ht" at item "hi".
/// Clear the variable value and free the dictitem.
static void delete_var(hashtable_st *ht, hashitem_st *hi)
{
    dictitem_st *di = TV_DICT_HI2DI(hi);
    hash_remove(ht, hi);
    tv_clear(&di->di_tv);
    xfree(di);
}

/// List the value of one internal variable.
static void list_one_var(dictitem_st *v, const char *prefix, int *first)
{
    char *const s = encode_tv2echo(&v->di_tv, NULL);

    list_one_var_a(prefix,
                   (const char *)v->di_key,
                   ustrlen(v->di_key),
                   v->di_tv.v_type,
                   (s == NULL ? "" : s),
                   first);
    xfree(s);
}

/// @param[in] name_len
/// Length of the name.
/// May be -1, in this case strlen() will be used.
///
/// @param[in,out] first
/// When true clear rest of screen and set to false.
static void list_one_var_a(const char *prefix,
                           const char *name,
                           const ptrdiff_t name_len,
                           const int type,
                           const char *string,
                           int *first)
{
    // don't use msg() or msg_attr() to avoid overwriting "v:statusmsg"
    msg_start();
    msg_puts(prefix);

    if(name != NULL) // "a:" vars don't have a name stored
    {
        msg_puts_attr_len(name, name_len, 0);
    }

    msg_putchar(' ');
    msg_advance(22);

    if(type == kNvarNumber)
    {
        msg_putchar('#');
    }
    else if(type == kNvarUfunc || type == kNvarPartial)
    {
        msg_putchar('*');
    }
    else if(type == kNvarList)
    {
        msg_putchar('[');

        if(*string == '[')
        {
            ++string;
        }
    }
    else if(type == kNvarDict)
    {
        msg_putchar('{');

        if(*string == '{')
        {
            ++string;
        }
    }
    else
    {
        msg_putchar(' ');
    }

    msg_outtrans((uchar_kt *)string);

    if(type == kNvarUfunc || type == kNvarPartial)
    {
        msg_puts("()");
    }

    if(*first)
    {
        msg_clr_eos();
        *first = FALSE;
    }
}

/// Set variable to the given value
///
/// If the variable already exists, the value is updated. Otherwise the variable
/// is created.
///
/// @param[in]  name      Variable name to set.
/// @param[in]  name_len  Length of the variable name.
/// @param  tv  Variable  value.
/// @param[in]  copy      True if value in tv is to be copied.
static void set_var(const char *name,
                    const size_t name_len,
                    typval_st *const tv,
                    const bool copy)
FUNC_ATTR_NONNULL_ALL
{
    dictitem_st *v;
    hashtable_st *ht;
    dict_st *dict;

    const char *varname;
    ht = find_var_ht_dict(name, name_len, &varname, &dict);
    const bool watched = tv_dict_is_watched(dict);

    if(ht == NULL || *varname == NUL)
    {
        EMSG2(_(e_illvar), name);
        return;
    }

    v = find_var_in_ht(ht, 0, varname, name_len - (size_t)(varname - name), true);

    // Search in parent scope which is possible to reference from lambda
    if(v == NULL)
    {
        v = find_var_in_scoped_ht((const char *)name, name_len, true);
    }

    if(tv_is_func(*tv) && !var_check_func_name(name, v == NULL))
    {
        return;
    }

    typval_st oldtv = TV_INITIAL_VALUE;

    if(v != NULL)
    {
        // existing variable, need to clear the value
        if(var_check_ro(v->di_flags, name, name_len)
           || tv_check_lock(v->di_tv.v_lock, name, name_len))
        {
            return;
        }

        // Handle setting internal v: variables separately
        // where needed to prevent changing the type.
        if(ht == &vimvarht)
        {
            if(v->di_tv.v_type == kNvarString)
            {
                xfree(v->di_tv.vval.v_string);

                if(copy || tv->v_type != kNvarString)
                {
                    v->di_tv.vval.v_string =
                        (uchar_kt *)xstrdup(tv_get_string(tv));
                }
                else
                {
                    // Take over the string to avoid an extra alloc/free.
                    v->di_tv.vval.v_string = tv->vval.v_string;
                    tv->vval.v_string = NULL;
                }

                return;
            }
            else if(v->di_tv.v_type == kNvarNumber)
            {
                v->di_tv.vval.v_number = tv_get_number(tv);

                if(strcmp(varname, "searchforward") == 0)
                {
                    set_search_direction(v->di_tv.vval.v_number ? '/' : '?');
                }
                else if(strcmp(varname, "hlsearch") == 0)
                {
                    no_hlsearch = !v->di_tv.vval.v_number;
                    redraw_all_later(SOME_VALID);
                }

                return;
            }
            else if(v->di_tv.v_type != tv->v_type)
            {
                EMSG2(_(e_intern2), "set_var()");
            }
        }

        if(watched)
        {
            tv_copy(&v->di_tv, &oldtv);
        }

        tv_clear(&v->di_tv);
    }
    else  // Add a new variable.
    {
        // Can't add "v:" variable.
        if(ht == &vimvarht)
        {
            emsgf(_(e_illvar), name);
            return;
        }

        // Make sure the variable name is valid.
        if(!valid_varname(varname))
        {
            return;
        }

        v = xmalloc(sizeof(dictitem_st) + strlen(varname));
        ustrcpy(v->di_key, varname);

        if(tv_dict_add(dict, v) == FAIL)
        {
            xfree(v);
            return;
        }

        v->di_flags = DI_FLAGS_ALLOC;
    }

    if(copy || tv->v_type == kNvarNumber || tv->v_type == kNvarFloat)
    {
        tv_copy(tv, &v->di_tv);
    }
    else
    {
        v->di_tv = *tv;
        v->di_tv.v_lock = 0;
        tv_init(tv);
    }

    if(watched)
    {
        if(oldtv.v_type == kNvarUnknown)
        {
            tv_dict_watcher_notify(dict, (char *)v->di_key, &v->di_tv, NULL);
        }
        else
        {
            tv_dict_watcher_notify(dict, (char *)v->di_key, &v->di_tv, &oldtv);
            tv_clear(&oldtv);
        }
    }
}

/// Check whether variable is read-only (DI_FLAGS_RO, DI_FLAGS_RO_SBX)
///
/// Also gives an error message.
///
/// @param[in]  flags
/// di_flags attribute value.
///
/// @param[in]  name
/// Variable name, for use in error message.
///
/// @param[in]  name_len
/// Variable name length. Use #TV_TRANSLATE to translate
/// variable name and compute the length. Use #TV_CSTRING
/// to compute the length with strlen() without translating.
///
/// Both #TV_… values are used for optimization purposes:
/// variable name with its length is needed only in case
/// of error, when no error occurs computing them is
/// a waste of CPU resources. This especially applies to gettext.
///
/// @return
/// True if variable is read-only: either always or in sandbox when
/// sandbox is enabled, false otherwise.
bool var_check_ro(const int flags,
                  const char *name,
                  size_t name_len)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    const char *error_message = NULL;

    if(flags & DI_FLAGS_RO)
    {
        error_message = N_(e_readonlyvar);
    }
    else if((flags & DI_FLAGS_RO_SBX) && sandbox)
    {
        error_message = N_("E794: Cannot set variable in the sandbox: \"%.*s\"");
    }

    if(error_message == NULL)
    {
        return false;
    }

    if(name_len == TV_TRANSLATE)
    {
        name = _(name);
        name_len = strlen(name);
    }
    else if(name_len == TV_CSTRING)
    {
        name_len = strlen(name);
    }

    emsgf(_(error_message), (int)name_len, name);

    return true;
}

/// Check whether variable is fixed (DI_FLAGS_FIX)
///
/// Also gives an error message.
///
/// @param[in]  flags     di_flags attribute value.
/// @param[in]  name      Variable name, for use in error message.
/// @param[in]  name_len  Variable name length. Use #TV_TRANSLATE to translate
///                       variable name and compute the length. Use #TV_CSTRING
///                       to compute the length with strlen() without
///                       translating.
///
///                       Both #TV_… values are used for optimization purposes:
///                       variable name with its length is needed only in case
///                       of error, when no error occurs computing them is
///                       a waste of CPU resources. This especially applies to
///                       gettext.
///
/// @return True if variable is fixed, false otherwise.
static bool var_check_fixed(const int flags,
                            const char *name,
                            size_t name_len)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(flags & DI_FLAGS_FIX)
    {
        if(name_len == TV_TRANSLATE)
        {
            name = _(name);
            name_len = strlen(name);
        }
        else if(name_len == TV_CSTRING)
        {
            name_len = strlen(name);
        }

        emsgf(_("E795: Cannot delete variable %.*s"), (int)name_len, name);
        return true;
    }

    return false;
}

// TODO(ZyX-I): move to eval/expressions

/// Check if name is a valid name to assign funcref to
///
/// @param[in]  name     Possible function/funcref name.
/// @param[in]  new_var  True if it is a name for a variable.
///
/// @return false in case of error, true in case of success. Also gives an
///         error message if appropriate.
bool var_check_func_name(const char *const name, const bool new_var)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    // Allow for w: b: s: and t:.
    if(!(ustrchr((uchar_kt *)"wbst", name[0]) != NULL && name[1] == ':')
       && !ASCII_ISUPPER((name[0] != NUL && name[1] == ':') ? name[2] : name[0]))
    {
        EMSG2(_("E704: Funcref variable name must start with a capital: %s"), name);
        return false;
    }

    // Don't allow hiding a function.  When "v" is not NULL we might be
    // assigning another function to the same var, the type is checked
    // below.
    if(new_var && function_exists((const char *)name, false))
    {
        EMSG2(_("E705: Variable name conflicts with existing function: %s"),
              name);
        return false;
    }

    return true;
}

/// Check if a variable name is valid
///
/// @param[in]  varname  Variable name to check.
///
/// @return false when variable name is not valid, true when it is. Also gives
///         an error message if appropriate.
///
/// @todo move to eval/expressions
bool valid_varname(const char *varname)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    for(const char *p = varname; *p != NUL; p++)
    {
        if(!eval_isnamec1((int)(uint8_t)(*p))
           && (p == varname || !ascii_isdigit(*p))
           && *p != AUTOLOAD_CHAR)
        {
            emsgf(_(e_illvar), varname);
            return false;
        }
    }

    return true;
}

/// Make a copy of an item
///
/// Lists and Dictionaries are also copied.
///
/// @param[in]  conv    If not NULL, convert all copied strings.
/// @param[in]  from    Value to copy.
/// @param[out]  to     Location where to copy to.
/// @param[in]  deep    If true, use copy the container and all of the contained
///                     containers (nested).
/// @param[in]  copyID  If non-zero then when container is referenced more then
///                     once then copy of it that was already done is used. E.g.
///                     when copying list `list = [list2, list2]` (`list[0] is
///                     list[1]`) var_item_copy with zero copyID will emit
///                     a copy with (`copy[0] isnot copy[1]`), with non-zero it
///                     will emit a copy with (`copy[0] is copy[1]`) like in the
///                     original list. Not used when deep is false.
int var_item_copy(const vimconv_st *const conv,
                  typval_st *const from,
                  typval_st *const to,
                  const bool deep,
                  const int copyID)
FUNC_ATTR_NONNULL_ARG(2, 3)
{
    static int recurse = 0;
    int ret = OK;

    if(recurse >= DICT_MAXNEST)
    {
        EMSG(_("E698: variable nested too deep for making a copy"));
        return FAIL;
    }

    ++recurse;

    switch(from->v_type)
    {
        case kNvarNumber:
        case kNvarFloat:
        case kNvarUfunc:
        case kNvarPartial:
        case kNvarSpecial:
            tv_copy(from, to);
            break;

        case kNvarString:
            if(conv == NULL
               || conv->vc_type == CONV_NONE
               || from->vval.v_string == NULL)
            {
                tv_copy(from, to);
            }
            else
            {
                to->v_type = kNvarString;
                to->v_lock = 0;

                if((to->vval.v_string = string_convert((vimconv_st *)conv,
                                                       from->vval.v_string,
                                                       NULL)) == NULL)
                {
                    to->vval.v_string = ustrdup(from->vval.v_string);
                }
            }

            break;

        case kNvarList:
            to->v_type = kNvarList;
            to->v_lock = 0;

            if(from->vval.v_list == NULL)
            {
                to->vval.v_list = NULL;
            }
            else if(copyID != 0 && from->vval.v_list->lv_copyID == copyID)
            {
                // use the copy made earlier
                to->vval.v_list = from->vval.v_list->lv_copylist;
                ++to->vval.v_list->lv_refcount;
            }
            else
            {
                to->vval.v_list =
                    tv_list_copy(conv, from->vval.v_list, deep, copyID);
            }

            if(to->vval.v_list == NULL && from->vval.v_list != NULL)
            {
                ret = FAIL;
            }

            break;

        case kNvarDict:
            to->v_type = kNvarDict;
            to->v_lock = 0;

            if(from->vval.v_dict == NULL)
            {
                to->vval.v_dict = NULL;
            }
            else if(copyID != 0 && from->vval.v_dict->dv_copyID == copyID)
            {
                // use the copy made earlier
                to->vval.v_dict = from->vval.v_dict->dv_copydict;
                ++to->vval.v_dict->dv_refcount;
            }
            else
            {
                to->vval.v_dict =
                    tv_dict_copy(conv, from->vval.v_dict, deep, copyID);
            }

            if(to->vval.v_dict == NULL && from->vval.v_dict != NULL)
            {
                ret = FAIL;
            }

            break;

        case kNvarUnknown:
            EMSG2(_(e_intern2), "var_item_copy(UNKNOWN)");
            ret = FAIL;
    }

    --recurse;
    return ret;
}

/// - ":echo expr1 ..."
///   print each argument separated with a space, add a newline at the end.
/// - ":echon expr1 ..."
///   print each argument plain.
void ex_echo(exargs_st *eap)
{
    uchar_kt *arg = eap->arg;
    typval_st rettv;
    bool needclr = true;
    bool atstart = true;

    if(eap->skip)
    {
        ++emsg_skip;
    }

    while(*arg != NUL && *arg != '|' && *arg != '\n' && !got_int)
    {
        // If eval_lev_1() causes an error message the text from
        // the command may still need to be cleared. E.g., "echo 22,44".
        need_clr_eos = needclr;
        {
            uchar_kt *p = arg;

            if(eval_lev_1(&arg, &rettv, !eap->skip) == FAIL)
            {
                // Report the invalid expression unless the expression
                // evaluation has been cancelled due to an aborting error,
                // an interrupt, or an exception.
                if(!aborting())
                {
                    EMSG2(_(e_invexpr2), p);
                }

                need_clr_eos = false;
                break;
            }

            need_clr_eos = false;
        }

        if(!eap->skip)
        {
            if(atstart)
            {
                atstart = false;

                // Call msg_start() after eval_lev_1(), evaluating
                // the expression may cause a message to appear.
                if(eap->cmdidx == CMD_echo)
                {
                    // Mark the saved text as finishing the line, so that what
                    // follows is displayed on a new line when scrolling back
                    // at the more prompt.
                    msg_sb_eol();
                    msg_start();
                }
            }
            else if(eap->cmdidx == CMD_echo)
            {
                msg_puts_attr(" ", echo_attr);
            }

            char *tofree = encode_tv2echo(&rettv, NULL);
            const char *p = tofree;

            if(p != NULL)
            {
                for(; *p != NUL && !got_int; ++p)
                {
                    if(*p == '\n' || *p == '\r' || *p == TAB)
                    {
                        if(*p != TAB && needclr)
                        {
                            // remove any text still there from the command
                            msg_clr_eos();
                            needclr = false;
                        }

                        msg_putchar_attr((uint8_t)(*p), echo_attr);
                    }
                    else
                    {
                        int i = (*mb_ptr2len)((const uchar_kt *)p);
                        (void)msg_outtrans_len_attr((uchar_kt *)p, i, echo_attr);
                        p += i - 1;
                    }
                }
            }

            xfree(tofree);
        }

        tv_clear(&rettv);
        arg = skipwhite(arg);
    }

    eap->nextcmd = check_nextcmd(arg);

    if(eap->skip)
    {
        --emsg_skip;
    }
    else
    {
        // remove text that may still be there from the command
        if(needclr)
        {
            msg_clr_eos();
        }

        if(eap->cmdidx == CMD_echo)
        {
            msg_end();
        }
    }
}

/// ":echohl {name}".
void ex_echohl(exargs_st *eap)
{
    int id;
    id = syn_name2id(eap->arg);

    if(id == 0)
    {
        echo_attr = 0;
    }
    else
    {
        echo_attr = syn_id2attr(id);
    }
}

/// - ":execute expr1 ..."  execute the result of an expression.
/// - ":echomsg expr1 ..."  Print a message
/// - ":echoerr expr1 ..."  Print an error
///
/// Each gets spaces around each argument and a newline at the end for
/// echo commands
void ex_execute(exargs_st *eap)
{
    uchar_kt *arg = eap->arg;
    typval_st rettv;
    int ret = OK;
    uchar_kt *p;
    garray_st ga;
    int save_did_emsg;
    ga_init(&ga, 1, 80);

    if(eap->skip)
    {
        ++emsg_skip;
    }

    while(*arg != NUL && *arg != '|' && *arg != '\n')
    {
        p = arg;

        if(eval_lev_1(&arg, &rettv, !eap->skip) == FAIL)
        {
            // Report the invalid expression unless the expression evaluation
            // has been cancelled due to an aborting error, an interrupt,
            // or an exception.
            if(!aborting())
            {
                EMSG2(_(e_invexpr2), p);
            }

            ret = FAIL;
            break;
        }

        if(!eap->skip)
        {
            const char *const argstr = tv_get_string(&rettv);
            const size_t len = strlen(argstr);
            ga_grow(&ga, len + 2);

            if(!GA_EMPTY(&ga))
            {
                ((uchar_kt *)(ga.ga_data))[ga.ga_len++] = ' ';
            }

            memcpy((uchar_kt *)(ga.ga_data) + ga.ga_len, argstr, len + 1);
            ga.ga_len += len;
        }

        tv_clear(&rettv);
        arg = skipwhite(arg);
    }

    if(ret != FAIL && ga.ga_data != NULL)
    {
        if(eap->cmdidx == CMD_echomsg)
        {
            MSG_ATTR(ga.ga_data, echo_attr);
            ui_flush();
        }
        else if(eap->cmdidx == CMD_echoerr)
        {
            // We don't want to abort following
            // commands, restore did_emsg.
            save_did_emsg = did_emsg;
            EMSG((uchar_kt *)ga.ga_data);

            if(!force_abort)
            {
                did_emsg = save_did_emsg;
            }
        }
        else if(eap->cmdidx == CMD_execute)
        {
            do_cmdline((uchar_kt *)ga.ga_data,
                       eap->getline,
                       eap->cookie,
                       DOCMD_NOWAIT|DOCMD_VERBOSE);
        }
    }

    ga_clear(&ga);

    if(eap->skip)
    {
        --emsg_skip;
    }

    eap->nextcmd = check_nextcmd(arg);
}

/// Skip over the name of an option: "&option", "&g:option" or "&l:option".
/// "arg" points to the "&" or '+' when called, to "option" when returning.
/// Returns NULL when no option name found. Otherwise pointer to the char
/// after the option name.
static const char *find_option_end(const char **const arg,
                                   int *const opt_flags)
{
    const char *p = *arg;
    ++p;

    if(*p == 'g' && p[1] == ':')
    {
        *opt_flags = kOptSetGlobal;
        p += 2;
    }
    else if(*p == 'l' && p[1] == ':')
    {
        *opt_flags = kOptSetLocal;
        p += 2;
    }
    else
    {
        *opt_flags = 0;
    }

    if(!ASCII_ISALPHA(*p))
    {
        return NULL;
    }

    *arg = p;

    if(p[0] == 't' && p[1] == '_' && p[2] != NUL && p[3] != NUL)
    {
        p += 4; // termcap option
    }
    else
    {
        while(ASCII_ISALPHA(*p))
        {
            ++p;
        }
    }

    return p;
}

/// ":function"
void ex_function(exargs_st *eap)
{
    uchar_kt *theline;
    int c;
    int saved_did_emsg;
    int saved_wait_return = need_wait_return;
    uchar_kt *name = NULL;
    uchar_kt *p;
    uchar_kt *arg;
    uchar_kt *line_arg = NULL;
    garray_st newargs;
    garray_st newlines;
    int varargs = false;
    int flags = 0;
    ufunc_st *fp;
    bool overwrite = false;
    int indent;
    int nesting;
    uchar_kt *skip_until = NULL;
    dictitem_st  *v;
    funcdict_st fudi;
    static int func_nr = 0; // number for nameless function
    int paren;
    hashtable_st *ht;
    int todo;
    hashitem_st  *hi;
    int sourcing_lnum_off;

    // ":function" without argument: list functions.
    if(ends_excmd(*eap->arg))
    {
        if(!eap->skip)
        {
            todo = (int)func_hashtab.ht_used;

            for(hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
            {
                if(!HASHITEM_EMPTY(hi))
                {
                    --todo;
                    fp = HI2UF(hi);

                    if(!func_name_refcount(fp->uf_name))
                    {
                        list_func_head(fp, false);
                    }
                }
            }
        }

        eap->nextcmd = check_nextcmd(eap->arg);
        return;
    }

    // ":function /pat": list functions matching pattern.
    if(*eap->arg == '/')
    {
        p = skip_regexp(eap->arg + 1, '/', TRUE, NULL);

        if(!eap->skip)
        {
            regmatch_st regmatch;
            c = *p;
            *p = NUL;
            regmatch.regprog = regexp_compile(eap->arg + 1, RE_MAGIC);
            *p = c;

            if(regmatch.regprog != NULL)
            {
                regmatch.rm_ic = p_ic;
                todo = (int)func_hashtab.ht_used;

                for(hi = func_hashtab.ht_array; todo > 0 && !got_int; ++hi)
                {
                    if(!HASHITEM_EMPTY(hi))
                    {
                        --todo;
                        fp = HI2UF(hi);

                        if(!isdigit(*fp->uf_name)
                           && vim_regexec(&regmatch, fp->uf_name, 0))
                        {
                            list_func_head(fp, FALSE);
                        }
                    }
                }

                vim_regfree(regmatch.regprog);
            }
        }

        if(*p == '/')
        {
            ++p;
        }

        eap->nextcmd = check_nextcmd(p);
        return;
    }

    // Get the function name.  There are these situations:
    // func        function name
    //             "name" == func, "fudi.fd_dict" == NULL
    // dict.func   new dictionary entry
    //             "name" == NULL, "fudi.fd_dict" set,
    //             "fudi.fd_di" == NULL, "fudi.fd_newkey" == func
    // dict.func   existing dict entry with a Funcref
    //             "name" == func, "fudi.fd_dict" set,
    //             "fudi.fd_di" set, "fudi.fd_newkey" == NULL
    // dict.func   existing dict entry that's not a Funcref
    //             "name" == NULL, "fudi.fd_dict" set,
    //             "fudi.fd_di" set, "fudi.fd_newkey" == NULL
    // s:func      script-local function name
    // g:func      global function name, same as "func"
    p = eap->arg;
    name = trans_function_name(&p, eap->skip, 0, &fudi, NULL);
    paren = (ustrchr(p, '(') != NULL);

    if(name == NULL && (fudi.fd_dict == NULL || !paren) && !eap->skip)
    {
        // Return on an invalid expression in braces, unless the expression
        // evaluation has been cancelled due to an aborting error, an
        // interrupt, or an exception.
        if(!aborting())
        {
            if(fudi.fd_newkey != NULL)
            {
                EMSG2(_(e_dictkey), fudi.fd_newkey);
            }

            xfree(fudi.fd_newkey);
            return;
        }
        else
        {
            eap->skip = TRUE;
        }
    }

    // An error in a function call during evaluation of an expression in magic
    // braces should not cause the function not to be defined.
    saved_did_emsg = did_emsg;
    did_emsg = FALSE;

    // ":function func" with only function name: list function.
    if(!paren)
    {
        if(!ends_excmd(*skipwhite(p)))
        {
            EMSG(_(e_trailing));
            goto ret_free;
        }

        eap->nextcmd = check_nextcmd(p);

        if(eap->nextcmd != NULL)
        {
            *p = NUL;
        }

        if(!eap->skip && !got_int)
        {
            fp = find_func(name);

            if(fp != NULL)
            {
                list_func_head(fp, TRUE);

                for(int j = 0; j < fp->uf_lines.ga_len && !got_int; ++j)
                {
                    if(FUNCLINE(fp, j) == NULL)
                    {
                        continue;
                    }

                    msg_putchar('\n');
                    msg_outnum((long)(j + 1));

                    if(j < 9)
                    {
                        msg_putchar(' ');
                    }

                    if(j < 99)
                    {
                        msg_putchar(' ');
                    }

                    msg_prt_line(FUNCLINE(fp, j), FALSE);
                    ui_flush(); // show a line at a time
                    os_breakcheck();
                }

                if(!got_int)
                {
                    msg_putchar('\n');
                    msg_puts("   endfunction");
                }
            }
            else
            {
                emsg_funcname(N_("E123: Undefined function: %s"), name);
            }
        }

        goto ret_free;
    }

    // ":function name(arg1, arg2)" Define function.
    p = skipwhite(p);

    if(*p != '(')
    {
        if(!eap->skip)
        {
            EMSG2(_("E124: Missing '(': %s"), eap->arg);
            goto ret_free;
        }

        // attempt to continue by skipping some text
        if(ustrchr(p, '(') != NULL)
        {
            p = ustrchr(p, '(');
        }
    }

    p = skipwhite(p + 1);
    ga_init(&newargs, (int)sizeof(uchar_kt *), 3);
    ga_init(&newlines, (int)sizeof(uchar_kt *), 3);

    if(!eap->skip)
    {
        // Check the name of the function.
        // Unless it's a dictionary function
        // (that we are overwriting).
        if(name != NULL)
        {
            arg = name;
        }
        else
        {
            arg = fudi.fd_newkey;
        }

        if(arg != NULL
           && (fudi.fd_di == NULL || !tv_is_func(fudi.fd_di->di_tv)))
        {
            int j = (*arg == K_SPECIAL) ? 3 : 0;

            while(arg[j] != NUL
                  && (j == 0 ? eval_isnamec1(arg[j]) : eval_isnamec(arg[j])))
            {
                ++j;
            }

            if(arg[j] != NUL)
            {
                emsg_funcname((char *)e_invarg2, arg);
            }
        }

        // Disallow using the g: dict.
        if(fudi.fd_dict != NULL && fudi.fd_dict->dv_scope == VAR_DEF_SCOPE)
        {
            EMSG(_("E862: Cannot use g: here"));
        }
    }

    if(get_function_args(&p, ')', &newargs, &varargs, eap->skip) == FAIL)
    {
        goto errret_2;
    }

    // find extra arguments "range", "dict", "abort" and "closure"
    for(;;)
    {
        p = skipwhite(p);

        if(ustrncmp(p, "range", 5) == 0)
        {
            flags |= FC_RANGE;
            p += 5;
        }
        else if(ustrncmp(p, "dict", 4) == 0)
        {
            flags |= FC_DICT;
            p += 4;
        }
        else if(ustrncmp(p, "abort", 5) == 0)
        {
            flags |= FC_ABORT;
            p += 5;
        }
        else if(ustrncmp(p, "closure", 7) == 0)
        {
            flags |= FC_CLOSURE;
            p += 7;

            if(current_funccal == NULL)
            {
                emsg_funcname(N_("E932: Closure function should not be at top level: %s"),
                              name == NULL ? (uchar_kt *)"" : name);
                goto erret;
            }
        }
        else
        {
            break;
        }
    }

    // When there is a line break use what follows for the function body.
    // Makes 'exe "func Test()\n...\nendfunc"' work.
    if(*p == '\n')
    {
        line_arg = p + 1;
    }
    else if(*p != NUL && *p != '"' && !eap->skip && !did_emsg)
    {
        EMSG(_(e_trailing));
    }

    // Read the body of the function, until ":endfunction" is found.
    if(KeyTyped)
    {
        // Check if the function already exists, don't let the user type the
        // whole function before telling him it doesn't work! For a script we
        // need to skip the body to be able to find what follows.
        if(!eap->skip && !eap->forceit)
        {
            if(fudi.fd_dict != NULL && fudi.fd_newkey == NULL)
            {
                EMSG(_(e_funcdict));
            }
            else if(name != NULL && find_func(name) != NULL)
            {
                emsg_funcname(e_funcexts, name);
            }
        }

        if(!eap->skip && did_emsg)
        {
            goto erret;
        }

        msg_putchar('\n'); // don't overwrite the function name
        cmdline_row = msg_row;
    }

    indent = 2;
    nesting = 0;

    for(;;)
    {
        if(KeyTyped)
        {
            msg_scroll = TRUE;
            saved_wait_return = FALSE;
        }

        need_wait_return = FALSE;
        sourcing_lnum_off = sourcing_lnum;

        if(line_arg != NULL)
        {
            // Use eap->arg, split up in parts by line breaks.
            theline = line_arg;
            p = ustrchr(theline, '\n');

            if(p == NULL)
            {
                line_arg += ustrlen(line_arg);
            }
            else
            {
                *p = NUL;
                line_arg = p + 1;
            }
        }
        else if(eap->getline == NULL)
        {
            theline = getcmdline(':', 0L, indent);
        }
        else
        {
            theline = eap->getline(':', eap->cookie, indent);
        }

        if(KeyTyped)
        {
            lines_left = Rows - 1;
        }

        if(theline == NULL)
        {
            EMSG(_("E126: Missing :endfunction"));
            goto erret;
        }

        // Detect line continuation: sourcing_lnum
        // increased more than one.
        if(sourcing_lnum > sourcing_lnum_off + 1)
        {
            sourcing_lnum_off = sourcing_lnum - sourcing_lnum_off - 1;
        }
        else
        {
            sourcing_lnum_off = 0;
        }

        if(skip_until != NULL)
        {
            // between ":append" and "." and between
            // ":python <<EOF" and "EOF" don't check for ":endfunc".
            if(ustrcmp(theline, skip_until) == 0)
            {
                xfree(skip_until);
                skip_until = NULL;
            }
        }
        else
        {
            // skip ':' and blanks
            for(p = theline; ascii_iswhite(*p) || *p == ':'; ++p)
            { /* empty body */ }

            // Check for "endfunction".
            if(checkforcmd(&p, "endfunction", 4) && nesting-- == 0)
            {
                if(line_arg == NULL)
                {
                    xfree(theline);
                }

                break;
            }

            // Increase indent inside "if", "while", "for" and "try",
            // decrease at "end".
            if(indent > 2 && ustrncmp(p, "end", 3) == 0)
            {
                indent -= 2;
            }
            else if(ustrncmp(p, "if", 2) == 0
                    || ustrncmp(p, "wh", 2) == 0
                    || ustrncmp(p, "for", 3) == 0
                    || ustrncmp(p, "try", 3) == 0)
            {
                indent += 2;
            }

            // Check for defining a function inside this function.
            if(checkforcmd(&p, "function", 2))
            {
                if(*p == '!')
                {
                    p = skipwhite(p + 1);
                }

                p += eval_fname_script((const char *)p);
                xfree(trans_function_name(&p, true, 0, NULL, NULL));

                if(*skipwhite(p) == '(')
                {
                    nesting++;
                    indent += 2;
                }
            }

            // Check for ":append" or ":insert".
            p = skip_range(p, NULL);

            if((p[0] == 'a' && (!ASCII_ISALPHA(p[1]) || p[1] == 'p'))
                || (p[0] == 'i'
                    && (!ASCII_ISALPHA(p[1])
                        || (p[1] == 'n' && (!ASCII_ISALPHA(p[2]) || (p[2] == 's'))))))
            {
                skip_until = ustrdup((uchar_kt *)".");
            }

            // Check for ":python <<EOF", ":lua <<EOF", etc.
            arg = skipwhite(skiptowhite(p));

            if(arg[0] == '<' && arg[1] =='<'
               && ((p[0] == 'p' && p[1] == 'y' && (!ASCII_ISALPHA(p[2]) || p[2] == 't'))
                   || (p[0] == 'p' && p[1] == 'e' && (!ASCII_ISALPHA(p[2]) || p[2] == 'r'))
                   || (p[0] == 't' && p[1] == 'c' && (!ASCII_ISALPHA(p[2]) || p[2] == 'l'))
                   || (p[0] == 'l' && p[1] == 'u' && p[2] == 'a' && !ASCII_ISALPHA(p[3]))
                   || (p[0] == 'r' && p[1] == 'u' && p[2] == 'b' && (!ASCII_ISALPHA(p[3]) || p[3] == 'y'))
                   || (p[0] == 'm' && p[1] == 'z' && (!ASCII_ISALPHA(p[2]) || p[2] == 's'))))
            {
                // ":python <<" continues until a dot, like ":append"
                p = skipwhite(arg + 2);

                if(*p == NUL)
                {
                    skip_until = ustrdup((uchar_kt *)".");
                }
                else
                {
                    skip_until = ustrdup(p);
                }
            }
        }

        // Add the line to the function.
        ga_grow(&newlines, 1 + sourcing_lnum_off);

        // Copy the line to newly allocated memory. get_one_sourceline()
        // allocates 250 bytes per line, this saves 80% on average.
        // The cost is an extra alloc/free.
        p = ustrdup(theline);

        if(line_arg == NULL)
        {
            xfree(theline);
        }

        theline = p;
        ((uchar_kt **)(newlines.ga_data))[newlines.ga_len++] = theline;

        // Add NULL lines for continuation lines, so that the line count is
        // equal to the index in the growarray.
        while(sourcing_lnum_off-- > 0)
        {
            ((uchar_kt **)(newlines.ga_data))[newlines.ga_len++] = NULL;
        }

        // Check for end of eap->arg.
        if(line_arg != NULL && *line_arg == NUL)
        {
            line_arg = NULL;
        }
    }

    // Don't define the function when skipping commands
    // or when an error was detected.
    if(eap->skip || did_emsg)
    {
        goto erret;
    }

    // If there are no errors, add the function
    if(fudi.fd_dict == NULL)
    {
        v = find_var((const char *)name, ustrlen(name), &ht, false);

        if(v != NULL && v->di_tv.v_type == kNvarUfunc)
        {
            emsg_funcname(N_("E707: Function name conflicts with variable: %s"), name);
            goto erret;
        }

        fp = find_func(name);

        if(fp != NULL)
        {
            if(!eap->forceit)
            {
                emsg_funcname(e_funcexts, name);
                goto erret;
            }

            if(fp->uf_calls > 0)
            {
                emsg_funcname(N_("E127: Cannot redefine function %s: It is in use"), name);
                goto erret;
            }

            if(fp->uf_refcount > 1)
            {
                // This function is referenced somewhere,
                // don't redefine it but create a new one.
                (fp->uf_refcount)--;
                fp->uf_flags |= FC_REMOVED;
                fp = NULL;
                overwrite = true;
            }
            else
            {
                // redefine existing function
                ga_clear_strings(&(fp->uf_args));
                ga_clear_strings(&(fp->uf_lines));
                xfree(name);
                name = NULL;
            }
        }
    }
    else
    {
        char numbuf[20];
        fp = NULL;

        if(fudi.fd_newkey == NULL && !eap->forceit)
        {
            EMSG(_(e_funcdict));
            goto erret;
        }

        if(fudi.fd_di == NULL)
        {
            if(tv_check_lock(fudi.fd_dict->dv_lock, (const char *)eap->arg, TV_CSTRING))
            {
                // Can't add a function to a locked dictionary
                goto erret;
            }
        }
        else if(tv_check_lock(fudi.fd_di->di_tv.v_lock, (const char *)eap->arg, TV_CSTRING))
        {
            // Can't change an existing function if it is locked
            goto erret;
        }

        // Give the function a sequential number.
        // Can only be used with a Funcref!
        xfree(name);
        sprintf(numbuf, "%d", ++func_nr);
        name = ustrdup((uchar_kt *)numbuf);
    }

    if(fp == NULL)
    {
        if(fudi.fd_dict == NULL && ustrchr(name, AUTOLOAD_CHAR) != NULL)
        {
            int slen, plen;
            uchar_kt  *scriptname;

            // Check that the autoload name matches the script name.
            int j = FAIL;

            if(sourcing_name != NULL)
            {
                scriptname =
                    (uchar_kt *)autoload_name((const char *)name, ustrlen(name));

                p = ustrchr(scriptname, '/');
                plen = (int)ustrlen(p);
                slen = (int)ustrlen(sourcing_name);

                if(slen > plen && fnamecmp(p, sourcing_name + slen - plen) == 0)
                {
                    j = OK;
                }

                xfree(scriptname);
            }

            if(j == FAIL)
            {
                EMSG2(_("E746: Function name does not match script file name: %s"), name);
                goto erret;
            }
        }

        fp = xcalloc(1, sizeof(ufunc_st) + ustrlen(name));

        if(fudi.fd_dict != NULL)
        {
            if(fudi.fd_di == NULL)
            {
                // Add new dict entry
                fudi.fd_di = tv_dict_item_alloc((const char *)fudi.fd_newkey);

                if(tv_dict_add(fudi.fd_dict, fudi.fd_di) == FAIL)
                {
                    xfree(fudi.fd_di);
                    xfree(fp);
                    goto erret;
                }
            }
            else
            {
                // Overwrite existing dict entry.
                tv_clear(&fudi.fd_di->di_tv);
            }

            fudi.fd_di->di_tv.v_type = kNvarUfunc;
            fudi.fd_di->di_tv.v_lock = 0;
            fudi.fd_di->di_tv.vval.v_string = ustrdup(name);
            flags |= FC_DICT; // behave like "dict" was used
        }

        // insert the new function in the function list
        ustrcpy(fp->uf_name, name);

        if(overwrite)
        {
            hi = hash_find(&func_hashtab, name);
            hi->hi_key = UF2HIKEY(fp);
        }
        else if(hash_add(&func_hashtab, UF2HIKEY(fp)) == FAIL)
        {
            xfree(fp);
            goto erret;
        }

        fp->uf_refcount = 1;
    }

    fp->uf_args = newargs;
    fp->uf_lines = newlines;

    if((flags & FC_CLOSURE) != 0)
    {
        register_closure(fp);
    }
    else
    {
        fp->uf_scoped = NULL;
    }

    fp->uf_tml_count = NULL;
    fp->uf_tml_total = NULL;
    fp->uf_tml_self = NULL;
    fp->uf_profiling = FALSE;

    if(prof_def_func())
    {
        func_do_profile(fp);
    }

    fp->uf_varargs = varargs;
    fp->uf_flags = flags;
    fp->uf_calls = 0;
    fp->uf_script_ID = current_SID;
    goto ret_free;

erret:
    ga_clear_strings(&newargs);

errret_2:
    ga_clear_strings(&newlines);

ret_free:
    xfree(skip_until);
    xfree(fudi.fd_newkey);
    xfree(name);
    did_emsg |= saved_did_emsg;
    need_wait_return |= saved_wait_return;
}

/// Get a function name, translating "<SID>" and "<SNR>".
/// Also handles a Funcref in a List or Dictionary.
///
/// @param pp
/// @param skip
/// only find the end, don't evaluate
///
/// @param flags
/// - TFN_INT:         internal function name OK
/// - TFN_QUIET:       be quiet
/// - TFN_NO_AUTOLOAD: do not use script autoloading
/// - TFN_NO_DEREF:    do not dereference a Funcref
/// - Advances "pp" to just after the function name (if no error).
/// @param fdp
/// return: info about dictionary used
///
/// @param partial
/// return: partial of a FuncRef
///
/// @return the function name in allocated memory, or NULL for failure.
static uchar_kt *trans_function_name(uchar_kt **pp,
                                   int skip,
                                   int flags,
                                   funcdict_st *fdp,
                                   partial_st **partial)
{
    uchar_kt *name = NULL;
    const uchar_kt *start;
    const uchar_kt *end;
    int lead;
    int len;
    leftval_st lv;

    if(fdp != NULL)
    {
        memset(fdp, 0, sizeof(funcdict_st));
    }

    start = *pp;

    // Check for hard coded <SNR>: already translated
    // function ID (from a user command).
    if((*pp)[0] == K_SPECIAL
       && (*pp)[1] == KS_EXTRA
       && (*pp)[2] == (int)KE_SNR)
    {
        *pp += 3;
        len = get_id_len((const char **)pp) + 3;
        return (uchar_kt *)xmemdupz(start, len);
    }

    // A name starting with "<SID>" or "<SNR>" is local to a script. But
    // don't skip over "s:", get_lval() needs it for "s:dict.func".
    lead = eval_fname_script((const char *)start);

    if(lead > 2)
    {
        start += lead;
    }

    // Note that TFN_ flags use the same values as GLV_ flags.
    end = get_lval((uchar_kt *)start,
                   NULL,
                   &lv,
                   false,
                   skip,
                   flags,
                   lead > 2 ? 0 : FNE_CHECK_START);

    if(end == start)
    {
        if(!skip)
        {
            EMSG(_("E129: Function name required"));
        }

        goto theend;
    }

    if(end == NULL || (lv.ll_tv != NULL && (lead > 2 || lv.ll_range)))
    {
        // Report an invalid expression in braces, unless the expression
        // evaluation has been cancelled due to an aborting error, an
        // interrupt, or an exception.
        if(!aborting())
        {
            if(end != NULL)
            {
                emsgf(_(e_invarg2), start);
            }
        }
        else
        {
            *pp = (uchar_kt *)find_name_end(start, NULL, NULL, FNE_INCL_BR);
        }

        goto theend;
    }

    if(lv.ll_tv != NULL)
    {
        if(fdp != NULL)
        {
            fdp->fd_dict = lv.ll_dict;
            fdp->fd_newkey = lv.ll_newkey;
            lv.ll_newkey = NULL;
            fdp->fd_di = lv.ll_di;
        }

        if(lv.ll_tv->v_type == kNvarUfunc
           && lv.ll_tv->vval.v_string != NULL)
        {
            name = ustrdup(lv.ll_tv->vval.v_string);
            *pp = (uchar_kt *)end;
        }
        else if(lv.ll_tv->v_type == kNvarPartial
                && lv.ll_tv->vval.v_partial != NULL)
        {
            name = ustrdup(partial_name(lv.ll_tv->vval.v_partial));
            *pp = (uchar_kt *)end;

            if(partial != NULL)
            {
                *partial = lv.ll_tv->vval.v_partial;
            }
        }
        else
        {
            if(!skip
               && !(flags & TFN_QUIET)
               && (fdp == NULL
                   || lv.ll_dict == NULL
                   || fdp->fd_newkey == NULL))
            {
                EMSG(_(e_funcref));
            }
            else
            {
                *pp = (uchar_kt *)end;
            }

            name = NULL;
        }

        goto theend;
    }

    if(lv.ll_name == NULL)
    {
        // Error found, but continue after the function name.
        *pp = (uchar_kt *)end;
        goto theend;
    }

    // Check if the name is a Funcref.  If so, use the value.
    if(lv.ll_exp_name != NULL)
    {
        len = (int)strlen(lv.ll_exp_name);

        name = deref_func_name(lv.ll_exp_name, &len,
                               partial, flags & TFN_NO_AUTOLOAD);

        if((const char *)name == lv.ll_exp_name)
        {
            name = NULL;
        }
    }
    else if(!(flags & TFN_NO_DEREF))
    {
        len = (int)(end - *pp);

        name = deref_func_name((const char *)(*pp), &len,
                               partial, flags & TFN_NO_AUTOLOAD);

        if(name == *pp)
        {
            name = NULL;
        }
    }

    if(name != NULL)
    {
        name = ustrdup(name);
        *pp = (uchar_kt *)end;

        if(strncmp((char *)name, "<SNR>", 5) == 0)
        {
            // Change "<SNR>" to the byte sequence.
            name[0] = K_SPECIAL;
            name[1] = KS_EXTRA;
            name[2] = (int)KE_SNR;
            memmove(name + 3, name + 5, strlen((char *)name + 5) + 1);
        }

        goto theend;
    }

    if(lv.ll_exp_name != NULL)
    {
        len = (int)strlen(lv.ll_exp_name);

        if(lead <= 2
           && lv.ll_name == lv.ll_exp_name
           && lv.ll_name_len >= 2
           && memcmp(lv.ll_name, "s:", 2) == 0)
        {
            // When there was "s:" already or the
            // name expanded to get a leading "s:" then remove it.
            lv.ll_name += 2;
            lv.ll_name_len -= 2;
            len -= 2;
            lead = 2;
        }
    }
    else
    {
        // Skip over "s:" and "g:".
        if(lead == 2 || (lv.ll_name[0] == 'g' && lv.ll_name[1] == ':'))
        {
            lv.ll_name += 2;
            lv.ll_name_len -= 2;
        }

        len = (int)((const char *)end - lv.ll_name);
    }

    size_t sid_buf_len = 0;
    char sid_buf[20];

    // Copy the function name to allocated memory.
    // Accept <SID>name() inside a script, translate into <SNR>123_name().
    // Accept <SNR>123_name() outside a script.
    if(skip)
    {
        lead = 0; // do nothing
    }
    else if(lead > 0)
    {
        lead = 3;

        if((lv.ll_exp_name != NULL && eval_fname_sid(lv.ll_exp_name))
           || eval_fname_sid((const char *)(*pp)))
        {
            // It's "s:" or "<SID>".
            if(current_SID <= 0)
            {
                EMSG(_(e_usingsid));
                goto theend;
            }

            sid_buf_len = snprintf(sid_buf,
                                   sizeof(sid_buf),
                                   "%" ScriptIdKtPrtFmt "_",
                                   current_SID);

            lead += sid_buf_len;
        }
    }
    else if(!(flags & TFN_INT)
            && builtin_function(lv.ll_name, lv.ll_name_len))
    {
        EMSG2(_("E128: Function name must start with a capital or \"s:\": %s"), start);
        goto theend;
    }

    if(!skip && !(flags & TFN_QUIET) && !(flags & TFN_NO_DEREF))
    {
        uchar_kt *cp = xmemrchr(lv.ll_name, ':', lv.ll_name_len);

        if(cp != NULL && cp < end)
        {
            EMSG2(_("E884: Function name cannot contain a colon: %s"), start);
            goto theend;
        }
    }

    name = xmalloc(len + lead + 1);

    if(lead > 0)
    {
        name[0] = K_SPECIAL;
        name[1] = KS_EXTRA;
        name[2] = (int)KE_SNR;

        if(sid_buf_len > 0)  // If it's "<SID>"
        {
            memcpy(name + 3, sid_buf, sid_buf_len);
        }
    }

    memmove(name + lead, lv.ll_name, len);
    name[lead + len] = NUL;
    *pp = (uchar_kt *)end;

theend:

    clear_lval(&lv);
    return name;
}

/// - Return 5 if "p" starts with "<SID>" or "<SNR>" (ignoring case).
/// - Return 2 if "p" starts with "s:".
/// - Return 0 otherwise.
static int eval_fname_script(const char *const p)
{
    // Use mb_strnicmp() because in Turkish comparing the "I" may not work with
    // the standard library function.
    if(p[0] == '<'
       && (mb_strnicmp((uchar_kt *)p + 1, (uchar_kt *)"SID>", 4) == 0
           || mb_strnicmp((uchar_kt *)p + 1, (uchar_kt *)"SNR>", 4) == 0))
    {
        return 5;
    }

    if(p[0] == 's' && p[1] == ':')
    {
        return 2;
    }

    return 0;
}

/// Check whether function name starts with <SID> or s:
///
/// @warning
/// Only works for names previously checked by eval_fname_script(),
/// if it returned non-zero.
///
/// @param[in] name
/// Name to check.
///
/// @return true if it starts with <SID> or s:, false otherwise.
static inline bool eval_fname_sid(const char *const name)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_ALWAYS_INLINE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return *name == 's' || TOUPPER_ASC(name[2]) == 'I';
}

/// List the head of the function: "name(arg1, arg2)".
static void list_func_head(ufunc_st *fp, int indent)
{
    msg_start();

    if(indent)
    {
        MSG_PUTS("   ");
    }

    MSG_PUTS("function ");

    if(fp->uf_name[0] == K_SPECIAL)
    {
        MSG_PUTS_ATTR("<SNR>", hl_attr(HLF_8));
        msg_puts((const char *)fp->uf_name + 3);
    }
    else
    {
        msg_puts((const char *)fp->uf_name);
    }

    msg_putchar('(');
    int j;

    for(j = 0; j < fp->uf_args.ga_len; j++)
    {
        if(j)
        {
            msg_puts(", ");
        }

        msg_puts((const char *)FUNCARG(fp, j));
    }

    if(fp->uf_varargs)
    {
        if(j)
        {
            msg_puts(", ");
        }

        msg_puts("...");
    }

    msg_putchar(')');

    if(fp->uf_flags & FC_ABORT)
    {
        msg_puts(" abort");
    }

    if(fp->uf_flags & FC_RANGE)
    {
        msg_puts(" range");
    }

    if(fp->uf_flags & FC_DICT)
    {
        msg_puts(" dict");
    }

    if(fp->uf_flags & FC_CLOSURE)
    {
        msg_puts(" closure");
    }

    msg_clr_eos();

    if(p_verbose > 0)
    {
        last_set_msg(fp->uf_script_ID);
    }
}

/// Find a function by name, return pointer to it in ufuncs.
/// @return NULL for unknown function.
static ufunc_st *find_func(const uchar_kt *name)
{
    hashitem_st *hi;
    hi = hash_find(&func_hashtab, name);

    if(!HASHITEM_EMPTY(hi))
    {
        return HI2UF(hi);
    }

    return NULL;
}

#if defined(EXITFREE)
void free_all_functions(void)
{
    hashitem_st *hi;
    ufunc_st *fp;
    uint64_t skipped = 0;
    uint64_t todo = 1;
    uint64_t used;

    // First clear what the functions contain. Since this may lower the
    // reference count of a function, it may also free a function and change
    // the hash table. Restart if that happens.
    while(todo > 0)
    {
        todo = func_hashtab.ht_used;

        for(hi = func_hashtab.ht_array; todo > 0; hi++)
        {
            if(!HASHITEM_EMPTY(hi))
            {
                // Only free functions that are not refcounted, those are
                // supposed to be freed when no longer referenced.
                fp = HI2UF(hi);

                if(func_name_refcount(fp->uf_name))
                {
                    skipped++;
                }
                else
                {
                    used = func_hashtab.ht_used;
                    func_clear(fp, true);

                    if(used != func_hashtab.ht_used)
                    {
                        skipped = 0;
                        break;
                    }
                }

                todo--;
            }
        }
    }

    // Now actually free the functions. Need to start all over every time,
    // because func_free() may change the hash table.
    skipped = 0;

    while(func_hashtab.ht_used > skipped)
    {
        todo = func_hashtab.ht_used;

        for(hi = func_hashtab.ht_array; todo > 0; hi++)
        {
            if(!HASHITEM_EMPTY(hi))
            {
                todo--;
                // Only free functions that are not refcounted, those are
                // supposed to be freed when no longer referenced.
                fp = HI2UF(hi);

                if(func_name_refcount(fp->uf_name))
                {
                    skipped++;
                }
                else
                {
                    func_free(fp);
                    skipped = 0;
                    break;
                }
            }
        }
    }

    if(skipped == 0)
    {
        hash_clear(&func_hashtab);
    }
}
#endif

bool translated_function_exists(const char *name)
{
    if(builtin_function(name, -1))
    {
        return find_internal_func((char *)name) != NULL;
    }

    return find_func((const uchar_kt *)name) != NULL;
}

/// Check whether function with the given name exists
///
/// @param[in]  name      Function name.
/// @param[in]  no_deref  Whether to dereference a Funcref.
///
/// @return True if it exists, false otherwise.
static bool function_exists(const char *const name, bool no_deref)
{
    const uchar_kt *nm = (const uchar_kt *)name;
    bool n = false;
    int flag = TFN_INT | TFN_QUIET | TFN_NO_AUTOLOAD;

    if(no_deref)
    {
        flag |= TFN_NO_DEREF;
    }

    char *const p =
        (char *)trans_function_name((uchar_kt **)&nm, false, flag, NULL, NULL);
    nm = skipwhite(nm);

    // Only accept "funcname", "funcname ", "funcname (..." and
    // "funcname(...", not "funcname!...".
    if(p != NULL && (*nm == NUL || *nm == '('))
    {
        n = translated_function_exists(p);
    }

    xfree(p);
    return n;
}

/// Checks if a builtin function with the given name exists.
///
/// @param[in]   name
/// name of the builtin function to check.
///
/// @param[in]   len
/// length of "name", or -1 for NUL terminated.
///
/// @return
/// true if "name" looks like a builtin function name: starts with a
/// lower case letter and doesn't contain AUTOLOAD_CHAR.
static bool builtin_function(const char *name, int len)
{
    if(!ASCII_ISLOWER(name[0]))
    {
        return false;
    }

    const char *p = (len == -1
                     ? strchr(name, AUTOLOAD_CHAR)
                     : memchr(name, AUTOLOAD_CHAR, (size_t)len));

    return p == NULL;
}

/// Start profiling function "fp".
static void func_do_profile(ufunc_st *fp)
{
    int len = fp->uf_lines.ga_len;

    if(len == 0)
    {
        len = 1; // avoid getting error for allocating zero bytes
    }

    fp->uf_tm_count = 0;
    fp->uf_tm_self = profile_zero();
    fp->uf_tm_total = profile_zero();

    if(fp->uf_tml_count == NULL)
    {
        fp->uf_tml_count = xcalloc(len, sizeof(int));
    }

    if(fp->uf_tml_total == NULL)
    {
        fp->uf_tml_total = xcalloc(len, sizeof(proftime_kt));
    }

    if(fp->uf_tml_self == NULL)
    {
        fp->uf_tml_self = xcalloc(len, sizeof(proftime_kt));
    }

    fp->uf_tml_idx = -1;
    fp->uf_profiling = TRUE;
}

/// Dump the profiling results for all functions in file "fd".
void func_dump_profile(FILE *fd)
{
    hashitem_st *hi;
    int todo;
    ufunc_st *fp;
    ufunc_st **sorttab;
    int st_len = 0;
    todo = (int)func_hashtab.ht_used;

    if(todo == 0)
    {
        return; // nothing to dump
    }

    sorttab = xmalloc(sizeof(ufunc_st *) * todo);

    for(hi = func_hashtab.ht_array; todo > 0; ++hi)
    {
        if(!HASHITEM_EMPTY(hi))
        {
            --todo;
            fp = HI2UF(hi);

            if(fp->uf_profiling)
            {
                sorttab[st_len++] = fp;

                if(fp->uf_name[0] == K_SPECIAL)
                {
                    fprintf(fd, "FUNCTION  <SNR>%s()\n", fp->uf_name + 3);
                }
                else
                {
                    fprintf(fd, "FUNCTION  %s()\n", fp->uf_name);
                }

                if(fp->uf_tm_count == 1)
                {
                    fprintf(fd, "Called 1 time\n");
                }
                else
                {
                    fprintf(fd, "Called %d times\n", fp->uf_tm_count);
                }

                fprintf(fd, "Total time: %s\n", profile_msg(fp->uf_tm_total));
                fprintf(fd, " Self time: %s\n", profile_msg(fp->uf_tm_self));
                fprintf(fd, "\n");
                fprintf(fd, "count  total (s)   self (s)\n");

                for(int i = 0; i < fp->uf_lines.ga_len; ++i)
                {
                    if(FUNCLINE(fp, i) == NULL)
                    {
                        continue;
                    }

                    prof_func_line(fd,
                                   fp->uf_tml_count[i],
                                   &fp->uf_tml_total[i],
                                   &fp->uf_tml_self[i],
                                   TRUE);

                    fprintf(fd, "%s\n", FUNCLINE(fp, i));
                }

                fprintf(fd, "\n");
            }
        }
    }

    if(st_len > 0)
    {
        qsort((void *)sorttab, (size_t)st_len, sizeof(ufunc_st *), prof_total_cmp);
        prof_sort_list(fd, sorttab, st_len, "TOTAL", FALSE);

        qsort((void *)sorttab, (size_t)st_len, sizeof(ufunc_st *), prof_self_cmp);
        prof_sort_list(fd, sorttab, st_len, "SELF", TRUE);
    }

    xfree(sorttab);
}

/// @param fd
/// @param sorttab
/// @param st_len
/// @param title
/// @param prefer_self  when equal print only self time
static void prof_sort_list(FILE *fd,
                           ufunc_st **sorttab,
                           int st_len,
                           char *title,
                           int prefer_self)
{
    int i;
    ufunc_st *fp;
    fprintf(fd, "FUNCTIONS SORTED ON %s TIME\n", title);
    fprintf(fd, "count  total (s)   self (s)  function\n");

    for(i = 0; i < 20 && i < st_len; ++i)
    {
        fp = sorttab[i];

        prof_func_line(fd,
                       fp->uf_tm_count,
                       &fp->uf_tm_total,
                       &fp->uf_tm_self,
                       prefer_self);

        if(fp->uf_name[0] == K_SPECIAL)
        {
            fprintf(fd, " <SNR>%s()\n", fp->uf_name + 3);
        }
        else
        {
            fprintf(fd, " %s()\n", fp->uf_name);
        }
    }

    fprintf(fd, "\n");
}

/// Print the count and times for one function or function line.
///
/// @param fd
/// @param count
/// @param total
/// @param self
/// @param prefer_self  when equal print only self time
static void prof_func_line(FILE *fd,
                           int count,
                           proftime_kt *total,
                           proftime_kt *self,
                           int prefer_self)
{
    if(count > 0)
    {
        fprintf(fd, "%5d ", count);

        if(prefer_self && profile_equal(*total, *self))
        {
            fprintf(fd, "           ");
        }
        else
        {
            fprintf(fd, "%s ", profile_msg(*total));
        }

        if(!prefer_self && profile_equal(*total, *self))
        {
            fprintf(fd, "           ");
        }
        else
        {
            fprintf(fd, "%s ", profile_msg(*self));
        }
    }
    else
    {
        fprintf(fd, "                            ");
    }
}

/// Compare function for total time sorting.
static int prof_total_cmp(const void *s1, const void *s2)
{
    ufunc_st *p1 = *(ufunc_st **)s1;
    ufunc_st *p2 = *(ufunc_st **)s2;
    return profile_cmp(p1->uf_tm_total, p2->uf_tm_total);
}

/// Compare function for self time sorting.
static int prof_self_cmp(const void *s1, const void *s2)
{
    ufunc_st *p1 = *(ufunc_st **)s1;
    ufunc_st *p2 = *(ufunc_st **)s2;
    return profile_cmp(p1->uf_tm_self, p2->uf_tm_self);
}


/// If name has a package name try autoloading the script for it
///
/// @param[in]  name      Variable/function name.
/// @param[in]  name_len  Name length.
/// @param[in]  reload    If true, load script again when already loaded.
///
/// @return true if a package was loaded.
static bool script_autoload(const char *const name,
                            const size_t name_len,
                            const bool reload)
{
    // If there is no '#' after name[0] there is no package name.
    const char *p = memchr(name, AUTOLOAD_CHAR, name_len);

    if(p == NULL || p == name)
    {
        return false;
    }

    bool ret = false;
    char *tofree = autoload_name(name, name_len);
    char *scriptname = tofree;

    // Find the name in the list of previously loaded package names.
    // Skip "autoload/", it's always the same.
    int i = 0;

    for(; i < ga_loaded.ga_len; i++)
    {
        if(ustrcmp(((char **)ga_loaded.ga_data)[i] + 9, scriptname + 9) == 0)
        {
            break;
        }
    }

    if(!reload && i < ga_loaded.ga_len)
    {
        ret = false; // Was loaded already.
    }
    else
    {
        // Remember the name if it wasn't loaded already.
        if(i == ga_loaded.ga_len)
        {
            GA_APPEND(char *, &ga_loaded, scriptname);
            tofree = NULL;
        }

        // Try loading the package from $VIMRUNTIME/autoload/<name>.vim
        if(source_runtime((uchar_kt *)scriptname, 0) == OK)
        {
            ret = true;
        }
    }

    xfree(tofree);
    return ret;
}

/// Return the autoload script name for a function or variable name
///
/// @param[in]  name      Variable/function name.
/// @param[in]  name_len  Name length.
///
/// @return [allocated] autoload script name.
static char *autoload_name(const char *const name, const size_t name_len)
FUNC_ATTR_MALLOC
FUNC_ATTR_WARN_UNUSED_RESULT
{
    // Get the script file name: replace '#' with '/', append ".vim".
    char *const scriptname = xmalloc(name_len + sizeof("autoload/.vim"));

    memcpy(scriptname, "autoload/", sizeof("autoload/") - 1);
    memcpy(scriptname + sizeof("autoload/") - 1, name, name_len);
    size_t auchar_idx = 0;

    for(size_t i = sizeof("autoload/") - 1;
        i - sizeof("autoload/") + 1 < name_len;
        i++)
    {
        if(scriptname[i] == AUTOLOAD_CHAR)
        {
            scriptname[i] = '/';
            auchar_idx = i;
        }
    }

    memcpy(scriptname + auchar_idx, ".vim", sizeof(".vim"));

    return scriptname;
}


/// Function given to ExpandGeneric() to obtain the
/// list of user defined function names.
uchar_kt *get_user_func_name(expand_st *xp, int idx)
{
    static size_t done;
    static hashitem_st *hi;
    ufunc_st *fp;

    if(idx == 0)
    {
        done = 0;
        hi = func_hashtab.ht_array;
    }

    assert(hi);

    if(done < func_hashtab.ht_used)
    {
        if(done++ > 0)
        {
            ++hi;
        }

        while(HASHITEM_EMPTY(hi))
        {
            ++hi;
        }

        fp = HI2UF(hi);

        if((fp->uf_flags & FC_DICT)
           || ustrncmp(fp->uf_name, "<lambda>", 8) == 0)
        {
            return (uchar_kt *)""; // don't show dict and lambda functions
        }

        if(ustrlen(fp->uf_name) + 4 >= IOSIZE)
        {
            return fp->uf_name; // prevents overflow
        }

        cat_func_name(IObuff, fp);

        if(xp->xp_context != EXPAND_USER_FUNC)
        {
            ustrcat(IObuff, "(");

            if(!fp->uf_varargs && GA_EMPTY(&fp->uf_args))
            {
                ustrcat(IObuff, ")");
            }
        }

        return IObuff;
    }

    return NULL;
}

/// Copy the function name of "fp" to buffer "buf".
/// "buf" must be able to hold the function name plus three bytes.
/// Takes care of script-local function names.
static void cat_func_name(uchar_kt *buf, ufunc_st *fp)
{
    if(fp->uf_name[0] == K_SPECIAL)
    {
        ustrcpy(buf, "<SNR>");
        ustrcat(buf, fp->uf_name + 3);
    }
    else
    {
        ustrcpy(buf, fp->uf_name);
    }
}

/// There are two kinds of function names:
/// 1. ordinary names, function defined with :function
/// 2. numbered functions and lambdas
///
/// For the first we only count the name stored in func_hashtab as a reference,
/// using function() does not count as a reference, because the function is
/// looked up by name.
static bool func_name_refcount(uchar_kt *name)
{
    return isdigit(*name) || *name == '<';
}

/// ":delfunction {name}"
void ex_delfunction(exargs_st *eap)
{
    ufunc_st *fp = NULL;
    uchar_kt *p;
    uchar_kt *name;
    funcdict_st fudi;
    p = eap->arg;
    name = trans_function_name(&p, eap->skip, 0, &fudi, NULL);
    xfree(fudi.fd_newkey);

    if(name == NULL)
    {
        if(fudi.fd_dict != NULL && !eap->skip)
        {
            EMSG(_(e_funcref));
        }

        return;
    }

    if(!ends_excmd(*skipwhite(p)))
    {
        xfree(name);
        EMSG(_(e_trailing));
        return;
    }

    eap->nextcmd = check_nextcmd(p);

    if(eap->nextcmd != NULL)
    {
        *p = NUL;
    }

    if(!eap->skip)
    {
        fp = find_func(name);
    }

    xfree(name);

    if(!eap->skip)
    {
        if(fp == NULL)
        {
            EMSG2(_(e_nofunc), eap->arg);
            return;
        }

        if(fp->uf_calls > 0)
        {
            EMSG2(_("E131: Cannot delete function %s: It is in use"), eap->arg);
            return;
        }

        // check `uf_refcount > 2` because deleting a function should also reduce
        // the reference count, and 1 is the initial refcount.
        if(fp->uf_refcount > 2)
        {
            EMSG2(_("Cannot delete function %s: It is being used internally"),
                  eap->arg);
            return;
        }

        if(fudi.fd_dict != NULL)
        {
            // Delete the dict item that refers to the function, it will
            // invoke func_unref() and possibly delete the function.
            tv_dict_item_remove(fudi.fd_dict, fudi.fd_di);
        }
        else
        {
            // A normal function (not a numbered function or lambda) has a
            // refcount of 1 for the entry in the hashtable. When deleting
            // it and the refcount is more than one, it should be kept.
            // A numbered function or lambda should be kept if the refcount is
            // one or more.
            if(fp->uf_refcount > (func_name_refcount(fp->uf_name) ? 0 : 1))
            {
                // Function is still referenced somewhere. Don't free it but
                // do remove it from the hashtable.
                if(func_remove(fp))
                {
                    fp->uf_refcount--;
                }

                fp->uf_flags |= FC_DELETED;
            }
            else
            {
                func_clear_free(fp, false);
            }
        }
    }
}

/// Remove the function from the function hashtable. If the function was
/// deleted while it still has references this was already done.
///
/// @return true if the entry was deleted, false if it wasn't found.
static bool func_remove(ufunc_st *fp)
{
    hashitem_st *hi = hash_find(&func_hashtab, UF2HIKEY(fp));

    if(!HASHITEM_EMPTY(hi))
    {
        hash_remove(&func_hashtab, hi);
        return true;
    }

    return false;
}

/// Free all things that a function contains. Does not free the function
/// itself, use func_free() for that.
///
/// @param[in]  force   When true, we are exiting.
static void func_clear(ufunc_st *fp, bool force)
{
    if(fp->uf_cleared)
    {
        return;
    }

    fp->uf_cleared = true;

    // clear this function
    ga_clear_strings(&(fp->uf_args));
    ga_clear_strings(&(fp->uf_lines));
    xfree(fp->uf_tml_count);
    xfree(fp->uf_tml_total);
    xfree(fp->uf_tml_self);
    funccal_unref(fp->uf_scoped, fp, force);
}

/// Free a function and remove it from the list of functions. Does not free
/// what a function contains, call func_clear() first.
///
/// @param[in]   fp   The function to free.
static void func_free(ufunc_st *fp)
{
    // only remove it when not done already, otherwise
    // we would remove a newer version of the function
    if((fp->uf_flags & (FC_DELETED | FC_REMOVED)) == 0)
    {
        func_remove(fp);
    }

    xfree(fp);
}

/// Free all things that a function contains and free the function itself.
///
/// @param[in]  force  When true, we are exiting.
static void func_clear_free(ufunc_st *fp, bool force)
{
    func_clear(fp, force);
    func_free(fp);
}

/// Unreference a Function: decrement the reference
/// count and free it when it becomes zero.
void func_unref(uchar_kt *name)
{
    ufunc_st *fp = NULL;

    if(name == NULL || !func_name_refcount(name))
    {
        return;
    }

    fp = find_func(name);

    if(fp == NULL && isdigit(*name))
    {
#ifdef EXITFREE
        if(!entered_free_all_mem)
        {
            EMSG2(_(e_intern2), "func_unref()");
            abort();
        }
#else
        EMSG2(_(e_intern2), "func_unref()");
        abort();
#endif
    }

    func_ptr_unref(fp);
}

/// Unreference a Function: decrement the reference count and free it when it
/// becomes zero.
/// Unreference user function, freeing it if needed
///
/// Decrements the reference count and frees when it becomes zero.
///
/// @param  fp  Function to unreference.
void func_ptr_unref(ufunc_st *fp)
{
    if(fp != NULL && --fp->uf_refcount <= 0)
    {
        // Only delete it when it's not being used. Otherwise it's done
        // when "uf_calls" becomes zero.
        if(fp->uf_calls == 0)
        {
            func_clear_free(fp, false);
        }
    }
}

/// Count a reference to a Function.
void func_ref(uchar_kt *name)
{
    ufunc_st *fp;

    if(name == NULL || !func_name_refcount(name))
    {
        return;
    }

    fp = find_func(name);

    if(fp != NULL)
    {
        (fp->uf_refcount)++;
    }
    else if(isdigit(*name))
    {
        // Only give an error for a numbered function.
        // Fail silently, when named or lambda function isn't found.
        EMSG2(_(e_intern2), "func_ref()");
    }
}

/// Count a reference to a Function.
void func_ptr_ref(ufunc_st *fp)
{
    if(fp != NULL)
    {
        (fp->uf_refcount)++;
    }
}

/// Call a user function
///
/// @param  fp             Function to call.
/// @param[in]             argcount  Number of arguments.
/// @param  argvars        Arguments.
/// @param[out] rettv      Return value.
/// @param[in]  firstline  First line of range.
/// @param[in]  lastline   Last line of range.
/// @param  selfdict       Dictionary for "self" for dictionary functions.
void call_user_func(ufunc_st *fp,
                    int argcount,
                    typval_st *argvars,
                    typval_st *rettv,
                    linenum_kt firstline,
                    linenum_kt lastline,
                    dict_st *selfdict)
FUNC_ATTR_NONNULL_ARG(1, 3, 4)
{
    uchar_kt *save_sourcing_name;
    linenum_kt save_sourcing_lnum;
    script_id_kt save_current_SID;
    func_call_st  *fc;
    int save_did_emsg;
    static int depth = 0;
    dictitem_st *v;
    int fixvar_idx = 0; // index in fixvar[]
    int ai;
    bool islambda = false;
    uchar_kt numbuf[NUMBUFLEN];
    uchar_kt *name;
    proftime_kt wait_start;
    proftime_kt call_start;
    bool did_save_redo = false;

    // If depth of calling is getting too high, don't execute the function
    if(depth >= p_mfd)
    {
        EMSG(_("E132: Function call depth is higher than 'maxfuncdepth'"));
        rettv->v_type = kNvarNumber;
        rettv->vval.v_number = -1;
        return;
    }

    ++depth;
    // Save search patterns and redo buffer.
    save_search_patterns();

    if(!ins_compl_active())
    {
        saveRedobuff();
        did_save_redo = true;
    }

    ++fp->uf_calls;

    // check for CTRL-C hit
    line_breakcheck();

    // prepare the func_call_st structure
    fc = xmalloc(sizeof(func_call_st));
    fc->caller = current_funccal;
    current_funccal = fc;
    fc->func = fp;
    fc->rettv = rettv;
    rettv->vval.v_number = 0;
    fc->linenr = 0;
    fc->returned = FALSE;
    fc->level = ex_nesting_level;

    // Check if this function has a breakpoint.
    fc->breakpoint = dbg_find_breakpoint(FALSE, fp->uf_name, (linenum_kt)0);
    fc->dbg_tick = debug_tick;

    // Set up fields for closure.
    fc->fc_refcount = 0;
    fc->fc_copyID = 0;
    ga_init(&fc->fc_funcs, sizeof(ufunc_st *), 1);
    func_ptr_ref(fp);

    if(ustrncmp(fp->uf_name, "<lambda>", 8) == 0)
    {
        islambda = true;
    }

    // Note about using fc->fixvar[]: This is an array of FIXVAR_CNT variables
    // with names up to VAR_SHORT_LEN long.  This avoids having to alloc/free
    // each argument variable and saves a lot of time.
    //
    // Init l: variables.
    init_var_dict(&fc->l_vars, &fc->l_vars_var, VAR_DEF_SCOPE);

    if(selfdict != NULL)
    {
        // Set l:self to "selfdict".  Use "name" to avoid a warning from
        // some compiler that checks the destination size.
        v = (dictitem_st *)&fc->fixvar[fixvar_idx++];

#ifndef __clang_analyzer__
        name = v->di_key;
        ustrcpy(name, "self");
#endif

        v->di_flags = DI_FLAGS_RO + DI_FLAGS_FIX;
        tv_dict_add(&fc->l_vars, v);
        v->di_tv.v_type = kNvarDict;
        v->di_tv.v_lock = 0;
        v->di_tv.vval.v_dict = selfdict;
        ++selfdict->dv_refcount;
    }

    // Init a: variables.
    // Set a:0 to "argcount".
    // Set a:000 to a list with room for the "..." arguments.
    init_var_dict(&fc->l_avars, &fc->l_avars_var, VAR_SCOPE);

    add_nr_var(&fc->l_avars,
              (dictitem_st *)&fc->fixvar[fixvar_idx++],
              "0",
              (number_kt)(argcount - fp->uf_args.ga_len));

    // Use "name" to avoid a warning from some compiler
    // that checks the destination size.
    v = (dictitem_st *)&fc->fixvar[fixvar_idx++];

#ifndef __clang_analyzer__
    name = v->di_key;
    ustrcpy(name, "000");
#endif

    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
    tv_dict_add(&fc->l_avars, v);
    v->di_tv.v_type = kNvarList;
    v->di_tv.v_lock = kNvlVarFixed;
    v->di_tv.vval.v_list = &fc->l_varlist;
    memset(&fc->l_varlist, 0, sizeof(list_st));
    fc->l_varlist.lv_refcount = DO_NOT_FREE_CNT;
    fc->l_varlist.lv_lock = kNvlVarFixed;

    // Set a:firstline to "firstline" and a:lastline to "lastline".
    // Set a:name to named arguments.
    // Set a:N to the "..." arguments.
    add_nr_var(&fc->l_avars,
               (dictitem_st *)&fc->fixvar[fixvar_idx++],
               "firstline",
               (number_kt)firstline);

    add_nr_var(&fc->l_avars,
               (dictitem_st *)&fc->fixvar[fixvar_idx++],
               "lastline",
               (number_kt)lastline);

    for(int i = 0; i < argcount; i++)
    {
        bool addlocal = false;
        ai = i - fp->uf_args.ga_len;

        if(ai < 0)
        {
            // named argument a:name
            name = FUNCARG(fp, i);

            if(islambda)
            {
                addlocal = true;
            }
        }
        else
        {
            // "..." argument a:1, a:2, etc.
            snprintf((char *)numbuf, sizeof(numbuf), "%d", ai + 1);
            name = numbuf;
        }

        if(fixvar_idx < FIXVAR_CNT && ustrlen(name) <= VAR_SHORT_LEN)
        {
            v = (dictitem_st *)&fc->fixvar[fixvar_idx++];
            v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
        }
        else
        {
            v = xmalloc(sizeof(dictitem_st) + ustrlen(name));
            v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX | DI_FLAGS_ALLOC;
        }

        ustrcpy(v->di_key, name);

        // Note: the values are copied directly to avoid alloc/free.
        // "argvars" must have kNvlVarFixed for v_lock.
        v->di_tv = argvars[i];
        v->di_tv.v_lock = kNvlVarFixed;

        if(addlocal)
        {
            // Named arguments can be accessed without the "a:"
            // prefix in lambda expressions. Add to the l: dict.
            tv_copy(&v->di_tv, &v->di_tv);
            tv_dict_add(&fc->l_vars, v);
        }
        else
        {
            tv_dict_add(&fc->l_avars, v);
        }

        if(ai >= 0 && ai < MAX_FUNC_ARGS)
        {
            tv_list_append(&fc->l_varlist, &fc->l_listitems[ai]);
            fc->l_listitems[ai].li_tv = argvars[i];
            fc->l_listitems[ai].li_tv.v_lock = kNvlVarFixed;
        }
    }

    // Don't redraw while executing the function.
    ++RedrawingDisabled;
    save_sourcing_name = sourcing_name;
    save_sourcing_lnum = sourcing_lnum;
    sourcing_lnum = 1;

    // need space for new sourcing_name:
    // * save_sourcing_name
    // * "["number"].." or "function "
    // * "<SNR>" + fp->uf_name - 3
    // * terminating NUL
    size_t len = (save_sourcing_name == NULL ? 0 : ustrlen(save_sourcing_name))
                 + ustrlen(fp->uf_name) + 27;

    sourcing_name = xmalloc(len);

    {
        if(save_sourcing_name != NULL
           && ustrncmp(save_sourcing_name, "function ", 9) == 0)
        {
            xsnprintf((char *)sourcing_name,
                      len,
                      "%s[%" PRId64 "]..",
                      save_sourcing_name,
                      (int64_t)save_sourcing_lnum);
        }
        else
        {
            ustrcpy(sourcing_name, "function ");
        }

        cat_func_name(sourcing_name + ustrlen(sourcing_name), fp);

        if(p_verbose >= 12)
        {
            ++no_wait_return;
            verbose_enter_scroll();
            smsg(_("calling %s"), sourcing_name);

            if(p_verbose >= 14)
            {
                msg_puts("(");

                for(int i = 0; i < argcount; i++)
                {
                    if(i > 0)
                    {
                        msg_puts(", ");
                    }

                    if(argvars[i].v_type == kNvarNumber)
                    {
                        msg_outnum((long)argvars[i].vval.v_number);
                    }
                    else
                    {
                        // Do not want errors such as E724 here.
                        emsg_off++;
                        char *tofree = encode_tv2string(&argvars[i], NULL);
                        emsg_off--;

                        if(tofree != NULL)
                        {
                            char *s = tofree;
                            char buf[MSG_BUF_LEN];

                            if(ustr_scrsize((uchar_kt *)s) > MSG_BUF_CLEN)
                            {
                                trunc_string((uchar_kt *)s,
                                             (uchar_kt *)buf,
                                             MSG_BUF_CLEN,
                                             sizeof(buf));
                                s = buf;
                            }

                            msg_puts(s);
                            xfree(tofree);
                        }
                    }
                }

                msg_puts(")");
            }

            msg_puts("\n"); // don't overwrite this either
            verbose_leave_scroll();
            --no_wait_return;
        }
    }

    bool func_not_yet_profiling_but_should =
        do_profiling == PROF_YES
        && !fp->uf_profiling
        && has_profiling(FALSE, fp->uf_name, NULL);

    if(func_not_yet_profiling_but_should)
    {
        func_do_profile(fp);
    }

    bool func_or_func_caller_profiling =
        do_profiling == PROF_YES
        && (fp->uf_profiling
            || (fc->caller != NULL && fc->caller->func->uf_profiling));

    if(func_or_func_caller_profiling)
    {
        ++fp->uf_tm_count;
        call_start = profile_start();
        fp->uf_tm_children = profile_zero();
    }

    if(do_profiling == PROF_YES)
    {
        script_prof_save(&wait_start);
    }

    save_current_SID = current_SID;
    current_SID = fp->uf_script_ID;
    save_did_emsg = did_emsg;
    did_emsg = FALSE;

    // call do_cmdline() to execute the lines
    do_cmdline(NULL,
               get_func_line,
               (void *)fc,
               DOCMD_NOWAIT|DOCMD_VERBOSE|DOCMD_REPEAT);

    --RedrawingDisabled;

    // when the function was aborted because of an error, return -1
    if((did_emsg
       && (fp->uf_flags & FC_ABORT)) || rettv->v_type == kNvarUnknown)
    {
        tv_clear(rettv);
        rettv->v_type = kNvarNumber;
        rettv->vval.v_number = -1;
    }

    if(func_or_func_caller_profiling)
    {
        call_start = profile_end(call_start);
        call_start = profile_sub_wait(wait_start, call_start);
        fp->uf_tm_total = profile_add(fp->uf_tm_total, call_start);

        fp->uf_tm_self =
            profile_self(fp->uf_tm_self, call_start, fp->uf_tm_children);

        if(fc->caller != NULL && fc->caller->func->uf_profiling)
        {
            fc->caller->func->uf_tm_children =
                profile_add(fc->caller->func->uf_tm_children, call_start);

            fc->caller->func->uf_tml_children =
                profile_add(fc->caller->func->uf_tml_children, call_start);
        }
    }

    // when being verbose, mention the return value
    if(p_verbose >= 12)
    {
        ++no_wait_return;
        verbose_enter_scroll();

        if(aborting())
        {
            smsg(_("%s aborted"), sourcing_name);
        }
        else if(fc->rettv->v_type == kNvarNumber)
        {
            smsg(_("%s returning #%" PRId64 ""),
                 sourcing_name,
                 (int64_t)fc->rettv->vval.v_number);
        }
        else
        {
            uchar_kt buf[MSG_BUF_LEN];
            // The value may be very long. Skip the middle part, so that we
            // have some idea how it starts and ends. smsg() would always
            // truncate it at the end. Don't want errors such as E724 here.
            emsg_off++;
            uchar_kt *s = (uchar_kt *) encode_tv2string(fc->rettv, NULL);
            uchar_kt *tofree = s;
            emsg_off--;

            if(s != NULL)
            {
                if(ustr_scrsize(s) > MSG_BUF_CLEN)
                {
                    trunc_string(s, buf, MSG_BUF_CLEN, MSG_BUF_LEN);
                    s = buf;
                }

                smsg(_("%s returning %s"), sourcing_name, s);
                xfree(tofree);
            }
        }

        msg_puts("\n"); // don't overwrite this either
        verbose_leave_scroll();
        --no_wait_return;
    }

    xfree(sourcing_name);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
    current_SID = save_current_SID;

    if(do_profiling == PROF_YES)
    {
        script_prof_restore(&wait_start);
    }

    if(p_verbose >= 12 && sourcing_name != NULL)
    {
        ++no_wait_return;
        verbose_enter_scroll();
        smsg(_("continuing in %s"), sourcing_name);
        msg_puts("\n");  // don't overwrite this either
        verbose_leave_scroll();
        --no_wait_return;
    }

    did_emsg |= save_did_emsg;
    current_funccal = fc->caller;
    --depth;

    // If the a:000 list and the l: and a: dicts are not referenced and there
    // is no closure using it, we can free the func_call_st and what's in it.
    if(fc->l_varlist.lv_refcount == DO_NOT_FREE_CNT
       && fc->l_vars.dv_refcount == DO_NOT_FREE_CNT
       && fc->l_avars.dv_refcount == DO_NOT_FREE_CNT
       && fc->fc_refcount <= 0)
    {
        free_funccal(fc, false);
    }
    else
    {
        // "fc" is still in use.  This can happen when returning "a:000",
        // assigning "l:" to a global variable or defining a closure.
        // Link "fc" in the list for garbage collection later.
        fc->caller = previous_funccal;
        previous_funccal = fc;

        // Make a copy of the a: variables, since we didn't do that above.
        TV_DICT_ITER(&fc->l_avars, di, {
            tv_copy(&di->di_tv, &di->di_tv);
        });

        // Make a copy of the a:000 items, since we didn't do that above.
        for(listitem_st *li = fc->l_varlist.lv_first; li != NULL; li = li->li_next)
        {
            tv_copy(&li->li_tv, &li->li_tv);
        }
    }

    if(--fp->uf_calls <= 0 && fp->uf_refcount <= 0)
    {
        // Function was unreferenced while being used, free it now.
        func_clear_free(fp, false);
    }

    // restore search patterns and redo buffer
    if(did_save_redo)
    {
        restoreRedobuff();
    }

    restore_search_patterns();
}

/// Unreference "fc": decrement the reference count and free it when it
/// becomes zero. "fp" is detached from "fc".
///
/// @param[in] force  When true, we are exiting.
static void funccal_unref(func_call_st *fc, ufunc_st *fp, bool force)
{
    func_call_st **pfc;
    int i;

    if(fc == NULL)
    {
        return;
    }

    if(--fc->fc_refcount <= 0
       && (force
           || (fc->l_varlist.lv_refcount == DO_NOT_FREE_CNT
               && fc->l_vars.dv_refcount == DO_NOT_FREE_CNT
               && fc->l_avars.dv_refcount == DO_NOT_FREE_CNT)))
    {
        for(pfc = &previous_funccal; *pfc != NULL; pfc = &(*pfc)->caller)
        {
            if(fc == *pfc)
            {
                *pfc = fc->caller;
                free_funccal(fc, true);
                return;
            }
        }
    }

    for(i = 0; i < fc->fc_funcs.ga_len; i++)
    {
        if(((ufunc_st **)(fc->fc_funcs.ga_data))[i] == fp)
        {
            ((ufunc_st **)(fc->fc_funcs.ga_data))[i] = NULL;
        }
    }
}

/// @return true if items in "fc" do not have "copyID". That means they are not
/// referenced from anywhere that is in use.
static int can_free_funccal(func_call_st *fc, int copyID)
{
    return fc->l_varlist.lv_copyID != copyID
           && fc->l_vars.dv_copyID != copyID
           && fc->l_avars.dv_copyID != copyID
           && fc->fc_copyID != copyID;
}

/// Free "fc" and what it contains.
///
/// \param fc
/// \param free_val  a: vars were allocated
static void free_funccal(func_call_st *fc, int free_val)
{
    listitem_st *li;

    for(int i = 0; i < fc->fc_funcs.ga_len; i++)
    {
        ufunc_st *fp = ((ufunc_st **)(fc->fc_funcs.ga_data))[i];

        // When garbage collecting a func_call_st may be freed before the
        // function that references it, clear its uf_scoped field.
        // The function may have been redefined and point to another
        // funccal_T, don't clear it then.
        if(fp != NULL && fp->uf_scoped == fc)
        {
            fp->uf_scoped = NULL;
        }
    }

    ga_clear(&fc->fc_funcs);

    // The a: variables typevals may not have been allocated,
    // only free the allocated variables.
    vars_clear_ext(&fc->l_avars.dv_hashtab, free_val);

    // free all l: variables
    vars_clear(&fc->l_vars.dv_hashtab);

    // Free the a:000 variables if they were allocated.
    if(free_val)
    {
        for(li = fc->l_varlist.lv_first; li != NULL; li = li->li_next)
        {
            tv_clear(&li->li_tv);
        }
    }

    func_ptr_unref(fc->func);
    xfree(fc);
}

/// Add a number variable "name" to dict "dp" with value "nr".
static void add_nr_var(dict_st *dp, dictitem_st *v, char *name, number_kt nr)
{
#ifndef __clang_analyzer__
    ustrcpy(v->di_key, name);
#endif

    v->di_flags = DI_FLAGS_RO | DI_FLAGS_FIX;
    tv_dict_add(dp, v);
    v->di_tv.v_type = kNvarNumber;
    v->di_tv.v_lock = kNvlVarFixed;
    v->di_tv.vval.v_number = nr;
}

/// ":return [expr]"
void ex_return(exargs_st *eap)
{
    uchar_kt *arg = eap->arg;
    typval_st rettv;
    int returning = FALSE;

    if(current_funccal == NULL)
    {
        EMSG(_("E133: :return not inside a function"));
        return;
    }

    if(eap->skip)
    {
        ++emsg_skip;
    }

    eap->nextcmd = NULL;

    if((*arg != NUL && *arg != '|' && *arg != '\n')
       && eval_lev_0(arg, &rettv, &eap->nextcmd, !eap->skip) != FAIL)
    {
        if(!eap->skip)
        {
            returning = do_return(eap, false, true, &rettv);
        }
        else
        {
            tv_clear(&rettv);
        }
    }
    // It's safer to return also on error.
    else if(!eap->skip)
    {
        // Return unless the expression evaluation has been cancelled
        // due to an aborting error, an interrupt, or an exception.
        if(!aborting())
        {
            returning = do_return(eap, FALSE, TRUE, NULL);
        }
    }

    // When skipping or the return gets pending, advance to the next command
    // in this line (!returning). Otherwise, ignore the rest of the line.
    // Following lines will be ignored by get_func_line().
    if(returning)
    {
        eap->nextcmd = NULL;
    }
    else if(eap->nextcmd == NULL) // no argument
    {
        eap->nextcmd = check_nextcmd(arg);
    }

    if(eap->skip)
    {
        --emsg_skip;
    }
}

/// Return from a function. Possibly makes the return pending. Also called
/// for a pending return at the ":endtry" or after returning from an extra
/// do_cmdline(). "reanimate" is used in the latter case. "is_cmd" is set
/// when called due to a ":return" command. "rettv" may point to a typval_st
/// with the return rettv. Returns TRUE when the return can be carried out,
/// FALSE when the return gets pending.
int do_return(exargs_st *eap, int reanimate, int is_cmd, void *rettv)
{
    int idx;
    condstack_st *cstack = eap->cstack;

    if(reanimate)
    {
        // Undo the return.
        current_funccal->returned = FALSE;
    }

    // Cleanup (and inactivate) conditionals, but stop when a try conditional
    // not in its finally clause (which then is to be executed next) is found.
    // In this case, make the ":return" pending for execution at the ":endtry".
    // Otherwise, return normally.
    idx = cleanup_conditionals(eap->cstack, 0, TRUE);

    if(idx >= 0)
    {
        cstack->cs_pending[idx] = kCSTflgReturn;

        if(!is_cmd && !reanimate)
        {
            // A pending return again gets pending. "rettv" points to an
            // allocated variable with the rettv of the original ":return"'s
            // argument if present or is NULL else.
            cstack->cs_rettv[idx] = rettv;
        }
        else
        {
            // When undoing a return in order to make it pending,
            // get the stored return rettv.
            if(reanimate)
            {
                assert(current_funccal->rettv);
                rettv = current_funccal->rettv;
            }

            if(rettv != NULL)
            {
                // Store the value of the pending return.
                cstack->cs_rettv[idx] = xcalloc(1, sizeof(typval_st));
                *(typval_st *)cstack->cs_rettv[idx] = *(typval_st *)rettv;
            }
            else
            {
                cstack->cs_rettv[idx] = NULL;
            }

            if(reanimate)
            {
                // The pending return value could be overwritten by a ":return"
                // without argument in a finally clause; reset the default
                // return value.
                current_funccal->rettv->v_type = kNvarNumber;
                current_funccal->rettv->vval.v_number = 0;
            }
        }

        report_make_pending(kCSTflgReturn, rettv);
    }
    else
    {
        current_funccal->returned = TRUE;

        // If the return is carried out now, store the return value. For
        // a return immediately after reanimation, the value is already
        // there.
        if(!reanimate && rettv != NULL)
        {
            tv_clear(current_funccal->rettv);
            *current_funccal->rettv = *(typval_st *)rettv;

            if(!is_cmd)
            {
                xfree(rettv);
            }
        }
    }

    return idx < 0;
}

/// Generate a return command for producing the value of "rettv". The result
/// is an allocated string. Used by report_pending() for verbose messages.
uchar_kt *get_return_cmd(void *rettv)
{
    uchar_kt *s = NULL;
    uchar_kt *tofree = NULL;

    if(rettv != NULL)
    {
        tofree = s = (uchar_kt *) encode_tv2echo((typval_st *) rettv, NULL);
    }

    if(s == NULL)
    {
        s = (uchar_kt *)"";
    }

    ustrcpy(IObuff, ":return ");
    ustrlcpy(IObuff + 8, s, IOSIZE - 8);

    if(ustrlen(s) + 8 >= IOSIZE)
    {
        ustrcpy(IObuff + IOSIZE - 4, "...");
    }

    xfree(tofree);
    return ustrdup(IObuff);
}

/// Get next function line.
/// Called by do_cmdline() to get the next line.
/// Returns allocated string, or NULL for end of function.
uchar_kt *get_func_line(int FUNC_ARGS_UNUSED_MATCH(c),
                      void *cookie,
                      int FUNC_ARGS_UNUSED_MATCH(indent))
{
    func_call_st *fcp = (func_call_st *)cookie;
    ufunc_st *fp = fcp->func;
    uchar_kt *retval;
    garray_st *gap; // growarray with function lines

    // If breakpoints have been added/deleted need to check for it.
    if(fcp->dbg_tick != debug_tick)
    {
        fcp->breakpoint =
            dbg_find_breakpoint(FALSE, fp->uf_name, sourcing_lnum);

        fcp->dbg_tick = debug_tick;
    }

    if(do_profiling == PROF_YES)
    {
        func_line_end(cookie);
    }

    gap = &fp->uf_lines;

    if(((fp->uf_flags & FC_ABORT) && did_emsg && !aborted_in_try())
       || fcp->returned)
    {
        retval = NULL;
    }
    else
    {
        // Skip NULL lines (continuation lines).
        while(fcp->linenr < gap->ga_len
              && ((uchar_kt **)(gap->ga_data))[fcp->linenr] == NULL)
        {
            ++fcp->linenr;
        }

        if(fcp->linenr >= gap->ga_len)
        {
            retval = NULL;
        }
        else
        {
            retval = ustrdup(((uchar_kt **)(gap->ga_data))[fcp->linenr++]);
            sourcing_lnum = fcp->linenr;

            if(do_profiling == PROF_YES)
            {
                func_line_start(cookie);
            }
        }
    }

    // Did we encounter a breakpoint ?
    if(fcp->breakpoint != 0 && fcp->breakpoint <= sourcing_lnum)
    {
        dbg_breakpoint(fp->uf_name, sourcing_lnum);

        // Find next breakpoint.
        fcp->breakpoint =
            dbg_find_breakpoint(FALSE, fp->uf_name, sourcing_lnum);

        fcp->dbg_tick = debug_tick;
    }

    return retval;
}

/// Called when starting to read a function line.
/// "sourcing_lnum" must be correct!
/// When skipping lines it may not actually be executed, but we won't find out
/// until later and we need to store the time now.
void func_line_start(void *cookie)
{
    func_call_st *fcp = (func_call_st *)cookie;
    ufunc_st *fp = fcp->func;

    if(fp->uf_profiling
       && sourcing_lnum >= 1
       && sourcing_lnum <= fp->uf_lines.ga_len)
    {
        fp->uf_tml_idx = sourcing_lnum - 1;

        // Skip continuation lines.
        while(fp->uf_tml_idx > 0 && FUNCLINE(fp, fp->uf_tml_idx) == NULL)
        {
            --fp->uf_tml_idx;
        }

        fp->uf_tml_execed = FALSE;
        fp->uf_tml_start = profile_start();
        fp->uf_tml_children = profile_zero();
        fp->uf_tml_wait = profile_get_wait();
    }
}

/// Called when actually executing a function line.
void func_line_exec(void *cookie)
{
    func_call_st *fcp = (func_call_st *)cookie;
    ufunc_st *fp = fcp->func;

    if(fp->uf_profiling && fp->uf_tml_idx >= 0)
    {
        fp->uf_tml_execed = TRUE;
    }
}

/// Called when done with a function line.
void func_line_end(void *cookie)
{
    func_call_st *fcp = (func_call_st *)cookie;
    ufunc_st *fp = fcp->func;

    if(fp->uf_profiling && fp->uf_tml_idx >= 0)
    {
        if(fp->uf_tml_execed)
        {
            ++fp->uf_tml_count[fp->uf_tml_idx];
            fp->uf_tml_start = profile_end(fp->uf_tml_start);

            fp->uf_tml_start =
                profile_sub_wait(fp->uf_tml_wait, fp->uf_tml_start);

            fp->uf_tml_total[fp->uf_tml_idx] =
                profile_add(fp->uf_tml_total[fp->uf_tml_idx],
                            fp->uf_tml_start);

            fp->uf_tml_self[fp->uf_tml_idx] =
                profile_self(fp->uf_tml_self[fp->uf_tml_idx],
                             fp->uf_tml_start,
                             fp->uf_tml_children);
        }

        fp->uf_tml_idx = -1;
    }
}

/// Return TRUE if the currently active function should be ended, because a
/// return was encountered or an error occurred. Used inside a ":while".
int func_has_ended(void *cookie)
{
    func_call_st *fcp = (func_call_st *)cookie;

    // Ignore the "abort" flag if the abortion behavior has
    // been changed due to an error inside a try conditional.
    return ((fcp->func->uf_flags & FC_ABORT) && did_emsg && !aborted_in_try())
           || fcp->returned;
}

/// return TRUE if cookie indicates a function which "abort"s on errors.
int func_has_abort(void *cookie)
{
    return ((func_call_st *)cookie)->func->uf_flags & FC_ABORT;
}

static var_flavour_et var_flavour(uchar_kt *varname)
{
    uchar_kt *p = varname;

    if(ASCII_ISUPPER(*p))
    {
        while(*(++p))
        {
            if(ASCII_ISLOWER(*p))
            {
                return VAR_FLAVOUR_SESSION;
            }
        }

        return VAR_FLAVOUR_SHADA;
    }
    else
    {
        return VAR_FLAVOUR_DEFAULT;
    }
}

/// Search hashitem in parent scope.
hashitem_st *find_hi_in_scoped_ht(const char *name, hashtable_st **pht)
{
    if(current_funccal == NULL || current_funccal->func->uf_scoped == NULL)
    {
        return NULL;
    }

    func_call_st *old_current_funccal = current_funccal;
    hashitem_st *hi = NULL;
    const size_t namelen = strlen(name);
    const char *varname;

    // Search in parent scope which is possible to reference from lambda
    current_funccal = current_funccal->func->uf_scoped;

    while(current_funccal != NULL)
    {
        hashtable_st *ht = find_var_ht(name, namelen, &varname);

        if(ht != NULL && *varname != NUL)
        {
            hi = hash_find_len(ht, varname, namelen - (varname - name));

            if(!HASHITEM_EMPTY(hi))
            {
                *pht = ht;
                break;
            }
        }

        if(current_funccal == current_funccal->func->uf_scoped)
        {
            break;
        }

        current_funccal = current_funccal->func->uf_scoped;
    }

    current_funccal = old_current_funccal;
    return hi;
}

/// Search variable in parent scope.
dictitem_st *find_var_in_scoped_ht(const char *name,
                                  const size_t namelen,
                                  int no_autoload)
{
    if(current_funccal == NULL
       || current_funccal->func->uf_scoped == NULL)
    {
        return NULL;
    }

    dictitem_st *v = NULL;
    func_call_st *old_current_funccal = current_funccal;
    const char *varname;

    // Search in parent scope which is possible to reference from lambda
    current_funccal = current_funccal->func->uf_scoped;

    while(current_funccal)
    {
        hashtable_st *ht = find_var_ht(name, namelen, &varname);

        if(ht != NULL && *varname != NUL)
        {
            v = find_var_in_ht(ht,
                               *name,
                               varname,
                               namelen - (size_t)(varname - name),
                               no_autoload);

            if(v != NULL)
            {
                break;
            }
        }

        if(current_funccal == current_funccal->func->uf_scoped)
        {
            break;
        }

        current_funccal = current_funccal->func->uf_scoped;
    }

    current_funccal = old_current_funccal;
    return v;
}

/// Iterate over global variables
///
/// @warning
/// No modifications to global variable dictionary must be performed
/// while iteration is in progress.
///
/// @param[in]   iter   Iterator. Pass NULL to start iteration.
/// @param[out]  name   Variable name.
/// @param[out]  rettv  Variable value.
///
/// @return
/// Pointer that needs to be passed to next `var_shada_iter` invocation
/// or NULL to indicate that iteration is over.
const void *var_shada_iter(const void *const iter,
                           const char **const name,
                           typval_st *rettv)
FUNC_ATTR_NONNULL_ARG(2, 3)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    const hashitem_st *hi;
    const hashitem_st *hifirst = globvarht.ht_array;
    const size_t hinum = (size_t) globvarht.ht_mask + 1;
    *name = NULL;

    if(iter == NULL)
    {
        hi = globvarht.ht_array;

        while((size_t)(hi - hifirst) < hinum
              && (HASHITEM_EMPTY(hi)
                  || var_flavour(hi->hi_key) != VAR_FLAVOUR_SHADA))
        {
            hi++;
        }

        if((size_t)(hi - hifirst) == hinum)
        {
            return NULL;
        }
    }
    else
    {
        hi = (const hashitem_st *) iter;
    }

    *name = (char *)TV_DICT_HI2DI(hi)->di_key;
    tv_copy(&TV_DICT_HI2DI(hi)->di_tv, rettv);

    while((size_t)(++hi - hifirst) < hinum)
    {
        if(!HASHITEM_EMPTY(hi)
           && var_flavour(hi->hi_key) == VAR_FLAVOUR_SHADA)
        {
            return hi;
        }
    }

    return NULL;
}

void var_set_global(const char *const name, typval_st vartv)
{
    func_call_st *const saved_current_funccal = current_funccal;
    current_funccal = NULL;
    set_var(name, strlen(name), &vartv, false);
    current_funccal = saved_current_funccal;
}

int store_session_globals(FILE *fd)
{
    TV_DICT_ITER(&globvardict, this_var,
    {
        if((this_var->di_tv.v_type == kNvarNumber
            || this_var->di_tv.v_type == kNvarString)
           && var_flavour(this_var->di_key) == VAR_FLAVOUR_SESSION)
        {
            // Escape special characters with a backslash.
            // Turn a LF and CR into \n and \r.
            uchar_kt *const p =
                ustrdup_escape((const uchar_kt *)tv_get_string(&this_var->di_tv),
                               (const uchar_kt *)"\\\"\n\r");

            for(uchar_kt *t = p; *t != NUL; t++)
            {
                if(*t == '\n')
                {
                    *t = 'n';
                }
                else if(*t == '\r')
                {
                    *t = 'r';
                }
            }

            if((fprintf(fd,
                        "let %s = %c%s%c",
                        this_var->di_key,
                        ((this_var->di_tv.v_type == kNvarString) ? '"' : ' '),
                        p,
                        ((this_var->di_tv.v_type == kNvarString) ? '"' : ' ')) < 0)
               || put_eol(fd) == FAIL)
            {
                xfree(p);
                return FAIL;
            }

            xfree(p);
        }
        else if(this_var->di_tv.v_type == kNvarFloat
                && var_flavour(this_var->di_key) == VAR_FLAVOUR_SESSION)
        {
            float_kt f = this_var->di_tv.vval.v_float;
            int sign = ' ';

            if(f < 0)
            {
                f = -f;
                sign = '-';
            }

            if((fprintf(fd, "let %s = %c%f", this_var->di_key, sign, f) < 0)
               || put_eol(fd) == FAIL)
            {
                return FAIL;
            }
        }
    });
    return OK;
}

/// Display script name where an item was last set.
/// Should only be invoked when 'verbose' is non-zero.
void last_set_msg(script_id_kt scriptID)
{
    if(scriptID != 0)
    {
        uchar_kt *p = usr_home_replace_malloc(NULL, get_scriptname(scriptID));
        verbose_enter();
        MSG_PUTS(_("\n\tLast set from "));
        MSG_PUTS(p);
        xfree(p);
        verbose_leave();
    }
}

/// reset v:option_new, v:option_old and v:option_type
void reset_v_option_vars(void)
{
    set_vim_var_string(VV_OPTION_NEW,  NULL, -1);
    set_vim_var_string(VV_OPTION_OLD,  NULL, -1);
    set_vim_var_string(VV_OPTION_TYPE, NULL, -1);
}

/// Adjust a filename, according to a string of modifiers.
/// *fnamep must be NUL terminated when called. When returning, the length is
/// determined by *fnamelen. When there is an error, *fnamep is set to NULL.
///
/// @param src        string with modifiers
/// @param usedlen    characters after src that are used
/// @param fnamep     file name so far
/// @param bufp       buffer for allocated file name or NULL
/// @param fnamelen   length of fnamep
///
/// @return
/// Returns VALID_ flags or -1 for failure.
int modify_fname(uchar_kt *src,
                 size_t *usedlen,
                 uchar_kt **fnamep,
                 uchar_kt **bufp,
                 size_t *fnamelen)
{
    int valid = 0;
    uchar_kt *tail;
    uchar_kt *s, *p, *pbuf;
    uchar_kt dirname[MAXPATHL];
    int c;
    int has_fullname = 0;

repeat:

    // ":p" - full path/file_name
    if(src[*usedlen] == ':' && src[*usedlen + 1] == 'p')
    {
        has_fullname = 1;
        valid |= VALID_PATH;
        *usedlen += 2;

        // Expand "~/path" for all systems and "~user/path" for Unix
        if((*fnamep)[0] == '~'
#if !defined(UNIX)
           && ((*fnamep)[1] == '/'
    #ifdef BACKSLASH_IN_FILENAME
               || (*fnamep)[1] == '\\'
    #endif
               || (*fnamep)[1] == NUL)
#endif
          )
        {
            *fnamep = expand_env_save(*fnamep);
            xfree(*bufp); // free any allocated file name
            *bufp = *fnamep;

            if(*fnamep == NULL)
            {
                return -1;
            }
        }

        // When "/." or "/.." is used: force expansion to get rid of it.
        for(p = *fnamep; *p != NUL; mb_ptr_adv(p))
        {
            if(vim_ispathsep(*p)
               && p[1] == '.'
               && (p[2] == NUL
                   || vim_ispathsep(p[2])
                   || (p[2] == '.' && (p[3] == NUL || vim_ispathsep(p[3])))))
            {
                break;
            }
        }

        // FullName_save() is slow, don't use it when not needed.
        if(*p != NUL || !vim_isAbsName(*fnamep))
        {
            *fnamep = (uchar_kt *)FullName_save((char *)*fnamep, *p != NUL);
            xfree(*bufp); // free any allocated file name
            *bufp = *fnamep;

            if(*fnamep == NULL)
            {
                return -1;
            }
        }

        // Append a path separator to a directory.
        if(os_isdir(*fnamep))
        {
            // Make room for one or two extra characters.
            *fnamep = ustrndup(*fnamep, ustrlen(*fnamep) + 2);
            xfree(*bufp); // free any allocated file name
            *bufp = *fnamep;

            if(*fnamep == NULL)
            {
                return -1;
            }

            add_pathsep((char *)*fnamep);
        }
    }

    // ":." - path relative to the current directory
    // ":~" - path relative to the home directory
    // ":8" - shortname path - postponed till after
    while(src[*usedlen] == ':'
          && ((c = src[*usedlen + 1]) == '.' || c == '~' || c == '8'))
    {
        *usedlen += 2;

        if(c == '8')
        {
            continue;
        }

        pbuf = NULL;

        // Need full path first (use expand_env() to remove a "~/")
        if(!has_fullname)
        {
            if(c == '.' && **fnamep == '~')
            {
                p = pbuf = expand_env_save(*fnamep);
            }
            else
            {
                p = pbuf = (uchar_kt *)FullName_save((char *)*fnamep, FALSE);
            }
        }
        else
        {
            p = *fnamep;
        }

        has_fullname = 0;

        if(p != NULL)
        {
            if(c == '.')
            {
                os_dirname(dirname, MAXPATHL);
                s = path_shorten_fname(p, dirname);

                if(s != NULL)
                {
                    *fnamep = s;

                    if(pbuf != NULL)
                    {
                        xfree(*bufp); // free any allocated file name
                        *bufp = pbuf;
                        pbuf = NULL;
                    }
                }
            }
            else
            {
                usr_home_replace(NULL, p, dirname, MAXPATHL);

                // Only replace it when it starts with '~'
                if(*dirname == '~')
                {
                    s = ustrdup(dirname);
                    *fnamep = s;
                    xfree(*bufp);
                    *bufp = s;
                }
            }

            xfree(pbuf);
        }
    }

    tail = path_tail(*fnamep);
    *fnamelen = ustrlen(*fnamep);

    // ":h" - head, remove "/file_name", can be repeated
    // Don't remove the first "/" or "c:\"
    while(src[*usedlen] == ':' && src[*usedlen + 1] == 'h')
    {
        valid |= VALID_HEAD;
        *usedlen += 2;
        s = get_past_head(*fnamep);

        while(tail > s && after_pathsep((char *)s, (char *)tail))
        {
            mb_ptr_back(*fnamep, tail);
        }

        *fnamelen = (size_t)(tail - *fnamep);

        if(*fnamelen == 0)
        {
            // Result is empty.  Turn it into "." to make ":cd %:h" work.
            xfree(*bufp);
            *bufp = *fnamep = tail = ustrdup((uchar_kt *)".");
            *fnamelen = 1;
        }
        else
        {
            while(tail > s && !after_pathsep((char *)s, (char *)tail))
            {
                mb_ptr_back(*fnamep, tail);
            }
        }
    }

    // ":8" - shortname
    if(src[*usedlen] == ':' && src[*usedlen + 1] == '8')
    {
        *usedlen += 2;
    }

    // ":t" - tail, just the basename
    if(src[*usedlen] == ':' && src[*usedlen + 1] == 't')
    {
        *usedlen += 2;
        *fnamelen -= (size_t)(tail - *fnamep);
        *fnamep = tail;
    }

    // ":e" - extension, can be repeated
    // ":r" - root, without extension, can be repeated
    while(src[*usedlen] == ':'
          && (src[*usedlen + 1] == 'e' || src[*usedlen + 1] == 'r'))
    {
        // find a '.' in the tail:
        // - for second :e: before the current fname
        // - otherwise: The last '.'
        if(src[*usedlen + 1] == 'e' && *fnamep > tail)
        {
            s = *fnamep - 2;
        }
        else
        {
            s = *fnamep + *fnamelen - 1;
        }

        for(; s > tail; --s)
        {
            if(s[0] == '.')
            {
                break;
            }
        }

        if(src[*usedlen + 1] == 'e') // :e
        {
            if(s > tail)
            {
                *fnamelen += (size_t)(*fnamep - (s + 1));
                *fnamep = s + 1;
            }
            else if(*fnamep <= tail)
            {
                *fnamelen = 0;
            }
        }
        else // :r
        {
            if(s > tail) // remove one extension
            {
                *fnamelen = (size_t)(s - *fnamep);
            }
        }

        *usedlen += 2;
    }

    // ":s?pat?foo?" - substitute
    // ":gs?pat?foo?" - global substitute
    if(src[*usedlen] == ':'
       && (src[*usedlen + 1] == 's'
           || (src[*usedlen + 1] == 'g' && src[*usedlen + 2] == 's')))
    {
        uchar_kt *str;
        uchar_kt *pat;
        uchar_kt *sub;
        int sep;
        uchar_kt *flags;
        int didit = FALSE;
        flags = (uchar_kt *)"";
        s = src + *usedlen + 2;

        if(src[*usedlen + 1] == 'g')
        {
            flags = (uchar_kt *)"g";
            ++s;
        }

        sep = *s++;

        if(sep)
        {
            // find end of pattern
            p = ustrchr(s, sep);

            if(p != NULL)
            {
                pat = ustrndup(s, (int)(p - s));
                s = p + 1;

                // find end of substitution
                p = ustrchr(s, sep);

                if(p != NULL)
                {
                    sub = ustrndup(s, (int)(p - s));
                    str = ustrndup(*fnamep, *fnamelen);
                    *usedlen = (size_t)(p + 1 - src);
                    s = do_string_sub(str, pat, sub, NULL, flags);
                    *fnamep = s;
                    *fnamelen = ustrlen(s);
                    xfree(*bufp);
                    *bufp = s;
                    didit = TRUE;
                    xfree(sub);
                    xfree(str);
                }

                xfree(pat);
            }

            // after using ":s", repeat all the modifiers
            if(didit)
            {
                goto repeat;
            }
        }
    }

    if(src[*usedlen] == ':' && src[*usedlen + 1] == 'S')
    {
        // ustrdup_escape_shell() needs a NUL terminated string.
        c = (*fnamep)[*fnamelen];

        if(c != NUL)
        {
            (*fnamep)[*fnamelen] = NUL;
        }

        p = ustrdup_escape_shell(*fnamep, false, false);

        if(c != NUL)
        {
            (*fnamep)[*fnamelen] = c;
        }

        xfree(*bufp);
        *bufp = *fnamep = p;
        *fnamelen = ustrlen(p);
        *usedlen += 2;
    }

    return valid;
}

/// Perform a substitution on "str" with pattern "pat" and substitute "sub".
/// When "sub" is NULL "expr" is used, must be a kNvarUfunc or kNvarPartial.
/// "flags" can be "g" to do a global substitute.
/// Returns an allocated string, NULL for error.
uchar_kt *do_string_sub(uchar_kt *str,
                      uchar_kt *pat,
                      uchar_kt *sub,
                      typval_st *expr,
                      uchar_kt *flags)
{
    int sublen;
    regmatch_st regmatch;
    int do_all;
    uchar_kt *tail;
    uchar_kt *end;
    garray_st ga;
    uchar_kt *save_cpo;
    uchar_kt *zero_width = NULL;

    // Make 'cpoptions' empty, so that the 'l' flag doesn't work here
    save_cpo = p_cpo;
    p_cpo = empty_option;
    ga_init(&ga, 1, 200);
    do_all = (flags[0] == 'g');
    regmatch.rm_ic = p_ic;
    regmatch.regprog = regexp_compile(pat, RE_MAGIC + RE_STRING);

    if(regmatch.regprog != NULL)
    {
        tail = str;
        end = str + ustrlen(str);

        while(vim_regexec_nl(&regmatch, str, (columnum_kt)(tail - str)))
        {
            // Skip empty match except for first match.
            if(regmatch.startp[0] == regmatch.endp[0])
            {
                if(zero_width == regmatch.startp[0])
                {
                    // avoid getting stuck on a match with an empty string
                    int i = mb_ptr2len(tail);
                    memmove((uchar_kt *)ga.ga_data + ga.ga_len, tail, (size_t)i);
                    ga.ga_len += i;
                    tail += i;
                    continue;
                }

                zero_width = regmatch.startp[0];
            }

            // Get some space for a temporary buffer to do the substitution
            // into.  It will contain:
            // - The text up to where the match is.
            // - The substituted text.
            // - The text after the match.
            sublen = vim_regsub(&regmatch, sub, expr, tail, false, true, false);
            ga_grow(&ga, (int)((end - tail) + sublen - (regmatch.endp[0] - regmatch.startp[0])));

            // copy the text up to where the match is
            int i = (int)(regmatch.startp[0] - tail);
            memmove((uchar_kt *)ga.ga_data + ga.ga_len, tail, (size_t)i);

            // add the substituted text
            (void)vim_regsub(&regmatch,
                             sub,
                             expr,
                             (uchar_kt *)ga.ga_data + ga.ga_len + i,
                             true,
                             true,
                             false);

            ga.ga_len += i + sublen - 1;
            tail = regmatch.endp[0];

            if(*tail == NUL)
            {
                break;
            }

            if(!do_all)
            {
                break;
            }
        }

        if(ga.ga_data != NULL)
        {
            ustrcpy((char *)ga.ga_data + ga.ga_len, tail);
        }

        vim_regfree(regmatch.regprog);
    }

    uchar_kt *ret = ustrdup(ga.ga_data == NULL ? str : (uchar_kt *)ga.ga_data);
    ga_clear(&ga);

    if(p_cpo == empty_option)
    {
        p_cpo = save_cpo;
    }
    else
    {
        // Darn, evaluating {sub} expression or {expr} changed the value.
        free_string_option(save_cpo);
    }

    return ret;
}

static inline terminal_jobdata_st *common_job_init(char **argv,
                                               callback_st on_stdout,
                                               callback_st on_stderr,
                                               callback_st on_exit,
                                               bool pty,
                                               bool rpc,
                                               bool detach,
                                               const char *cwd)
{
    terminal_jobdata_st *data = xcalloc(1, sizeof(terminal_jobdata_st));

    data->stopped = false;
    data->on_stdout = on_stdout;
    data->on_stderr = on_stderr;
    data->on_exit = on_exit;
    data->events = multiqueue_new_child(main_loop.events);
    data->rpc = rpc;

    if(pty)
    {
        data->proc.pty = pty_process_init(&main_loop, data);
    }
    else
    {
        data->proc.uv = libuv_process_init(&main_loop, data);
    }

    process_st *proc = (process_st *)&data->proc;

    proc->argv = argv;
    proc->in = &data->in;
    proc->out = &data->out;

    if(!pty)
    {
        proc->err = &data->err;
    }

    proc->cb = eval_job_process_exit_cb;
    proc->events = data->events;
    proc->detach = detach;
    proc->cwd = cwd;
    return data;
}

/// common code for getting job callbacks for jobstart, termopen and rpcstart
///
/// @return true/false on success/failure.
static inline bool common_job_callbacks(dict_st *vopts,
                                        callback_st *on_stdout,
                                        callback_st *on_stderr,
                                        callback_st *on_exit)
{
    if(tv_dict_get_callback(vopts, S_LEN("on_stdout"), on_stdout)
       && tv_dict_get_callback(vopts, S_LEN("on_stderr"), on_stderr)
       && tv_dict_get_callback(vopts, S_LEN("on_exit"), on_exit))
    {
        vopts->dv_refcount++;
        return true;
    }

    callback_free(on_stdout);
    callback_free(on_stderr);
    callback_free(on_exit);
    return false;
}

static inline bool common_job_start(terminal_jobdata_st *data, typval_st *rettv)
{
    process_st *proc = (process_st *)&data->proc;

    if(proc->type == kProcessTypePty && proc->detach)
    {
        EMSG2(_(e_invarg2), "terminal/pty job cannot be detached");
        xfree(data->proc.pty.term_name);
        shell_free_argv(proc->argv);
        free_term_job_data_event((void **)&data);
        return false;
    }

    data->id = next_chan_id++;
    pmap_put(uint64_t)(jobs, data->id, data);
    data->refcount++;
    char *cmd = xstrdup(proc->argv[0]);
    int status = process_spawn(proc);

    if(status)
    {
        EMSG3(_(e_jobspawn), os_strerror(status), cmd);
        xfree(cmd);

        if(proc->type == kProcessTypePty)
        {
            xfree(data->proc.pty.term_name);
        }

        rettv->vval.v_number = proc->status;
        term_job_data_decref(data);
        return false;
    }

    xfree(cmd);

    if(data->rpc)
    {
        // the rpc channel takes over the in and out streams
        channel_from_process(proc, data->id);
    }
    else
    {
        wstream_init(proc->in, 0);

        if(proc->out)
        {
            rstream_init(proc->out, 0);
            rstream_start(proc->out, on_job_stdout, data);
        }
    }

    if(proc->err)
    {
        rstream_init(proc->err, 0);
        rstream_start(proc->err, on_job_stderr, data);
    }

    rettv->vval.v_number = data->id;
    return true;
}

static inline void free_term_job_data_event(void **argv)
{
    terminal_jobdata_st *data = argv[0];
    callback_free(&data->on_stdout);
    callback_free(&data->on_stderr);
    callback_free(&data->on_exit);
    multiqueue_free(data->events);
    pmap_del(uint64_t)(jobs, data->id);
    xfree(data);
}

static inline void free_term_job_data(terminal_jobdata_st *data)
{
    // data->queue may still be used after this function returns(process_wait), so
    // only free in the next event loop iteration
    multiqueue_put(main_loop.fast_events, free_term_job_data_event, 1, data);
}

// vimscript job callbacks must be executed on Nvim main loop
static inline void process_job_event(terminal_jobdata_st *data,
                                     callback_st *callback,
                                     const char *type,
                                     char *buf,
                                     size_t count,
                                     int status)
{
    job_event_st event_data;
    event_data.received = NULL;

    if(buf)
    {
        event_data.received = tv_list_alloc();
        char *ptr = buf;
        size_t remaining = count;
        size_t off = 0;

        while(off < remaining)
        {
            // append the line
            if(ptr[off] == NL)
            {
                tv_list_append_string(event_data.received, ptr, off);
                size_t skip = off + 1;
                ptr += skip;
                remaining -= skip;
                off = 0;
                continue;
            }

            if(ptr[off] == NUL)
            {
                // Translate NUL to NL
                ptr[off] = NL;
            }

            off++;
        }

        tv_list_append_string(event_data.received, ptr, off);
    }
    else
    {
        event_data.status = status;
    }

    event_data.data = data;
    event_data.callback = callback;
    event_data.type = type;
    on_job_event(&event_data);
}

static void on_job_stdout(stream_st *stream,
                          ringbuf_st *buf,
                          size_t count,
                          void *job,
                          bool eof)
{
    terminal_jobdata_st *data = job;
    on_job_output(stream, job, buf, count, eof, &data->on_stdout, "stdout");
}

static void on_job_stderr(stream_st *stream,
                          ringbuf_st *buf,
                          size_t count,
                          void *job,
                          bool eof)
{
    terminal_jobdata_st *data = job;
    on_job_output(stream, job, buf, count, eof, &data->on_stderr, "stderr");
}

static void on_job_output(stream_st *FUNC_ARGS_UNUSED_MATCH(stream),
                          terminal_jobdata_st *data,
                          ringbuf_st *buf,
                          size_t count,
                          bool eof,
                          callback_st *callback,
                          const char *type)
{
    if(eof)
    {
        return;
    }

    // stub variable, to keep reading consistent with the order
    // of events, only consider the count parameter.
    size_t r;
    char *ptr = rbuffer_read_ptr(buf, &r);

    // The order here matters, the terminal must receive the data first because
    // process_job_event will modify the read buffer(convert NULs into NLs)
    if(data->term)
    {
        terminal_receive(data->term, ptr, count);
    }

    rbuffer_consumed(buf, count);

    if(callback->type != kCallbackNone)
    {
        process_job_event(data, callback, type, ptr, count, 0);
    }
}

static void eval_job_process_exit_cb(process_st *proc, int status, void *d)
{
    terminal_jobdata_st *data = d;

    if(data->term && !data->exited)
    {
        data->exited = true;
        char msg[sizeof("\r\n[Process exited ]") + NUMBUFLEN];
        snprintf(msg, sizeof msg, "\r\n[Process exited %d]", proc->status);
        terminal_close(data->term, msg);
    }

    if(data->rpc)
    {
        channel_process_exit(data->id, status);
    }

    if(data->status_ptr)
    {
        *data->status_ptr = status;
    }

    process_job_event(data, &data->on_exit, "exit", NULL, 0, status);
    term_job_data_decref(data);
}

static void term_write(char *buf, size_t size, void *d)
{
    terminal_jobdata_st *job = d;

    if(job->in.closed)
    {
        // If the backing stream was closed abruptly, there may be write events
        // ahead of the terminal close event. Just ignore the writes.
        STATE_LOG("write failed: stream is closed");
        return;
    }

    wbuffer_st *wbuf = wstream_new_buffer(xmemdup(buf, size), size, 1, xfree);
    wstream_write(&job->in, wbuf);
}

static void term_resize(uint16_t width, uint16_t height, void *d)
{
    terminal_jobdata_st *data = d;
    pty_process_resize(&data->proc.pty, width, height);
}

static inline void term_delayed_free(void **argv)
{
    terminal_jobdata_st *j = argv[0];

    if(j->in.pending_reqs || j->out.pending_reqs || j->err.pending_reqs)
    {
        multiqueue_put(j->events, term_delayed_free, 1, j);
        return;
    }

    terminal_destroy(j->term);
    term_job_data_decref(j);
}

static void term_close(void *d)
{
    terminal_jobdata_st *data = d;

    if(!data->exited)
    {
        data->exited = true;
        process_stop((process_st *)&data->proc);
    }

    multiqueue_put(data->events, term_delayed_free, 1, data);
}

static void term_job_data_decref(terminal_jobdata_st *data)
{
    if(!(--data->refcount))
    {
        free_term_job_data(data);
    }
}

static void on_job_event(job_event_st *ev)
{
    if(!ev->callback)
    {
        return;
    }

    typval_st argv[4];
    argv[0].v_type = kNvarNumber;
    argv[0].v_lock = 0;
    argv[0].vval.v_number = ev->data->id;

    if(ev->received)
    {
        argv[1].v_type = kNvarList;
        argv[1].v_lock = 0;
        argv[1].vval.v_list = ev->received;
        argv[1].vval.v_list->lv_refcount++;
    }
    else
    {
        argv[1].v_type = kNvarNumber;
        argv[1].v_lock = 0;
        argv[1].vval.v_number = ev->status;
    }

    argv[2].v_type = kNvarString;
    argv[2].v_lock = 0;
    argv[2].vval.v_string = (uint8_t *)ev->type;

    typval_st rettv = TV_INITIAL_VALUE;
    callback_call(ev->callback, 3, argv, &rettv);
    tv_clear(&rettv);
}

static terminal_jobdata_st *find_job(uint64_t id)
{
    terminal_jobdata_st *data = pmap_get(uint64_t)(jobs, id);

    if(!data || data->stopped)
    {
        return NULL;
    }

    return data;
}

static void script_host_eval(char *name, typval_st *argvars, typval_st *rettv)
{
    if(check_restricted() || check_secure())
    {
        return;
    }

    if(argvars[0].v_type != kNvarString)
    {
        EMSG(_(e_invarg));
        return;
    }

    list_st *args = tv_list_alloc();

    tv_list_append_string(args, (const char *)argvars[0].vval.v_string, -1);
    *rettv = eval_call_provider(name, "eval", args);
}

typval_st eval_call_provider(char *provider, char *method, list_st *arguments)
{
    char func[256];
    int name_len = snprintf(func, sizeof(func), "provider#%s#Call", provider);

    // Save caller scope information
    caller_scope_st saved_provider_caller_scope = provider_caller_scope;
    provider_caller_scope = (caller_scope_st) {
        .SID = current_SID,
        .sourcing_name = sourcing_name,
        .sourcing_lnum = sourcing_lnum,
        .autocmd_fname = autocmd_fname,
        .autocmd_match = autocmd_match,
        .autocmd_fname_full = autocmd_fname_full,
        .autocmd_bufnr = autocmd_bufnr,
        .funccalp = save_funccal()
    };

    provider_call_nesting++;

    typval_st argvars[3] = {
        {.v_type = kNvarString, .vval.v_string = (uint8_t *)method, .v_lock = 0},
        {.v_type = kNvarList, .vval.v_list = arguments, .v_lock = 0},
        {.v_type = kNvarUnknown}
    };

    typval_st rettv = {.v_type = kNvarUnknown, .v_lock = 0};
    arguments->lv_refcount++;
    int dummy;

    (void)call_func((const uchar_kt *)func,
                    name_len,
                    &rettv,
                    2,
                    argvars,
                    NULL,
                    curwin->w_cursor.lnum,
                    curwin->w_cursor.lnum,
                    &dummy,
                    true,
                    NULL,
                    NULL);

    tv_list_unref(arguments);

    // Restore caller scope information
    restore_funccal(provider_caller_scope.funccalp);
    provider_caller_scope = saved_provider_caller_scope;
    provider_call_nesting--;

    return rettv;
}

#define check_provider(name)                                               \
    if(has_##name == -1)                                                   \
    {                                                                      \
        has_##name = !!find_func((uchar_kt *)"provider#" #name "#Call");     \
        if(!has_##name)                                                    \
        {                                                                  \
            script_autoload("provider#" #name "#Call",                     \
                            sizeof("provider#" #name "#Call") - 1, false); \
            has_##name = !!find_func((uchar_kt *)"provider#" #name "#Call"); \
        }                                                                  \
    }

bool eval_has_provider(const char *name)
{
    static int has_clipboard = -1;
    static int has_python = -1;
    static int has_python3 = -1;
    static int has_ruby = -1;

    if(!strcmp(name, "clipboard"))
    {
        check_provider(clipboard);
        return has_clipboard;
    }
    else if(!strcmp(name, "python3"))
    {
        check_provider(python3);
        return has_python3;
    }
    else if(!strcmp(name, "python"))
    {
        check_provider(python);
        return has_python;
    }
    else if(!strcmp(name, "ruby"))
    {
        check_provider(ruby);
        return has_ruby;
    }

    return false;
}
