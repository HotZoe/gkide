/// @file nvim/version.h

#ifndef NVIM_VERSION_H
#define NVIM_VERSION_H

#include "generated/config/gkideversion.h"
#include "nvim/ex_cmds_defs.h"

// defined in version.c
extern char *nvim_version_long;

/// swap file version, for compatibility, also the base version of vim.
///
/// the max length is 6 chars, not including NUL
#define VIM_SWAP_VERSION    "7.4"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "version.h.generated.h"
#endif

#endif // NVIM_VERSION_H
