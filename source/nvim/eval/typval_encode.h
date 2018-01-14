/// @file nvim/eval/typval_encode.h
///
/// Contains common definitions for eval/typval_encode.c.h.
/// Most of time should not be included directly.

#ifndef NVIM_EVAL_TYPVAL_ENCODE_H
#define NVIM_EVAL_TYPVAL_ENCODE_H

#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "nvim/lib/kvec.h"
#include "nvim/eval/typval.h"
#include "nvim/func_attr.h"

/// Type of the stack entry
typedef enum
{
    kMPConvDict,         ///< Convert dict_st *dictionary.
    kMPConvList,         ///< Convert list_st *list.
    kMPConvPairs,        ///< Convert mapping represented as a list_st* of pairs.
    kMPConvPartial,      ///< Convert partial_st* partial.
    kMPConvPartialList,  ///< Convert argc/argv pair coming from a partial.
} MPConvStackValType;

/// Stage at which partial is being converted
typedef enum
{
    kMPConvPartialArgs,  ///< About to convert arguments.
    kMPConvPartialSelf,  ///< About to convert self dictionary.
    kMPConvPartialEnd,   ///< Already converted everything.
} MPConvPartialStage;

/// Structure representing current VimL to messagepack conversion state
typedef struct mpconv_stack_s
{
    MPConvStackValType type;  ///< Type of the stack entry.
    typval_st *tv;             ///< Currently converted typval_st.
    int saved_copyID;         ///< copyID item used to have.
    union
    {
        struct
        {
            dict_st *dict;     ///< Currently converted dictionary.
            dict_st **dictp;   ///< Location where that dictionary is stored.
                              ///< Normally it is &.tv->vval.v_dict, but not when
                              ///< converting partials.
            hashitem_st *hi;   ///< Currently converted dictionary item.
            size_t todo;      ///< Amount of items left to process.
        } d;                  ///< State of dictionary conversion.

        struct
        {
            list_st *list;     ///< Currently converted list.
            listitem_st *li;   ///< Currently converted list item.
        } l;                  ///< State of list or generic mapping conversion.

        struct
        {
            MPConvPartialStage stage;  ///< Stage at which partial is being converted.
            partial_st *pt;             ///< Currently converted partial.
        } p;                           ///< State of partial conversion.

        struct
        {
            typval_st *arg;    ///< Currently converted argument.
            typval_st *argv;   ///< Start of the argument list.
            size_t todo;      ///< Number of items left to process.
        } a;                  ///< State of list or generic mapping conversion.
    } data;                   ///< Data to convert.
} mpconv_stack_st;

/// Stack used to convert VimL values to messagepack.
typedef kvec_withinit_t(mpconv_stack_st, 8) MPConvStack;

// Defines for MPConvStack
#define _mp_size    kv_size
#define _mp_init    kvi_init
#define _mp_destroy kvi_destroy
#define _mp_push    kvi_push
#define _mp_pop     kv_pop
#define _mp_last    kv_last

static inline size_t tv_strlen(const typval_st *const tv)
REAL_FATTR_ALWAYS_INLINE
REAL_FATTR_PURE
REAL_FATTR_WARN_UNUSED_RESULT
REAL_FATTR_NONNULL_ALL;

/// Length of the string stored in typval_st
///
/// @param[in]  tv
/// String for which to compute length for. Must be typval_st with kNvarString.
///
/// @return
/// Length of the string stored in typval_st, including 0 for NULL string.
static inline size_t tv_strlen(const typval_st *const tv)
{
    assert(tv->v_type == kNvarString);
    return (tv->vval.v_string == NULL ? 0 : strlen((char *) tv->vval.v_string));
}

/// Code for checking whether container references itself
///
/// @param[in,out]  val  Container to check.
/// @param  copyID_attr  Name of the container attribute that holds copyID.
///                      After checking whether value of this attribute is
///                      copyID (variable) it is set to copyID.
/// @param[in]  copyID   CopyID used by the caller.
/// @param  conv_type    Type of the conversion, @see MPConvStackValType.
#define _TYPVAL_ENCODE_DO_CHECK_SELF_REFERENCE(val, copyID_attr, copyID, conv_type) \
    do                                                                              \
    {                                                                               \
        const int te_csr_ret = _TYPVAL_ENCODE_CHECK_SELF_REFERENCE(                 \
                                TYPVAL_ENCODE_FIRST_ARG_NAME,                       \
                                (val),                                              \
                                &(val)->copyID_attr,                                \
                                mpstack,                                            \
                                copyID,                                             \
                                conv_type,                                          \
                                objname);                                           \
        if(te_csr_ret != NOTDONE)                                                   \
        {                                                                           \
            return te_csr_ret;                                                      \
        }                                                                           \
    } while(0)

#define _TYPVAL_ENCODE_FUNC_NAME_INNER_2(pref, name, suf) \
    pref##name##suf

#define _TYPVAL_ENCODE_FUNC_NAME_INNER(pref, name, suf) \
    _TYPVAL_ENCODE_FUNC_NAME_INNER_2(pref, name, suf)

/// Construct function name, possibly using macros
///
/// Is used to expand macros that may appear in arguments.
///
/// @note Expands all arguments, even if only one is needed.
///
/// @param[in]  pref  Prefix.
/// @param[in]  suf   Suffix.
///
/// @return Concat: pref + #TYPVAL_ENCODE_NAME + suf.
#define _TYPVAL_ENCODE_FUNC_NAME(pref, suf) \
    _TYPVAL_ENCODE_FUNC_NAME_INNER(pref, TYPVAL_ENCODE_NAME, suf)

/// Self reference checker function name
#define _TYPVAL_ENCODE_CHECK_SELF_REFERENCE \
    _TYPVAL_ENCODE_FUNC_NAME(_typval_encode_, _check_self_reference)

/// Entry point function name
#define _TYPVAL_ENCODE_ENCODE \
    _TYPVAL_ENCODE_FUNC_NAME(encode_vim_to_, )

/// Name of the …convert_one_value function
#define _TYPVAL_ENCODE_CONVERT_ONE_VALUE \
    _TYPVAL_ENCODE_FUNC_NAME(_typval_encode_, _convert_one_value)

/// Name of the dummy const dict_st *const variable
#define TYPVAL_ENCODE_NODICT_VAR \
    _TYPVAL_ENCODE_FUNC_NAME(_typval_encode_, _nodict_var)

#endif // NVIM_EVAL_TYPVAL_ENCODE_H
