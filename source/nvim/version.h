/// @file nvim/version.h

#ifndef NVIM_VERSION_H
#define NVIM_VERSION_H

#include "nvim/ex_cmds_defs.h"

// defined in version.c
extern char *longVersion;

// Vim version number, name, etc. Patchlevel is defined in version.c.
#define VIM_VERSION_MAJOR    7
#define VIM_VERSION_MINOR    4
#define VIM_VERSION_100      (VIM_VERSION_MAJOR * 100 + VIM_VERSION_MINOR)

/// swap file version, for compatibility, also the base version of vim.
///
/// the max length is 6 chars, not including NUL
#define VIM_SWAP_VERSION     "7.4"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "version.h.generated.h"
#endif

#endif // NVIM_VERSION_H
