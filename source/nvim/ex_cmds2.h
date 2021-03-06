/// @file nvim/ex_cmds2.h

#ifndef NVIM_EX_CMDS2_H
#define NVIM_EX_CMDS2_H

#include <stdbool.h>

#include "nvim/ex_docmd.h"

typedef void (*DoInRuntimepathCB)(uchar_kt *, void *);

// flags for check_changed()
#define CCGD_AW         1    ///< do autowrite if buffer was changed
#define CCGD_MULTWIN    2    ///< check also when several wins for the buf
#define CCGD_FORCEIT    4    ///< ! used
#define CCGD_ALLBUF     8    ///< may write all buffers
#define CCGD_EXCMD      16   ///< may suggest using

/// The nvim source file type: @b nvimrc type and @b cmdrc type
/// last argument for do_source()
enum SourceFileType_T
{
    kLoadSftAuto   = 0,  ///< loading defaut auto type

    // Source File Type
    kLoadSftNvimrc = 1,  ///< loading nvimrc file type
    kLoadSftCmdrc  = 2,  ///< loading cmdrc  file type

    // Source File Scope
    kLoadSfsSys    = 4,  ///< loading system  scope
    kLoadSfsUsr    = 8,  ///< loading user    scope
    kLoadSfsDyn    = 16, ///< loading project scope
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "ex_cmds2.h.generated.h"
#endif

#endif // NVIM_EX_CMDS2_H
