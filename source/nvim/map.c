/// @file nvim/map.c

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "nvim/map.h"
#include "nvim/map_defs.h"
#include "nvim/vim.h"
#include "nvim/memory.h"
#include "nvim/api/private/dispatch.h"

#include "nvim/lib/khash.h"

#define cstr_t_hash     kh_str_hash_func
#define cstr_t_eq       kh_str_hash_equal
#define uint64_t_hash   kh_int64_hash_func
#define uint64_t_eq     kh_int64_hash_equal
#define uint32_t_hash   kh_int_hash_func
#define uint32_t_eq     kh_int_hash_equal
#define int_hash        kh_int_hash_func
#define int_eq          kh_int_hash_equal
#define linenum_kt_hash kh_int_hash_func
#define linenum_kt_eq   kh_int_hash_equal
#define handle_kt_hash  kh_int_hash_func
#define handle_kt_eq    kh_int_hash_equal

#if defined(HOST_OS_ARCH_64)
    #define ptr_kt_hash(key)     uint64_t_hash((uint64_t)key)
    #define ptr_kt_eq(a, b)      uint64_t_eq((uint64_t)a, (uint64_t)b)
#elif defined(HOST_OS_ARCH_32)
    #define ptr_kt_hash(key)     uint32_t_hash((uint32_t)key)
    #define ptr_kt_eq(a, b)      uint32_t_eq((uint32_t)a, (uint32_t)b)
#endif

#define INITIALIZER(T, U)               T##_##U##_initializer
#define INITIALIZER_DECLARE(T, U, ...)  const U INITIALIZER(T, U) = __VA_ARGS__
#define DEFAULT_INITIALIZER             { 0 }

#define MAP_IMPL(T, U, ...)                                                    \
    INITIALIZER_DECLARE(T, U, __VA_ARGS__);                                    \
    __KHASH_IMPL(T##_##U##_map, /* skip */, T, U, 1, T##_hash, T##_eq)         \
                                                                               \
    Map(T, U) *map_##T##_##U##_new(void)                                       \
    {                                                                          \
        Map(T, U) *rv = xmalloc(sizeof(Map(T, U)));                            \
        rv->table = kh_init(T##_##U##_map);                                    \
        return rv;                                                             \
    }                                                                          \
                                                                               \
    void map_##T##_##U##_free(Map(T, U) *map)                                  \
    {                                                                          \
        kh_destroy(T##_##U##_map, map->table);                                 \
        xfree(map);                                                            \
    }                                                                          \
                                                                               \
    U map_##T##_##U##_get(Map(T, U) *map, T key)                               \
    {                                                                          \
        khiter_t k;                                                            \
                                                                               \
        if((k = kh_get(T##_##U##_map, map->table, key)) == kh_end(map->table)) \
        {                                                                      \
            return INITIALIZER(T, U);                                          \
        }                                                                      \
                                                                               \
        return kh_val(map->table, k);                                          \
    }                                                                          \
                                                                               \
    bool map_##T##_##U##_has(Map(T, U) *map, T key)                            \
    {                                                                          \
        return kh_get(T##_##U##_map, map->table, key) != kh_end(map->table);   \
    }                                                                          \
                                                                               \
    U map_##T##_##U##_put(Map(T, U) *map, T key, U value)                      \
    {                                                                          \
        int ret;                                                               \
        U rv = INITIALIZER(T, U);                                              \
        khiter_t k = kh_put(T##_##U##_map, map->table, key, &ret);             \
                                                                               \
        if(!ret)                                                               \
        {                                                                      \
            rv = kh_val(map->table, k);                                        \
        }                                                                      \
                                                                               \
        kh_val(map->table, k) = value;                                         \
        return rv;                                                             \
    }                                                                          \
                                                                               \
    U *map_##T##_##U##_ref(Map(T, U) *map, T key, bool put)                    \
    {                                                                          \
        int ret;                                                               \
        khiter_t k;                                                            \
        if(put)                                                                \
        {                                                                      \
            k = kh_put(T##_##U##_map, map->table, key, &ret);                  \
            if(ret)                                                            \
            {                                                                  \
                kh_val(map->table, k) = INITIALIZER(T, U);                     \
            }                                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            k = kh_get(T##_##U##_map, map->table, key);                        \
            if(k == kh_end(map->table))                                        \
            {                                                                  \
                return NULL;                                                   \
            }                                                                  \
        }                                                                      \
                                                                               \
        return &kh_val(map->table, k);                                         \
    }                                                                          \
                                                                               \
    U map_##T##_##U##_del(Map(T, U) *map, T key)                               \
    {                                                                          \
        U rv = INITIALIZER(T, U);                                              \
        khiter_t k;                                                            \
                                                                               \
        if((k = kh_get(T##_##U##_map, map->table, key)) != kh_end(map->table)) \
        {                                                                      \
            rv = kh_val(map->table, k);                                        \
            kh_del(T##_##U##_map, map->table, k);                              \
        }                                                                      \
                                                                               \
        return rv;                                                             \
    }                                                                          \
                                                                               \
    void map_##T##_##U##_clear(Map(T, U) *map)                                 \
    {                                                                          \
        kh_clear(T##_##U##_map, map->table);                                   \
    }

static inline khint_t String_hash(String s)
{
    khint_t h = 0;

    for(size_t i = 0; i < s.size && s.data[i]; i++)
    {
        h = (h << 5) - h + (uint8_t)s.data[i];
    }

    return h;
}

static inline bool String_eq(String a, String b)
{
    if(a.size != b.size)
    {
        return false;
    }

    return memcmp(a.data, b.data, a.size) == 0;
}


MAP_IMPL(int,      int,   DEFAULT_INITIALIZER)
MAP_IMPL(cstr_t,   ptr_kt, DEFAULT_INITIALIZER)
MAP_IMPL(ptr_kt,    ptr_kt, DEFAULT_INITIALIZER)
MAP_IMPL(uint64_t, ptr_kt, DEFAULT_INITIALIZER)
MAP_IMPL(handle_kt, ptr_kt, DEFAULT_INITIALIZER)

#define BUFFER_HL_VEC_INITIALIZER    { .size = 0,  .capacity = 0, .items = NULL }
#define MSGPACK_HANDLER_INITIALIZER  { .fn = NULL, .async = false }

MAP_IMPL(linenum_kt, bufhl_vec_st,            BUFFER_HL_VEC_INITIALIZER)
MAP_IMPL(String,   rpc_request_handler_st, MSGPACK_HANDLER_INITIALIZER)
