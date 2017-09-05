/// @file nvim/map_defs.h

#ifndef NVIM_MAP_DEFS_H
#define NVIM_MAP_DEFS_H

#include "nvim/lib/khash.h"

typedef void *ptr_t;
typedef const char *cstr_t;

#define Map(T, U)  Map_##T##_##U
#define PMap(T)    Map(T, ptr_t)

#endif // NVIM_MAP_DEFS_H
