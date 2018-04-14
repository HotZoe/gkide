/// @file nvim/error.h

#ifndef NVIM_ERROR_H
#define NVIM_ERROR_H

#define mch_errmsg(str)     fprintf(stderr, "%s", (str))

#define TO_FIX_THIS(msg)                                \
    do                                                  \
    {                                                   \
        fprintf(stderr, "[ToFixThis]-[%s, %d]: %s\n",   \
                __func__, __LINE__, msg);               \
        exit(1); /* fatal error, need to fix it ! */    \
    } while(0)

/// nvim exit status
typedef enum nvim_exit_status_e
{
    kNEStatusSuccess = 0, ///< nvim exit normally
    kNEStatusFailure = 1, ///< nvim exit with error

    kNEStatusHostMemoryNotEnough,
    kNEStatusFileTooBigToOpen,
    kNEStatusWinAllocateFailed,
    kNEStatusNoUserHome,
    kNEStatusNoRecoveryFile,
    kNEStatusQuickFixInitErr,
    kNEStatusOpenNvlScriptAgain,
    kNEStatusNvlScriptCanNotOpen,
    kNEStatusNvlScriptCanNotWrite,
    kNEStatusCommandLineArgsError,
    kNEStatusBufAllocateFailed,
    kNEStatusCscopeConnectionError,
    kNEStatusPreserveFilesExit,
    kNEStatusNvimServerInvalidPort,
    kNEStatusNvimServerInvalidAddr,

    kNEStatusInterpreterInitErrorLua,
} nvim_exit_status_et;


#endif // NVIM_ERROR_H
