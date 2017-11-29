/// @file nvim/version.h

#ifndef NVIM_VERSION_H
#define NVIM_VERSION_H

#include "versiondef.h"
#include "nvim/ex_cmds_defs.h"

// defined in version.c
extern char *nvim_gkide_version;

/// swap file version, for compatibility, also the base version of vim.
///
/// the max length is 6 chars, not including NUL
#define VIM_SWAP_VERSION    "7.4"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "version.h.generated.h"
#endif

#endif // NVIM_VERSION_H
