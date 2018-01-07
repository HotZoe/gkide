/// @file nvim/eval/typval.h

#ifndef NVIM_EVAL_TYPVAL_H
#define NVIM_EVAL_TYPVAL_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "nvim/types.h"
#include "nvim/hashtab.h"
#include "nvim/garray.h"
#include "nvim/mbyte.h"
#include "nvim/func_attr.h"
#include "nvim/lib/queue.h"
#include "nvim/profile.h" // for proftime_T
#include "nvim/pos.h" // for linenr_T
#include "nvim/gettext.h"
#include "nvim/message.h"
#include "nvim/macros.h"

/// Maximal possible value of number_kt variable
#define VARNUMBER_MAX INT_MAX

/// Mimimal possible value of number_kt variable
#define VARNUMBER_MIN INT_MIN

/// %d printf format specifier for number_kt
#define PRIdVARNUMBER "d"

typedef int               number_kt;
// =======================string_st
typedef struct ufunc_s    ufunc_st;
typedef struct list_s     list_st;
typedef struct dict_s     dict_st;
typedef double            float_kt;
/// @todo remove, boolean
/// =======================special
typedef struct partial_s  partial_st;
/// @todo add userdata
/// =======================userdata_st

/// NvimL(nvl) variable types, @see typval_st::v_type
typedef enum
{
    kNvarUnknown = 0,  ///< Unknown (unspecified) value.
    kNvarNumber,       ///< Number, number_kt, typval_st::v_number
    kNvarString,       ///< String, string_st, typval_st::v_string
    kNvarUfunc,        ///< User Function, ufunc_st, typval_st::v_string
                       ///< is the function name.
    kNvarList,         ///< List, list_st, typval_st::v_list
    kNvarDict,         ///< Dictionary, dict_st, typval_st::v_dict
    kNvarFloat,        ///< Float, float_kt, typval_st::v_float
    kNvarSpecial,      ///< Special value (true, false, null),
                       ///< special_st, typval_st::v_special
    kNvarPartial,      ///< Partial, partial_st, typval_st::v_partial
} nvlvar_type_et;

typedef enum
{
    kCallbackNone,
    kCallbackFuncref,
    kCallbackPartial,
} callback_type_et;

typedef struct
{
    union
    {
        uchar_kt *funcref;
        partial_st *partial;
    } data;
    callback_type_et type;
} callback_st;

#define CALLBACK_NONE ((callback_st){ .type = kCallbackNone })

/// Structure holding dictionary watcher
typedef struct dict_watcher_s
{
    callback_st callback;
    char *key_pattern;
    size_t key_pattern_len;
    queue_st node;
    /// prevent recursion if the dict is changed in the callback
    bool busy;
} dict_watcher_st;

/// nvl special variable value, @see special_st
typedef enum
{
    kSpecialVarFalse,  ///< v:false
    kSpecialVarTrue,   ///< v:true
    kSpecialVarNull,   ///< v:null
} nvlvar_special_value_et;

/// Variable lock status for typval_st::v_lock
typedef enum
{
    kNvlVarUnlocked = 0, ///< Not locked.
    kNvlVarLocked   = 1, ///< User locked, can be unlocked.
    kNvlVarFixed    = 2, ///< Locked forever.
} nvlvar_lock_status_et;

/// Structure that holds an internal variable value
typedef struct
{
    nvlvar_type_et v_type;        ///< Variable type.
    nvlvar_lock_status_et v_lock; ///< Variable lock status.

    union typval_vval_union
    {
        ///< Number, for kNvarNumber.
        number_kt v_number;

        ///< Special value, for kNvarSpecial.
        nvlvar_special_value_et v_special;

        ///< Floating-point number, for kNvarFloat.
        float_kt v_float;

        ///< String, for kNvarString and kNvarUfunc, can be NULL.
        uchar_kt *v_string;

        ///< List for kNvarList, can be NULL.
        list_st *v_list;

        ///< Dictionary for kNvarDict, can be NULL.
        dict_st *v_dict;

        ///< Closure: function with args, for kNvarPartial
        partial_st *v_partial;
    } vval; ///< Actual value.
} typval_st;

