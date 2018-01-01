/// @file nvim/types.h

#ifndef NVIM_TYPES_H
#define NVIM_TYPES_H

#include <stdint.h>

/// dummy to pass an ACL to a function
typedef void *vim_acl_T;

/// Shorthand for unsigned variables.
typedef unsigned char uchar_kt;

/// Can hold one decoded UTF-8 character.
typedef uint32_t u8char_T;

typedef struct expand expand_T;

#endif // NVIM_TYPES_H
