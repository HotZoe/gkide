/// @file nvim/map.h

#ifndef NVIM_MAP_H
#define NVIM_MAP_H

#include <stdbool.h>

#include "nvim/map_defs.h"
#include "nvim/api/private/defs.h"
#include "nvim/api/private/dispatch.h"
#include "nvim/bufhl_defs.h"

// key(T), value(U)
#define MAP_DECLS(T, U)                                        \
    KHASH_DECLARE(T##_##U##_map, T, U)                         \
                                                               \
    typedef struct                                             \
    {                                                          \
        khash_t(T##_##U##_map) *table;                         \
    } Map(T, U);

MAP_DECLS(int,        int)
MAP_DECLS(cstr_kt,    ptr_kt)
MAP_DECLS(ptr_kt,     ptr_kt)
MAP_DECLS(uint64_t,   ptr_kt)
MAP_DECLS(handle_kt,  ptr_kt)
MAP_DECLS(linenum_kt, bufhl_vec_st)
MAP_DECLS(String,     rpc_request_handler_st)

#define map_new(T, U)   map_##T##_##U##_new
#define map_free(T, U)  map_##T##_##U##_free
#define map_get(T, U)   map_##T##_##U##_get
#define map_has(T, U)   map_##T##_##U##_has
#define map_put(T, U)   map_##T##_##U##_put
#define map_ref(T, U)   map_##T##_##U##_ref
#define map_del(T, U)   map_##T##_##U##_del
#define map_clear(T, U) map_##T##_##U##_clear

#define pmap_new(T)   map_new(T, ptr_kt)
#define pmap_free(T)  map_free(T, ptr_kt)
#define pmap_get(T)   map_get(T, ptr_kt)
#define pmap_has(T)   map_has(T, ptr_kt)
#define pmap_put(T)   map_put(T, ptr_kt)
#define pmap_del(T)   map_del(T, ptr_kt)
#define pmap_clear(T) map_clear(T, ptr_kt)

#define map_foreach(map, key, value, block) \
    kh_foreach(map->table, key, value, block)

#define map_foreach_value(map, value, block) \
    kh_foreach_value(map->table, value, block)

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "map.h.generated.h"
#endif

#endif // NVIM_MAP_H
