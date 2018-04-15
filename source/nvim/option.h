/// @file nvim/option.h

#ifndef NVIM_OPTION_H
#define NVIM_OPTION_H

#include "nvim/ex_cmds_defs.h"

// flags for buf_copy_options()
#define BCO_ENTER     1  ///< going to enter the buffer
#define BCO_ALWAYS    2  ///< always copy the options
#define BCO_NOHELP    4  ///< don't touch the help related options

/// Flags for option-setting functions
///
/// When kOptSetGlobal and kOptSetLocal are both missing,
/// set both local and global values, get local value.
typedef enum option_set_flags_e
{
    kOptSetDefault  = 0,
    kOptSetFree     = 1,  ///< Free old value if it was allocated
    kOptSetGlobal   = 2,  ///< Use global value
    kOptSetLocal    = 4,  ///< Use local value
    kOptSetModeline = 8,  ///< Option in modeline
    kOptSetWinOnly  = 16, ///< Only set window-local options
    kOptSetWinNone  = 32, ///< Don’t set window-local options
} option_set_flags_et;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "option.h.generated.h"
#endif

#endif // NVIM_OPTION_H
