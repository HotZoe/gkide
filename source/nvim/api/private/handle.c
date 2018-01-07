/// @file nvim/api/private/handle.c

#include <assert.h>
#include <stdint.h>

#include "nvim/vim.h"
#include "nvim/map.h"
#include "nvim/api/private/handle.h"

#define HANDLE_INIT(name) name##_handles = pmap_new(handle_kt)()

#define HANDLE_IMPL(type, name)                                  \
    static PMap(handle_kt) *name##_handles = NULL;               \
                                                                 \
    type *handle_get_##name(handle_kt handle)                    \
    {                                                            \
        return pmap_get(handle_kt)(name##_handles, handle);      \
    }                                                            \
                                                                 \
    void handle_register_##name(type *name)                      \
    {                                                            \
        pmap_put(handle_kt)(name##_handles, name->handle, name); \
    }                                                            \
                                                                 \
    void handle_unregister_##name(type *name)                    \
    {                                                            \
        pmap_del(handle_kt)(name##_handles, name->handle);       \
    }

HANDLE_IMPL(fbuf_st, buffer)
HANDLE_IMPL(win_st, window)
HANDLE_IMPL(tabpage_T, tabpage)

/// init three big thing: buffer, window, tabpage
void handle_init(void)
{
    HANDLE_INIT(buffer);
    HANDLE_INIT(window);
    HANDLE_INIT(tabpage);
}
