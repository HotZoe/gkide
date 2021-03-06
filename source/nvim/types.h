/// @file nvim/types.h

#ifndef NVIM_TYPES_H
#define NVIM_TYPES_H

#include <stdint.h>

/// dummy to pass an ACL to a function
typedef void *  nvim_acl_kt;

/// Can hold one decoded UTF-8 character.
typedef uint32_t utf8char_kt;

/// Shorthand for unsigned variables.
typedef unsigned char uchar_kt;

typedef struct expand_s expand_st;

typedef struct caller_scope_s caller_scope_st;

#endif // NVIM_TYPES_H
