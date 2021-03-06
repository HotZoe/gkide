/// @file nvim/indent.h

#ifndef NVIM_INDENT_H
#define NVIM_INDENT_H

#include "nvim/nvim.h"

// flags for set_indent()
#define SIN_CHANGED     1   ///< call changed_bytes() when line changed
#define SIN_INSERT      2   ///< insert indent before existing text
#define SIN_UNDO        4   ///< save line for undo before changing it

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "indent.h.generated.h"
#endif

#endif // NVIM_INDENT_H
