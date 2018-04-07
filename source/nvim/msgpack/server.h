/// @file nvim/msgpack/server.h

#ifndef NVIM_MSGPACK_RPC_SERVER_H
#define NVIM_MSGPACK_RPC_SERVER_H

#include <stdio.h>

/// interface info
typedef struct if_info_s if_info_st;
struct if_info_s
{
    char *if_name;      ///< interface name
    char *if_address;   ///< interface address, IPv4 or IPv6
    int is_internal;    ///< is this interface internal or not, true or false
    int ip_protocol;    ///< IP protocol
    if_info_st *brother;///< interface have same name but use IPv4/IPv6
    if_info_st *next;
};

/// host interface info
typedef struct host_if_info_s
{
    int if_count;       ///< host interface number
    if_info_st *header; ///< point to if info list
} host_if_info_st;

typedef enum server_type_e
{
    kServerTypeNotStart = 0, ///< do not start nvim server
    kServerTypeLocal    = 1, ///< use unix socket or windows named pipe
    kServerTypeNetwork  = 2, ///< for remote networking connection
} server_type_et;

/// nvim server information
typedef struct server_addr_info_s
{
    int server_type;             ///< local or network, @b server_type_e
    union
    {
        struct
        {
            if_info_st *if_info; ///< server used interface information
            int prefer_protocol; ///< which protocol prefered, IPv4 or IPv6
            unsigned short port; ///< server port
        } network;               ///< valid for @b kServerTypeNetwork
        const char *local;       ///< valid for @b kServerTypeLocal
    } data;
} server_addr_info_st;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "msgpack/server.h.generated.h"
#endif

#endif // NVIM_MSGPACK_RPC_SERVER_H