/// Values for dict_st::dv_scope
typedef enum
{
    /// Not a scope dictionary.
    VAR_NO_SCOPE = 0,
    /// Scope dictionary which requires prefix (a:, v:, ...).
    VAR_SCOPE = 1,
    /// Scope dictionary which may be accessed without prefix (l:, g:).
    VAR_DEF_SCOPE = 2,
} nvlvar_scope_type_et;

/// Structure to hold an item of a list
typedef struct listitem_s listitem_st;

struct listitem_s
{
    listitem_st *li_next; ///< Next item in list.
    listitem_st *li_prev; ///< Previous item in list.
    typval_st li_tv;      ///< Item value.
};

/// Structure used by those that are using an item in a list
typedef struct list_watcher_s list_watcher_st;

struct list_watcher_s
{
    listitem_st *lw_item;     ///< Item being watched.
    list_watcher_st *lw_next; ///< Next watcher.
};

/// Structure to hold info about a list
struct list_s
{
    listitem_st *lv_first;         ///< First item, NULL if none.
    listitem_st *lv_last;          ///< Last item, NULL if none.
    int lv_refcount;               ///< Reference count.
    int lv_len;                    ///< Number of items.
    list_watcher_st *lv_watch;     ///< First watcher, NULL if none.
    int lv_idx;                    ///< Index of a cached item, used for
                                   ///< optimising repeated l[idx].
    listitem_st *lv_idx_item;      ///< When not NULL item at index "lv_idx".
    int lv_copyID;                 ///< ID used by deepcopy().
    list_st *lv_copylist;          ///< Copied list used by deepcopy().
    nvlvar_lock_status_et lv_lock; ///< Zero, kNvlVarLocked, kNvlVarFixed.
    list_st *lv_used_next;         ///< next list in used lists list.
    list_st *lv_used_prev;         ///< Previous list in used lists list.
};

/// Static list with 10 items.
/// Use init_static_list"()" to initialize.
typedef struct
{
    list_st sl_list; // must be first
    listitem_st sl_items[10];
} staticList10_T;

// Structure to hold an item of a Dictionary.
// Also used for a variable.
// The key is copied into "di_key" to avoid an extra alloc/free for it.
struct dictitem_S
{
    typval_st di_tv;    ///< type and value of the variable
    uchar_kt di_flags;  ///< flags (only used for variable)
    uchar_kt di_key[1]; ///< key (actually longer!)
};

#define TV_DICTITEM_STRUCT(KEY_LEN)                      \
    struct                                               \
    {                                                    \
        typval_st di_tv;          /* scope dictionary */ \
        uint8_t di_flags;         /* Flags.           */ \
        uchar_kt di_key[KEY_LEN]; /* Key value.       */ \
    }

/// Structure to hold a scope dictionary
///
/// @warning Must be compatible with dictitem_T.
///
/// For use in find_var_in_ht to pretend that it found
/// dictionary item when it finds scope dictionary.
typedef TV_DICTITEM_STRUCT(1) scope_dict_T;

/// Structure to hold an item of a Dictionary
///
/// @warning Must be compatible with #scope_dict_T.
///
/// Also used for a variable.
typedef TV_DICTITEM_STRUCT() dictitem_T;

/// Flags for dictitem_T::di_flags
typedef enum
{
    DI_FLAGS_RO = 1,      ///< Read-only value
    DI_FLAGS_RO_SBX = 2,  ///< Value, read-only in the sandbox
    DI_FLAGS_FIX = 4,     ///< Fixed value: cannot be :unlet or remove()d.
    DI_FLAGS_LOCK = 8,    ///< Locked value.
    DI_FLAGS_ALLOC = 16,  ///< Separately allocated.
} DictItemFlags;

/// Structure representing a Dictionary
struct dict_s
{
    /// Whole dictionary lock status.
    nvlvar_lock_status_et dv_lock;
    /// Non-zero (#VAR_SCOPE, #VAR_DEF_SCOPE) if
    /// dictionary represents a scope (i.e. g:, l: ...).
    nvlvar_scope_type_et dv_scope;

    int dv_refcount;        ///< Reference count.
    int dv_copyID;          ///< ID used when recursivery traversing a value.
    hashtab_T dv_hashtab;  ///< Hashtab containing all items.
    dict_st *dv_copydict;   ///< Copied dict used by deepcopy().
    dict_st *dv_used_next;  ///< Next dictionary in used dictionaries list.
    dict_st *dv_used_prev;  ///< Previous dictionary in used dictionaries list.
    queue_st watchers;      ///< Dictionary key watchers set by user code.
};

