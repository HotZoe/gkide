/// @file nvim/api/private/defs.h

#ifndef NVIM_API_PRIVATE_DEFS_H
#define NVIM_API_PRIVATE_DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nvim/func_attr.h"

/// Mask for all internal calls
#define INTERNAL_CALL_MASK    (((uint64_t)1) << (sizeof(uint64_t) * 8 - 1))

/// Used as the message ID of notifications.
#define NO_RESPONSE           UINT64_MAX

/// Internal call from VimL code
#define VIML_INTERNAL_CALL    INTERNAL_CALL_MASK

/// Internal call from lua code
#define LUA_INTERNAL_CALL     (VIML_INTERNAL_CALL + 1)

/// Maximum value of an Integer
#define API_INTEGER_MAX       INT64_MAX

/// Minimum value of an Integer
#define API_INTEGER_MIN       INT64_MIN

#define ERROR_INIT          { .type = kErrorTypeNone, .msg = NULL }
#define STRING_INIT         { .data = NULL, .size = 0 }
#define OBJECT_INIT         { .type = kObjectTypeNil }
#define ARRAY_DICT_INIT     { .size = 0, .capacity = 0, .items = NULL }
#define NIL                 ((Object) { .type = kObjectTypeNil })
#define ERROR_SET(e)        ((e)->type != kErrorTypeNone)

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #define ArrayOf(...)      Array
    #define DictionaryOf(...) Dictionary
#endif

typedef int                  handle_kt;

typedef struct object_s      Object;    ///< Remote API object type
typedef bool                 Boolean;   ///< Remote API object type
typedef int64_t              Integer;   ///< Remote API object type
typedef double               Float;     ///< Remote API object type
typedef struct string_s      String;    ///< Remote API object type
typedef struct array_s       Array;     ///< Remote API object type
typedef struct dictionary_s  Dictionary;///< Remote API object type
typedef handle_kt            Buffer;    ///< Remote API object type
typedef handle_kt            Window;    ///< Remote API object type
typedef handle_kt            Tabpage;   ///< Remote API object type

typedef enum object_type_e
{
    kObjectTypeNil = 0,
    kObjectTypeBoolean,
    kObjectTypeInteger,
    kObjectTypeFloat,
    kObjectTypeString,
    kObjectTypeArray,
    kObjectTypeDictionary,

    // EXT types, cannot be split or reordered,
    // see #EXT_OBJECT_TYPE_SHIFT
    kObjectTypeBuffer,
    kObjectTypeWindow,
    kObjectTypeTabpage,
} object_type_et;

struct string_s
{
    char *data;
    size_t size;
};

struct array_s
{
    Object *items;
    size_t size;
    size_t capacity;
};

typedef struct key_value_pair_s key_value_pair_st;

struct dictionary_s
{
    key_value_pair_st *items;
    size_t size;
    size_t capacity;
};

struct object_s
{
    object_type_et type;
    union
    {
        Boolean boolean;
        Integer integer;
        Float floating;
        String string;
        Array array;
        Dictionary dictionary;
    } data;
};

struct key_value_pair_s
{
    String key;
    Object value;
};

// about error and message
typedef enum
{
    kErrorTypeNone = -1,
    kErrorTypeException,
    kErrorTypeValidation
} error_type_et;

typedef enum
{
    kMessageTypeRequest,
    kMessageTypeResponse,
    kMessageTypeNotification
} message_type_et;

typedef struct error_s
{
    error_type_et type;
    char *msg;
} error_st;

static inline bool is_internal_call(uint64_t channel_id)
REAL_FATTR_ALWAYS_INLINE
REAL_FATTR_CONST;

/// Check whether call is internal
///
/// @param[in]  channel_id  Channel id.
///
/// @return true if channel_id refers to internal channel.
static inline bool is_internal_call(const uint64_t channel_id)
{
    return !!(channel_id & INTERNAL_CALL_MASK);
}

#endif // NVIM_API_PRIVATE_DEFS_H
