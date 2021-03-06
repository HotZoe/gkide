/// @file nvim/eval/encode.h

#ifndef NVIM_EVAL_ENCODE_H
#define NVIM_EVAL_ENCODE_H

#include <stddef.h>

#include <msgpack.h>

#include "nvim/eval.h"
#include "nvim/garray.h"
#include "nvim/nvim.h" // For ustrlen

/// Convert VimL value to msgpack string
///
/// @param[out] packer   Packer to save results in.
/// @param[in]  tv       Dumped value.
/// @param[in]  objname  Object name, used for error message.
///
/// @return OK in case of success, FAIL otherwise.
int encode_vim_to_msgpack(msgpack_packer *const packer,
                          typval_st *const tv,
                          const char *const objname);

/// Convert VimL value to :echo output
///
/// @param[out] packer   Packer to save results in.
/// @param[in]  tv       Dumped value.
/// @param[in]  objname  Object name, used for error message.
///
/// @return OK in case of success, FAIL otherwise.
int encode_vim_to_echo(garray_st *const packer,
                       typval_st *const tv,
                       const char *const objname);

/// Structure defining state for read_from_list()
typedef struct list_state_s
{
    const listitem_st *li; ///< Item currently read.
    size_t offset;         ///< Byte offset inside the read item.
    size_t li_length;      ///< Length of the string inside the read item.
} list_state_st;

/// Initialize list_state_st structure
static inline list_state_st encode_init_lrstate(const list_st *const list)
FUNC_ATTR_NONNULL_ALL
{
    return (list_state_st) {
        .li = list->lv_first,
        .offset = 0,
        .li_length = (list->lv_first->li_tv.vval.v_string == NULL
                      ? 0 : ustrlen(list->lv_first->li_tv.vval.v_string)),
    };
}

/// Array mapping values from nvlvar_special_value_et enum to names
extern const char *const encode_special_var_names[];

/// First codepoint in high surrogates block
#define SURROGATE_HI_START   0xD800

/// Last codepoint in high surrogates block
#define SURROGATE_HI_END     0xDBFF

/// First codepoint in low surrogates block
#define SURROGATE_LO_START   0xDC00

/// Last codepoint in low surrogates block
#define SURROGATE_LO_END     0xDFFF

/// First character that needs to be encoded as surrogate pair
#define SURROGATE_FIRST_CHAR 0x10000

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "eval/encode.h.generated.h"
#endif

#endif // NVIM_EVAL_ENCODE_H
