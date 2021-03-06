/// @file nvim/if_cscope_defs.h
///
/// CSCOPE support for Vim added by Andy Kahn <kahn@zk3.dec.com>
/// Ported to Win32 by Sergey Khorev <sergey.khorev@gmail.com>
///
/// The basic idea/structure of cscope for Vim was borrowed from Nvi.
/// There might be a few lines of code that look similar to what Nvi
/// has. If this is a problem and requires inclusion of the annoying
/// BSD license, then sue me; I'm not worth much anyway.

#ifndef NVIM_IF_CSCOPE_DEFS_H
#define NVIM_IF_CSCOPE_DEFS_H

#if defined(UNIX)
    #include <sys/types.h> // pid_t
#endif

#include "nvim/os/os_defs.h"
#include "nvim/os/fs_defs.h"
#include "nvim/ex_cmds_defs.h"

#define CSCOPE_SUCCESS      0
#define CSCOPE_FAILURE      -1

#define CSCOPE_DBFILE       "cscope.out"
#define CSCOPE_PROMPT       ">> "

/// See ":help cscope-find" for the possible queries.
typedef struct cscmd_s
{
    char *name;
    int (*func)(exargs_st *eap);
    char *help;
    char *usage;
    /// if supports splitting window
    int cansplit;
} cscmd_st;

typedef struct csinfo_s
{
    char *fname;  ///< cscope db name
    char *ppath;  ///< path to prepend (the -P option)
    char *flags;  ///< additional cscope flags/options (e.g, -p2)

#if defined(UNIX)
    pid_t pid;    ///< PID of the connected cscope process
#else
    DWORD   pid;         ///< PID of the connected cscope process
    HANDLE  hProc;       ///< cscope process handle
    DWORD   nVolume;     ///< Volume serial number, instead of st_dev
    DWORD   nIndexHigh;  ///< st_ino has no meaning on Windows
    DWORD   nIndexLow;
#endif

    fileid_st file_id;

    FILE *fr_fp;  ///< from cscope: FILE.
    FILE *to_fp;  ///< to cscope: FILE.
} csinfo_st;

typedef enum csid_e
{
    Add,
    Find,
    Help,
    Kill,
    Reset,
    Show
} csid_et;

typedef enum mcmd_e
{
    Store,
    Get,
    Free,
    Print
} mcmd_et;

#endif // NVIM_IF_CSCOPE_DEFS_H
