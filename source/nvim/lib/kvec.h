/// @file nvim/lib/kvec.h
///
/// Generic vector library.

/*
An example:
-----------------------------------------------------------
#include "kvec.h"

int main()
{
    kvec_t(int) array = KV_INITIAL_VALUE;
    kv_push(array, 10); // append
    kv_a(array, 20) = 5; // dynamic
    kv_A(array, 20) = 4; // static
    kv_destroy(array);
    return 0;
}
-----------------------------------------------------------
*/

#ifndef NVIM_LIB_KVEC_H
#define NVIM_LIB_KVEC_H

#include <stdlib.h>
#include <string.h>

#include "nvim/memory.h"

#define kv_roundup32(x) \
    ((--(x)),           \
     ((x)|=(x)>>1,      \
      (x)|=(x)>>2,      \
      (x)|=(x)>>4,      \
      (x)|=(x)>>8,      \
      (x)|=(x)>>16),    \
     (++(x)))

#define KV_INITIAL_VALUE  { .size = 0, .capacity = 0, .items = NULL }

#define kvec_t(type)     \
    struct               \
    {                    \
        size_t size;     \
        size_t capacity; \
        type *items;     \
    }

#define kv_init(v)    ((v).size = (v).capacity = 0, (v).items = 0)
#define kv_A(v, i)    ((v).items[(i)])
#define kv_pop(v)     ((v).items[--(v).size])
#define kv_size(v)    ((v).size)
#define kv_max(v)     ((v).capacity)
#define kv_destroy(v) xfree((v).items)
#define kv_last(v)    kv_A(v, kv_size(v) - 1)

#define kv_resize(v, s)                     \
    ((v).capacity = (s),                    \
     (v).items = xrealloc((v).items,        \
     sizeof((v).items[0]) * (v).capacity))

#define kv_resize_full(v) \
    kv_resize(v, (v).capacity ? (v).capacity << 1 : 8)

#define kv_copy(v1, v0)                             \
    do                                              \
    {                                               \
        if((v1).capacity < (v0).size)               \
        {                                           \
            kv_resize(v1, (v0).size);               \
        }                                           \
                                                    \
        (v1).size = (v0).size;                      \
                                                    \
        memcpy((v1).items,                          \
               (v0).items,                          \
               sizeof((v1).items[0]) * (v0).size);  \
    } while(0)

#define kv_pushp(v) \
    ((((v).size == (v).capacity) ? (kv_resize_full(v), 0) : 0), \
     ((v).items + ((v).size++)))

#define kv_push(v, x)  (*kv_pushp(v) = (x))

#define kv_a(v, i)                          \
    (((v).capacity <= (size_t) (i)          \
      ? ((v).capacity = (v).size = (i) + 1, \
         kv_roundup32((v).capacity),        \
         kv_resize((v), (v).capacity), 0)   \
      : ((v).size <= (size_t) (i)           \
         ? ((v).size = (i) + 1) : 0)),      \
     (v).items[(i)])

/// Type of a vector with a few first members allocated on stack
///
/// Is compatible with #kv_A, #kv_pop, #kv_size, #kv_max, #kv_last.
/// Is not compatible with #kv_resize, #kv_resize_full, #kv_copy, #kv_push,
/// #kv_pushp, #kv_a, #kv_destroy.
///
/// @param[in]  type       Type of vector elements.
/// @param[in]  init_size  Number of the elements in the initial array.
#define kvec_withinit_t(type, INIT_SIZE) \
    struct                               \
    {                                    \
        size_t size;                     \
        size_t capacity;                 \
        type *items;                     \
        type init_array[INIT_SIZE];      \
    }

/// Initialize vector with preallocated array
///
/// @param[out]  v  Vector to initialize.
#define kvi_init(v)                             \
    ((v).capacity = ARRAY_SIZE((v).init_array), \
     (v).size = 0,                              \
     (v).items = (v).init_array)

/// Move data to a new destination and free source
static inline void *_memcpy_free(void *const restrict dest,
                                 void *const restrict src,
                                 const size_t size)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_ALWAYS_INLINE
{
    memcpy(dest, src, size);
    xfree(src);
    return dest;
}

/// Resize vector with preallocated array
///
/// @note
/// May not resize to an array smaller then init_array:
/// if requested, init_array will be used.
///
/// @param[out] v  Vector to resize.
/// @param[in]  s  New size.
#define kvi_resize(v, s)                                                    \
    ((v).capacity = ((s) > ARRAY_SIZE((v).init_array)                       \
                     ? (s)                                                  \
                     : ARRAY_SIZE((v).init_array)),                         \
     (v).items = ((v).capacity == ARRAY_SIZE((v).init_array)                \
                  ? ((v).items == (v).init_array                            \
                     ? (v).items                                            \
                     : _memcpy_free((v).init_array, (v).items,              \
                                    (v).size * sizeof((v).items[0])))       \
                  : ((v).items == (v).init_array                            \
                     ? memcpy(xmalloc((v).capacity * sizeof((v).items[0])), \
                              (v).items,                                    \
                              (v).size * sizeof((v).items[0]))              \
                     : xrealloc((v).items,                                  \
                                (v).capacity * sizeof((v).items[0])))))

/// Resize vector with preallocated array when it is full
///
/// @param[out]  v  Vector to resize.
///
/// @note for kvi_resize()
/// ARRAY_SIZE((v).init_array) is the minimal capacity of this vector
/// Thus when vector is full capacity may not be zero and it is safe
/// not to bother with checking whether (v).capacity is 0. But now
/// capacity is not guaranteed to have size that is a power of 2, it is
/// hard to fix this here and is not very necessary if users will use
/// 2^x initial array size.
#define kvi_resize_full(v)  kvi_resize(v, (v).capacity << 1)

/// Get location where to store new
/// element to a vector with preallocated array
///
/// @param[in,out]  v  Vector to push to.
///
/// @return
/// Pointer to the place where new value should be stored.
#define kvi_pushp(v) \
    ((((v).size == (v).capacity) ? (kvi_resize_full(v), 0) : 0), \
     ((v).items + ((v).size++)))

/// Push value to a vector with preallocated array
///
/// @param[out] v  Vector to push to.
/// @param[in]  x  Value to push.
#define kvi_push(v, x)  (*kvi_pushp(v) = (x))

/// Free array of elements of a vector
/// with preallocated array if needed
///
/// @param[out]  v  Vector to free.
#define kvi_destroy(v) \
    do                                  \
    {                                   \
        if((v).items != (v).init_array) \
        {                               \
            xfree((v).items);           \
        }                               \
    } while(0)

#endif // NVIM_LIB_KVEC_H
