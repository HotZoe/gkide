/// @file nvim/api/private/dispatch.h

#ifndef NVIM_API_PRIVATE_DISPATCH_H
#define NVIM_API_PRIVATE_DISPATCH_H

#include "nvim/api/private/defs.h"

/// API dispatch wrapper function type
typedef Object(*api_dispatch_ft)(uint64_t channel_id,
                                 Array args,
                                 error_st *error);

/// The rpc_method_handlers table, used in
/// rpc_dispatch(), stores functions of this type.
typedef struct rpc_request_handler_s
{
    api_dispatch_ft fn;

    /// function is always safe to run immediately instead of being
    /// put in a request queue for handling when nvim waits for input.
    bool async;
} rpc_request_handler_st;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "api/private/dispatch.h.generated.h"
    #include "api/private/dispatch_wrappers.h.generated.h"
#endif

#endif // NVIM_API_PRIVATE_DISPATCH_H