/// Type used for script ID
typedef int scid_T;

/// Format argument for scid_T
#define PRIdSCID "d"

/// Structure to hold info for a function that is currently being executed.
typedef struct funccall_S funccall_T;

/// Structure to hold info for a user function.
struct ufunc_s
{
    int uf_varargs;             ///< variable nr of arguments
    int uf_flags;               ///<
    int uf_calls;               ///< nr of active calls
    bool uf_cleared;            ///< func_clear() was already called
    garray_T uf_args;          ///< arguments
    garray_T uf_lines;         ///< function lines
    int uf_profiling;           ///< true when func is being profiled

    // Profiling the function as a whole.
    int uf_tm_count;            ///< nr of calls
    proftime_T uf_tm_total;    ///< time spent in function + children
    proftime_T uf_tm_self;     ///< time spent in function itself
    proftime_T uf_tm_children; ///< time spent in children this call

    // Profiling the function per line.
    int *uf_tml_count;          ///< nr of times line was executed
    proftime_T *uf_tml_total;  ///< time spent in a line + children
    proftime_T *uf_tml_self;   ///< time spent in a line itself
    proftime_T uf_tml_start;   ///< start time for current line
    proftime_T uf_tml_children;///< time spent in children for this line
    proftime_T uf_tml_wait;    ///< start wait time for current line
    int uf_tml_idx;             ///< index of line being timed; -1 if none
    int uf_tml_execed;          ///< line being timed was executed
    scid_T uf_script_ID;       ///< ID of script where function was defined,
                                ///< used for s: variables
    int uf_refcount;            ///< reference count, see func_name_refcount()
    funccall_T *uf_scoped;     ///< l: local variables for closure
    uchar_kt uf_name[1];        ///< name of function (actually longer);
                                ///< can start with <SNR>123_
                                ///< (<SNR> is K_SPECIAL, KS_EXTRA, KE_SNR)
};

/// Maximum number of function arguments
#define MAX_FUNC_ARGS   20

/// hold partial information
struct partial_s
{
    int pt_refcount;   ///< Reference count.
    uchar_kt *pt_name; ///< Function name; when NULL use pt_func->name.
    ufunc_st *pt_func; ///< Function pointer; when NULL lookup function with pt_name.
    bool pt_auto;      ///< When true the partial was created by using dict.member
                       ///< in handle_subscript().
    int pt_argc;       ///< Number of arguments.
    typval_st *pt_argv;///< Arguments in allocated array.
    dict_st *pt_dict;  ///< Dict for "self".
};

/// Structure used for explicit stack while garbage collecting hash tables
typedef struct ht_stack_S
{
    hashtab_T *ht;
    struct ht_stack_S *prev;
} ht_stack_T;

/// Structure used for explicit stack while garbage collecting lists
typedef struct list_stack_S
{
    list_st *list;
    struct list_stack_S *prev;
} list_stack_T;

// In a hashtab item "hi_key" points to "di_key" in a dictitem.
// This avoids adding a pointer to the hashtab item.

/// Convert a hashitem pointer to a dictitem pointer
#define TV_DICT_HI2DI(hi) \
    ((dictitem_T *)((hi)->hi_key - offsetof(dictitem_T, di_key)))

static inline long tv_list_len(const list_st *const l)
REAL_FATTR_PURE
REAL_FATTR_WARN_UNUSED_RESULT;

/// Get the number of items in a list
///
/// @param[in]  l  List to check.
static inline long tv_list_len(const list_st *const l)
{
    if(l == NULL)
    {
        return 0;
    }

    return l->lv_len;
}

static inline long tv_dict_len(const dict_st *const d)
REAL_FATTR_PURE
REAL_FATTR_WARN_UNUSED_RESULT;

/// Get the number of items in a Dictionary
///
/// @param[in]  d  Dictionary to check.
static inline long tv_dict_len(const dict_st *const d)
{
    if(d == NULL)
    {
        return 0L;
    }

    return (long)d->dv_hashtab.ht_used;
}

static inline bool tv_dict_is_watched(const dict_st *const d)
REAL_FATTR_PURE
REAL_FATTR_WARN_UNUSED_RESULT;

