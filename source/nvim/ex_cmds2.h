/// @file nvim/ex_cmds2.h

#ifndef NVIM_EX_CMDS2_H
#define NVIM_EX_CMDS2_H

#include <stdbool.h>

#include "nvim/ex_docmd.h"

typedef void (*DoInRuntimepathCB)(char_u *, void *);

// flags for check_changed()
#define CCGD_AW         1       /// do autowrite if buffer was changed
#define CCGD_MULTWIN    2       /// check also when several wins for the buf
#define CCGD_FORCEIT    4       /// ! used
#define CCGD_ALLBUF     8       /// may write all buffers
#define CCGD_EXCMD      16      /// may suggest using

/// The nvim config, plugin and autocmds file types
enum SourceFileType_S
{
    kLoadNvimrcSys = 0,  ///< loading system nvimrc
    kLoadNvimrcUsr = 1,  ///< loading user nvimrc
    kLoadNvimrcDyn = 2,  ///< loading dynamic nvimrc, e.g.: for project

    kLoadCmdrcSys  = 3,  ///< loading system cmdrc
    kLoadCmdrcUsr  = 4,  ///< loading user cmdrc
    kLoadCmdrcDyn  = 5,  ///< loading dynamic cmdrc, e.g.: for project
};

/* last argument for do_source() */
#define DOSO_NONE       0
#define DOSO_VIMRC      1       /* loading vimrc file */
#define DOSO_GVIMRC     2       /* loading gvimrc file */

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "ex_cmds2.h.generated.h"
#endif

#endif // NVIM_EX_CMDS2_H
