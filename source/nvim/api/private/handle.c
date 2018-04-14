/// @file nvim/api/private/handle.c

#include <assert.h>
#include <stdint.h>

#include "nvim/nvim.h"
#include "nvim/map.h"
#include "nvim/api/private/handle.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "api/private/handle.c.generated.h"
#endif

static PMap(handle_kt) *window_handles = NULL;
static PMap(handle_kt) *buffer_handles = NULL;
static PMap(handle_kt) *tabpage_handles = NULL;

win_st *handle_get_window(handle_kt handle)
{
    return pmap_get(handle_kt)(window_handles, handle);
}

void handle_register_window(win_st *window)
{
    pmap_put(handle_kt)(window_handles, window->handle, window);
}

void handle_unregister_window(win_st *window)
{
    pmap_del(handle_kt)(window_handles, window->handle);
}

filebuf_st *handle_get_buffer(handle_kt handle)
{
    return pmap_get(handle_kt)(buffer_handles, handle);
}

void handle_register_buffer(filebuf_st *buffer)
{
    pmap_put(handle_kt)(buffer_handles, buffer->b_id, buffer);
}

void handle_unregister_buffer(filebuf_st *buffer)
{
    pmap_del(handle_kt)(buffer_handles, buffer->b_id);
}

tabpage_st *handle_get_tabpage(handle_kt handle)
{
    return pmap_get(handle_kt)(tabpage_handles, handle);
}

void handle_register_tabpage(tabpage_st *tabpage)
{
    pmap_put(handle_kt)(tabpage_handles, tabpage->handle, tabpage);
}

void handle_unregister_tabpage(tabpage_st *tabpage)
{
    pmap_del(handle_kt)(tabpage_handles, tabpage->handle);
}

/// init three big thing: buffer, window, tabpage
void handle_init(void)
{
    buffer_handles = pmap_new(handle_kt)();
    window_handles = pmap_new(handle_kt)();
    tabpage_handles = pmap_new(handle_kt)();
}
