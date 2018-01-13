/// @file nvim/os/fs_defs.h

#ifndef NVIM_OS_FS_DEFS_H
#define NVIM_OS_FS_DEFS_H

#include <uv.h>

/// Struct which encapsulates stat information.
typedef struct fileinfo_s
{
    uv_stat_t stat;  ///<
} fileinfo_st;

/// Struct which encapsulates inode/dev_id information.
typedef struct fileid_s
{
    uint64_t inode;     ///< The inode of the file
    uint64_t device_id; ///< The id of the device containing the file
} fileid_st;

#define FILE_ID_EMPTY   (fileid_st){ .inode = 0, .device_id = 0 }

typedef struct directory_s
{
    uv_fs_t request;  ///< The request to uv for the directory.
    uv_dirent_t ent;  ///< The entry information.
} directory_st;

enum
{
    /// Many fs functions from libuv return this value on success.
    kLibuvSuccess = 0,
};

/// Function to convert libuv error to char * error description
///
/// negative libuv error codes are returned by a number of os functions.
#define os_strerror uv_strerror

// Values returned by os_nodetype()
//
/// file or directory, check with os_isdir()
#define NODE_NORMAL     0

/// something we can write to (character device, fifo, socket, ..)
#define NODE_WRITABLE   1

/// non-writable thing (e.g., block device)
#define NODE_OTHER      2

#endif // NVIM_OS_FS_DEFS_H
