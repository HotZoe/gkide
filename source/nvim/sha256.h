/// @file nvim/sha256.h

#ifndef NVIM_SHA256_H
#define NVIM_SHA256_H

#include <stdint.h>
#include <stddef.h>

#include "nvim/types.h"

#define SHA256_SUM_SIZE     32
#define SHA256_BUFFER_SIZE  64

typedef struct sha256_ctx_s
{
    uint32_t total[2];
    uint32_t state[8];
    uchar_kt buffer[SHA256_BUFFER_SIZE];
} sha256_ctx_st;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "sha256.h.generated.h"
#endif

#endif // NVIM_SHA256_H
