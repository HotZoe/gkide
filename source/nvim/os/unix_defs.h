/// @file nvim/os/unix_defs.h

#ifndef NVIM_OS_UNIX_DEFS_H
#define NVIM_OS_UNIX_DEFS_H

// Windows doesn't have unistd.h, so we include it here to avoid
// numerous instances of '#ifdef HOST_OS_WINDOWS'.
#include <unistd.h>

// POSIX.1-2008 says that NAME_MAX should be in here
#include <limits.h>

#define TEMP_FILE_PATH_MAXLEN  256
#define TEMP_DIR_NAMES         { "$TMPDIR", "/tmp", ".", "~" }

/// Special wildcards that need to be handled by the shell.
#define SPECIAL_WILDCHAR  "`'{"

// Character that separates entries in $PATH.
#define ENV_SEPSTR   ":"
#define ENV_SEPCHAR  ':'

#endif // NVIM_OS_UNIX_DEFS_H
