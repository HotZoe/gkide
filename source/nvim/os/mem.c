/// @file nvim/os/mem.c
///
/// Functions for accessing system memory information.

#include <uv.h>

#include "nvim/os/os.h"

/// Get the total system physical memory in KB.
uint64_t os_get_total_mem_kib(void)
{
    // Convert bytes to KB.
    return uv_get_total_memory() / 1024;
}
