/// @file nvim/popupmnu.h

#ifndef NVIM_POPUPMNU_H
#define NVIM_POPUPMNU_H

#include "nvim/types.h"

/// Used for popup menu items.
typedef struct
{
    uchar_kt *pum_text;   ///< main menu text
    uchar_kt *pum_kind;   ///< extra kind text (may be truncated)
    uchar_kt *pum_extra;  ///< extra menu text (may be truncated)
    uchar_kt *pum_info;   ///< extra info
} pumitem_T;


#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "popupmnu.h.generated.h"
#endif

#endif // NVIM_POPUPMNU_H
