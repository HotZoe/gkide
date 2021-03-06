/// @file nvim/memory.h

#ifndef NVIM_MEMORY_H
#define NVIM_MEMORY_H

#include <stdbool.h> // for bool
#include <stdint.h> // for uint8_t
#include <stddef.h> // for size_t
#include <time.h> // for time_t

#ifdef UNIT_TESTING
    /// malloc() function signature
    typedef void *(*mem_malloc_ft)(size_t);

    /// free() function signature
    typedef void (*mem_free_ft)(void *);

    /// calloc() function signature
    typedef void *(*mem_calloc_ft)(size_t, size_t);

    /// realloc() function signature
    typedef void *(*mem_realloc_ft)(void *, size_t);

    /// When unit testing: pointer to the malloc() function, may be altered
    extern mem_malloc_ft mem_malloc;

    /// When unit testing: pointer to the free() function, may be altered
    extern mem_free_ft mem_free;

    /// When unit testing: pointer to the calloc() function, may be altered
    extern mem_calloc_ft mem_calloc;

    /// When unit testing: pointer to the realloc() function, may be altered
    extern mem_realloc_ft mem_realloc;
#endif

#ifdef EXITFREE
    /// Indicates that free_all_mem function was or is running
    extern bool entered_free_all_mem;
#endif

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "memory.h.generated.h"
#endif

#endif // NVIM_MEMORY_H
