/// @file nvim/lib/queue.h
///
/// Generic Queue library.
///
/// Queue implemented by circularly-linked list.
///
/// Adapted from libuv. Simpler and more efficient
/// than klist.h for implementing queues that support
/// arbitrary insertion/removal.

#ifndef NVIM_LIB_QUEUE_H
#define NVIM_LIB_QUEUE_H

#include <stddef.h>

#include "nvim/func_attr.h"

typedef struct _queue
{
    struct _queue *next;
    struct _queue *prev;
} QUEUE;

/// Public macros.
#define QUEUE_DATA(ptr, type, field) \
    ((type *)((char *)(ptr) - offsetof(type, field)))

/// @note:
/// mutating the list while QUEUE_FOREACH is iterating
/// over its elements results in undefined behavior.
#define QUEUE_FOREACH(q, h) \
    for((q) = (h)->next; (q) != (h); (q) = (q)->next)

/// ffi.cdef is unable to swallow @b bool in place of @b int here.
static inline int QUEUE_EMPTY(const QUEUE *const q)
FUNC_ATTR_ALWAYS_INLINE
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return q == q->next;
}

#define QUEUE_HEAD(q)   (q)->next

static inline void QUEUE_INIT(QUEUE *const q)
FUNC_ATTR_ALWAYS_INLINE
{
    q->next = q;
    q->prev = q;
}

static inline void QUEUE_ADD(QUEUE *const h, QUEUE *const n)
FUNC_ATTR_ALWAYS_INLINE
{
    h->prev->next = n->next;
    n->next->prev = h->prev;
    h->prev = n->prev;
    h->prev->next = h;
}

static inline void QUEUE_INSERT_HEAD(QUEUE *const h, QUEUE *const q)
FUNC_ATTR_ALWAYS_INLINE
{
    q->next = h->next;
    q->prev = h;
    q->next->prev = q;
    h->next = q;
}

static inline void QUEUE_INSERT_TAIL(QUEUE *const h, QUEUE *const q)
FUNC_ATTR_ALWAYS_INLINE
{
    q->next = h;
    q->prev = h->prev;
    q->prev->next = q;
    h->prev = q;
}

static inline void QUEUE_REMOVE(QUEUE *const q)
FUNC_ATTR_ALWAYS_INLINE
{
    q->prev->next = q->next;
    q->next->prev = q->prev;
}

#endif // NVIM_LIB_QUEUE_H