/// Check if dictionary is watched
///
/// @param[in]  d  Dictionary to check.
///
/// @return true if there is at least one watcher.
static inline bool tv_dict_is_watched(const dict_st *const d)
{
    return d && !queue_empty(&d->watchers);
}

/// Initialize VimL object
///
/// Initializes to unlocked kNvarUnknown object.
///
/// @param[out]  tv  Object to initialize.
static inline void tv_init(typval_st *const tv)
{
    if(tv != NULL)
    {
        memset(tv, 0, sizeof(*tv));
    }
}

#define TV_INITIAL_VALUE  \
    ((typval_st) { .v_type = kNvarUnknown, .v_lock = kNvlVarUnlocked, })

/// Empty string
///
/// Needed for hack which allows not allocating empty string and still not
/// crashing when freeing it.
extern const char *const tv_empty_string;

/// Specifies that free_unref_items() function has (not) been entered
extern bool tv_in_free_unref_items;

/// Iterate over a dictionary
///
/// @param[in] d     Dictionary to iterate over.
/// @param     di    Name of the variable with current dictitem_T entry.
/// @param     code  Cycle body.
#define TV_DICT_ITER(d, di, code)                          \
    HASHTAB_ITER(&(d)->dv_hashtab, di##hi_, {              \
        {                                                  \
            dictitem_T *const di = TV_DICT_HI2DI(di##hi_); \
            {                                              \
                code                                       \
            }                                              \
        }                                                  \
    })

static inline bool tv_get_float_chk(const typval_st *const tv,
                                    float_kt *const ret_f)
REAL_FATTR_NONNULL_ALL
REAL_FATTR_WARN_UNUSED_RESULT;

/// FIXME circular dependency, cannot import message.h.
bool emsgf(const char *const fmt, ...);

/// Get the float value
///
/// Raises an error if object is not number or floating-point.
///
/// @param[in]   tv     VimL object to get value from.
/// @param[out]  ret_f  Location where resulting float is stored.
///
/// @return true in case of success, false if tv is not a number or float.
static inline bool tv_get_float_chk(const typval_st *const tv,
                                    float_kt *const ret_f)
{
    if(tv->v_type == kNvarFloat)
    {
        *ret_f = tv->vval.v_float;
        return true;
    }

    if(tv->v_type == kNvarNumber)
    {
        *ret_f = (float_kt)tv->vval.v_number;
        return true;
    }

    emsgf(_("E808: Number or Float required"));
    return false;
}

static inline dict_watcher_st *tv_dict_watcher_node_data(queue_st *q)
REAL_FATTR_NONNULL_ALL
REAL_FATTR_NONNULL_RET
REAL_FATTR_PURE
REAL_FATTR_WARN_UNUSED_RESULT
REAL_FATTR_ALWAYS_INLINE;

/// Compute the dict_watcher_st address from a queue_st node.
///
/// This only exists for .asan-blacklist
/// (ASAN doesn't handle QUEUE_DATA pointer arithmetic).
static inline dict_watcher_st *tv_dict_watcher_node_data(queue_st *q)
{
    return QUEUE_DATA(q, dict_watcher_st, node);
}

static inline bool tv_is_func(const typval_st tv)
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_ALWAYS_INLINE
FUNC_ATTR_CONST;

/// Check whether given typval_st contains a function
///
/// That is, whether it contains kNvarUfunc or kNvarPartial.
///
/// @param[in]  tv  Typval to check.
///
/// @return True if it is a function or a partial, false otherwise.
static inline bool tv_is_func(const typval_st tv)
{
    return tv.v_type == kNvarUfunc || tv.v_type == kNvarPartial;
}

/// Specify that argument needs to be translated
///
/// Used for size_t length arguments to avoid calling
/// gettext() and strlen() unless needed.
#define TV_TRANSLATE (SIZE_MAX)

/// Specify that argument is a NUL-terminated C string
///
/// Used for size_t length arguments to avoid calling strlen() unless needed.
#define TV_CSTRING    (SIZE_MAX - 1)

#ifdef UNIT_TESTING
    // Do not use enum constants, see commit message.
    EXTERN const size_t kTVCstring INIT(= TV_CSTRING);
    EXTERN const size_t kTVTranslate INIT(= TV_TRANSLATE);
#endif

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "eval/typval.h.generated.h"
#endif

#endif // NVIM_EVAL_TYPVAL_H
