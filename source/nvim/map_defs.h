/// @file nvim/map_defs.h

#ifndef NVIM_MAP_DEFS_H
#define NVIM_MAP_DEFS_H

#include "nvim/lib/khash.h"

typedef void *ptr_kt;
typedef const char *cstr_kt;

#define Map(T, U)  Map_##T##_##U
#define PMap(T)    Map(T, ptr_kt)

#endif // NVIM_MAP_DEFS_H
