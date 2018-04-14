/// @file nvim/api/private/dispatch.c

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <msgpack.h>

#include "nvim/map.h"
#include "nvim/log.h"
#include "nvim/nvim.h"
#include "nvim/msgpack/helpers.h"
#include "nvim/api/private/dispatch.h"
#include "nvim/api/private/helpers.h"
#include "nvim/api/private/defs.h"

#include "nvim/api/buffer.h"
#include "nvim/api/tabpage.h"
#include "nvim/api/ui.h"
#include "nvim/api/nvim.h"
#include "nvim/api/window.h"

static Map(String, rpc_request_handler_st) *methods = NULL;

static void rpc_add_method_handler(String method,
                                   rpc_request_handler_st handler)
{
    map_put(String, rpc_request_handler_st)(methods, method, handler);
}

rpc_request_handler_st rpc_get_handler_for(const char *name,
                                           size_t name_len)
{
    String m = { 0 };
    m.data = (char *)name;
    m.size = name_len;

    rpc_request_handler_st rv =
        map_get(String, rpc_request_handler_st)(methods, m);

    if(!rv.fn)
    {
        rv.fn = rpc_handle_missing_method;
    }

    return rv;
}

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "api/private/dispatch_wrappers.generated.h"
#endif
