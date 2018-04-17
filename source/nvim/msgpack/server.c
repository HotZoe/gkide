/// @file nvim/msgpack/server.c

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "nvim/msgpack/channel.h"
#include "nvim/msgpack/server.h"
#include "nvim/os/os.h"
#include "nvim/os_unix.h"
#include "nvim/event/socket.h"
#include "nvim/ascii.h"
#include "nvim/error.h"
#include "nvim/eval.h"
#include "nvim/garray.h"
#include "nvim/nvim.h"
#include "nvim/main.h"
#include "nvim/memory.h"
#include "nvim/log.h"
#include "nvim/charset.h"
#include "nvim/fileio.h"
#include "nvim/path.h"
#include "nvim/strings.h"

#include "generated/config/config.h"
#include "generated/config/gkideenvs.h"

/// nvim server max connections
#define MAX_CONNECTIONS         32

/// nvim network server default port
#define SERVER_DEFAULT_PORT     6666

#define EMPTY_HOST_IF_INFO      { 0, NULL }

static garray_st watchers = GA_EMPTY_INIT_VALUE;

// nvim server server info
static server_addr_info_st nvim_server_addr;

// host interface info list
static host_if_info_st host_if_info = EMPTY_HOST_IF_INFO;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "msgpack/server.c.generated.h"
#endif

static bool is_ip_address_char_valid(const char *addr)
{
    if(NULL == addr)
    {
        return false;
    }

    while(*addr)
    {
        char c = addr[0];
        if(isxdigit(c) || c == ':' || c == '.')
        {
            addr++;
        }
        else
        {
            return false;
        }
    }
    return true;
}

static bool is_ip_address_valid(const char *addr)
{
    char data[10] = { 0 };
    int ip_num[16] = { -1 };
    bool ip_v4_valid = true;

    if(false == is_ip_address_char_valid(addr))
    {
        return false;
    }

    // check for IPv4
    sscanf(addr, "%d.%d.%d.%d%s",
           ip_num + 0, ip_num + 1, ip_num + 2, ip_num + 3, data);

    if(ip_num[0] > 255 || ip_num[0] < 0
       || ip_num[1] > 255 || ip_num[1] < 0
       || ip_num[2] > 255 || ip_num[2] < 0
       || ip_num[3] > 255 || ip_num[3] < 0)
    {
        ip_v4_valid = false;
    }

    if(data[0] != NUL)
    {
        ip_v4_valid = false;
    }

    if(ip_v4_valid)
    {
        return true;
    }

    // check for IPv6
    int sep_cnt_v6 = 0;
    int sep_cnt_v4 = 0;
    bool found_double_colon = false;
    bool found_embed_ipv4 = false;

    // for detect A:B:1.2:D.3 and like strings
    int sep_idx = 0;
    char found_sep_seq[11] = { NUL };

    int blk_idx = 0; // 0, 1, 2, 3
    memset(data, NUL, 5);

    while(*addr)
    {
        char c = addr[0];
        bool is_separator = false;
        if(isxdigit(c))
        {
            data[blk_idx++] = c;
        }
        else
        {
            // ':' or '.'
            is_separator = true;
            found_sep_seq[sep_idx++] = c;
            if('.' == c)
            {
                // ::ffff:192.1.56.10
                sep_cnt_v4++;
                found_embed_ipv4 = true;
                if(sep_cnt_v4 > 3)
                {
                    // the embed IPv4 invalid
                    return false;
                }
            }
            else
            {
                sep_cnt_v6++;
                if(':' == addr[1])
                {
                    sep_cnt_v6--; // :: as a whole
                    if(!found_double_colon)
                    {
                        // ff06:0:0:0:0:0:0:c3  <=> ff06::c3
                        found_double_colon = true;
                    }
                    else
                    {
                        return false;
                    }

                }
            }
        }

        if(blk_idx > 3)
        {
            // a block can not have more then 4 digit char
            return false;
        }

        if(is_separator || NUL == addr[1])
        {
            intmax_t num;
            uchar_kt *err_char = (uchar_kt *)data;
            int ret = getdigits_safe(&err_char, &num);

            if(ret == FAIL || num < 0 || num > 0xFFFF)
            {
                return false;
            }

            if('.' == c && num > 255)
            {
                return false;
            }

            blk_idx = 0;
            memset(data, NUL, 5);
        }

        addr++;
    }

    if(found_embed_ipv4)
    {
        // check for stupid strings
        bool have_colon = false;
        while(sep_idx--)
        {
            if(have_colon
               && '.' == found_sep_seq[sep_idx])
            {
                // what are this, are you kiding?
                return false;
            }

            if(':' == found_sep_seq[sep_idx])
            {
                have_colon = true;
            }
        }

        if(!found_double_colon
           && 6 != sep_cnt_v6)
        {
            return false;
        }
        else
        {
            if(sep_cnt_v6 > 5)
            {
                return false;
            }
        }
    }
    else
    {
        if(!found_double_colon
           && 7 != sep_cnt_v6)
        {
            return false;
        }
        else
        {
            if(sep_cnt_v6 > 5)
            {
                return false;
            }
        }
    }

    return true;
}

/// nvim server address info default vale
///
/// @param addr     nvim server address
///
/// @note
/// - piped-name        unix socket/piped name
/// - eth-name:port     TCP connection, IPv4 come first, if not then IPv6
/// - IP:port           TCP connection
void init_server_addr_info(const char *addr)
{
    init_host_interfaces_info();

    const char *listen_address = addr;
    int server_type = kServerTypeNotStart;

    if(NULL == listen_address)
    {
        // --server, get and check $GKIDE_NVIM_LISTEN
        listen_address = os_getenv(ENV_GKIDE_NVIM_LISTEN);
    }

    if(NULL == listen_address)
    {
        // unix socket or named pipe
        server_type = kServerTypeLocal;
        // the auto generated name should be make later, because
        // for now, something is not properly inited yet!
        listen_address = NULL;
    }

    if(kServerTypeLocal == server_type)
    {
        nvim_server_addr.server_type = kServerTypeLocal;
        nvim_server_addr.data.local = listen_address;
        return;
    }

    char *addr_end = (char *)strrchr(listen_address, ':');
    if(NULL == addr_end && !is_ip_address_valid(listen_address))
    {
        // unix socket or named pipe
        nvim_server_addr.server_type = kServerTypeLocal;
        nvim_server_addr.data.local = listen_address;
        return;
    }

    // prepare for TCP connection
    nvim_server_addr.server_type = kServerTypeNetwork;

    char *server_port = NULL;
    const char *server_addr = listen_address;
    if(addr_end)
    {
        *addr_end = '\0';
        server_port = addr_end + 1;
    }

    if(NULL == server_port || NUL == *server_port)
    {
        // address ending with :, no port given
        // use the default nvim server port number
        nvim_server_addr.data.network.port = SERVER_DEFAULT_PORT;
    }
    else
    {
        intmax_t iport;
        uchar_kt *port_err = (uchar_kt *)server_port;
        int ret = getdigits_safe(&port_err, &iport);

        if(ret == FAIL || iport < 0 || iport > UINT16_MAX)
        {
            mch_errmsg("Invalid port: ");
            mch_errmsg(server_port);
            mch_errmsg("\n");
            exit(kNEStatusNvimServerInvalidPort);
        }
        nvim_server_addr.data.network.port = (unsigned short)iport;
    }

    nvim_server_addr.data.network.if_info = NULL;

    // find out a working interface to use for remote connection
    if_info_st *ptr = host_if_info.header;
    while(NULL != ptr)
    {
        if(ustricmp(server_addr, ptr->if_name) == 0)
        {
            if(AF_INET == ptr->ip_protocol)
            {
                nvim_server_addr.data.network.if_info = ptr;
                nvim_server_addr.data.network.prefer_protocol = AF_INET;
            }
            else if(ptr->brother
                    && AF_INET == ptr->brother->ip_protocol)
            {
                nvim_server_addr.data.network.if_info = ptr->brother;
                nvim_server_addr.data.network.prefer_protocol = AF_INET;
            }
            else
            {
                nvim_server_addr.data.network.if_info = ptr;
                nvim_server_addr.data.network.prefer_protocol = AF_INET6;
            }
            break;
        }
        else if(ustricmp(server_addr, ptr->if_address) == 0)
        {
            nvim_server_addr.data.network.if_info = ptr;
            nvim_server_addr.data.network.prefer_protocol = ptr->ip_protocol;
            break;
        }
        ptr = ptr->next;
    }

    if(NULL == nvim_server_addr.data.network.if_info)
    {
        // the given address is neither of one of the host IPs or eth-names
        mch_errmsg("Invalid address: ");
        mch_errmsg(server_addr);
        mch_errmsg("\n");
        exit(kNEStatusNvimServerInvalidAddr);
    }

    if(nvim_server_addr.data.network.if_info->is_internal)
    {
        mch_errmsg("Local network nvim server only!\n");
    }
}

/// get host interfaces information
static void init_host_interfaces_info(void)
{
    // just in case ...
    if(host_if_info.header != NULL
       && host_if_info.if_count != 0)
    {
        return;
    }

    int if_cnt = 0; // total number of interface
    uv_interface_address_t *ifs_info = NULL;

    uv_interface_addresses(&ifs_info, &if_cnt);
    host_if_info.if_count = if_cnt;

    if(0 == if_cnt)
    {
        return;
    }

    host_if_info.header = xmalloc(sizeof(if_info_st));

    char buf[512];
    int i = if_cnt;
    if_info_st *ptr = host_if_info.header;
    while(i--)
    {
        uv_interface_address_t info = ifs_info[i];

        ptr->if_name = xstrdup(info.name); // if name
        ptr->is_internal = info.is_internal; // internal or not

        if(info.address.address4.sin_family == AF_INET)
        {
            ptr->ip_protocol = AF_INET;
            uv_ip4_name(&info.address.address4, buf, sizeof(buf));
        }
        else if(info.address.address4.sin_family == AF_INET6)
        {
            ptr->ip_protocol = AF_INET6;
            uv_ip6_name(&info.address.address6, buf, sizeof(buf));
        }

        ptr->if_address = xstrdup(buf);
        ptr->next = NULL;
        ptr->brother = NULL;

        if(0 == i)
        {
            break;
        }
        else
        {
            ptr->next = xmalloc(sizeof(if_info_st));
            ptr = ptr->next;
        }
    }

    uv_free_interface_addresses(ifs_info, if_cnt);

    // find brothors
    i = if_cnt;
    ptr = host_if_info.header;
    while(i--)
    {
        int k = if_cnt;
        const char *if_name = ptr->if_name;
        if_info_st *chk_ptr = host_if_info.header;
        while(k--)
        {
            if(chk_ptr != ptr /* not the same if info object */
               && ustricmp(if_name, chk_ptr->if_name) == 0)
            {
                ptr->brother = chk_ptr;
                break;
            }

            chk_ptr = chk_ptr->next;
        }

        ptr = ptr->next;
    }

    assert(host_if_info.if_count != 0);
    assert(host_if_info.header != NULL);
}

/// return host interface info if it match
///
/// @param addr_or_name     IP address or interface name
if_info_st *get_addr_binded_host_if_info(const char *addr_or_name)
{
    int if_cnt = host_if_info.if_count;
    if_info_st *ptr = host_if_info.header;
    while(if_cnt--)
    {
        if(ustricmp(addr_or_name, ptr->if_address) == 0
           || ustricmp(addr_or_name, ptr->if_name) == 0)
        {
            return ptr;
        }
        ptr = ptr->next;
    }

    return NULL;
}

/// start nvim server or not
/// - true: start nvim server
/// - false: do not start nvim server
bool start_nvim_server(void)
{
    return nvim_server_addr.server_type;
}

/// Initializes the nvim server module
bool server_init(void)
{
    char addr_port[ADDRESS_MAX_SIZE] = { 0 };
    switch(nvim_server_addr.server_type)
    {
        case kServerTypeNotStart:
        {
            // no --server option
            // do not start nvim server for client connection
            return true;
        }
        case kServerTypeLocal:
        {
            if(NULL == nvim_server_addr.data.local)
            {
                nvim_server_addr.data.local = server_address_new();
            }
            ustrncpy(addr_port,
                     nvim_server_addr.data.local,
                     ADDRESS_MAX_SIZE - 1);
            break;
        }
        case kServerTypeNetwork:
        {
            char *srv_addr = nvim_server_addr.data.network.if_info->if_address;
            unsigned short srv_port = nvim_server_addr.data.network.port;
            ustrncpy(addr_port, srv_addr, ADDRESS_MAX_SIZE-1);
            xvsnprintf_add(addr_port, ADDRESS_MAX_SIZE-1, ":%d", srv_port);
            break;
        }
        default:
        {
            TO_FIX_THIS("This should never happen!");
        }
    }

    ga_init(&watchers, sizeof(socket_watcher_st *), 1);

    if(server_start(addr_port) == 0)
    {
        return true;
    }

    return false;
}

/// Teardown a single server
static void close_socket_watcher(socket_watcher_st **watcher)
{
    socket_watcher_close(*watcher, free_server);
}

/// Set v:servername to the first server in the server
/// list, or unset it if no servers are known.
static void set_vservername(garray_st *srvs)
{
    char *default_server = (srvs->ga_len > 0)
                           ? ((socket_watcher_st **)srvs->ga_data)[0]->addr
                           : NULL;

    set_vim_var_string(VV_SEND_SERVER, default_server, -1);
}

/// Teardown the server module
void server_teardown(void)
{
    GA_DEEP_CLEAR(&watchers, socket_watcher_st *, close_socket_watcher);
}

/// Generates unique address for local server.
///
/// In Windows this is a named pipe in the format
///     \\.\pipe\nvim-<PID>-<COUNTER>.
///
/// For other systems it is a path returned by vim_tempname().
///
/// This function is NOT thread safe
char *server_address_new(void)
{
#ifdef HOST_OS_WINDOWS
    static uint32_t count = 0;
    char template[ADDRESS_MAX_SIZE];
    snprintf(template,
             ADDRESS_MAX_SIZE,
             "\\\\.\\pipe\\nvim-%" PRIu64 "-%" PRIu32,
             os_get_pid(),
             count++);
    return xstrdup(template);
#else
    return (char *)vim_tempname();
#endif
}

/// Check if this instance owns a pipe address.
/// The argument must already be resolved to an absolute path!
bool server_owns_pipe_address(const char *path)
{
    for(int i = 0; i < watchers.ga_len; i++)
    {
        if(!strcmp(path, ((socket_watcher_st **)watchers.ga_data)[i]->addr))
        {
            return true;
        }
    }

    return false;
}

/// Starts listening for API calls.
///
/// The socket type is determined by parsing `endpoint`: If it's a valid IPv4
/// or IPv6 address in 'ip:[port]' format, then it will be a TCP socket.
/// Otherwise it will be a Unix socket or named pipe (Windows).
///
/// If no port is given, a random one will be assigned.
///
/// @param endpoint Address of the server. Either a 'ip:[port]' string or an
///                 arbitrary identifier (trimmed to 256 bytes) for the Unix
///                 socket or named pipe.
/// @returns 0 on success, 1 on a regular error, and negative errno
///          on failure to bind or listen.
int server_start(const char *endpoint)
{
    if(endpoint == NULL || endpoint[0] == '\0')
    {
        ERROR_LOG("Empty or NULL endpoint");
        return 1;
    }

    socket_watcher_st *watcher = xmalloc(sizeof(socket_watcher_st));
    int result = socket_watcher_init(&main_loop, watcher, endpoint);

    if(result < 0)
    {
        xfree(watcher);
        return result;
    }

    // Check if a watcher for the endpoint already exists
    for(int i = 0; i < watchers.ga_len; i++)
    {
        if(!strcmp(watcher->addr,
                   ((socket_watcher_st **)watchers.ga_data)[i]->addr))
        {
            ERROR_LOG("Already listening on %s", watcher->addr);

            if(watcher->stream->type == UV_TCP)
            {
                uv_freeaddrinfo(watcher->uv.tcp.addrinfo);
            }

            socket_watcher_close(watcher, free_server);
            return 1;
        }
    }

    result = socket_watcher_start(watcher, MAX_CONNECTIONS, connection_cb);

    if(result < 0)
    {
        ERROR_LOG("Failed to start server: %s", uv_strerror(result));
        socket_watcher_close(watcher, free_server);
        return result;
    }

    // Update $GKIDE_NVIM_LISTEN, if not set.
    const char *listen_address = os_getenv(ENV_GKIDE_NVIM_LISTEN);

    if(listen_address == NULL)
    {
        os_setenv(ENV_GKIDE_NVIM_LISTEN, watcher->addr, 1);
    }

    // Add the watcher to the list.
    ga_grow(&watchers, 1);
    ((socket_watcher_st **)watchers.ga_data)[watchers.ga_len++] = watcher;

    // Update v:servername, if not set.
    if(ustrlen(get_vim_var_str(VV_SEND_SERVER)) == 0)
    {
        set_vservername(&watchers);
    }

    return 0;
}

/// Stops listening on the address specified by `endpoint`.
///
/// @param endpoint Address of the server.
void server_stop(char *endpoint)
{
    socket_watcher_st *watcher;
    char addr[ADDRESS_MAX_SIZE];

    // Trim to 'ADDRESS_MAX_SIZE'
    xstrncpy(addr, endpoint, sizeof(addr));
    int i = 0; // Index of the server whose address equals addr.

    for(; i < watchers.ga_len; i++)
    {
        watcher = ((socket_watcher_st **)watchers.ga_data)[i];

        if(strcmp(addr, watcher->addr) == 0)
        {
            break;
        }
    }

    if(i >= watchers.ga_len)
    {
        ERROR_LOG("Not listening on %s", addr);
        return;
    }

    // Unset $GKIDE_NVIM_LISTEN if it is the stopped address.
    const char *listen_address = os_getenv(ENV_GKIDE_NVIM_LISTEN);

    if(listen_address && ustrcmp(addr, listen_address) == 0)
    {
        os_unsetenv(ENV_GKIDE_NVIM_LISTEN);
    }

    socket_watcher_close(watcher, free_server);

    // Remove this server from the list by swapping it with the last item.
    if(i != watchers.ga_len - 1)
    {
        ((socket_watcher_st **)watchers.ga_data)[i] =
            ((socket_watcher_st **)watchers.ga_data)[watchers.ga_len - 1];
    }

    watchers.ga_len--;

    // If v:servername is the stopped address, re-initialize it.
    if(ustrcmp(addr, get_vim_var_str(VV_SEND_SERVER)) == 0)
    {
        set_vservername(&watchers);
    }
}

/// Returns an allocated array of server addresses.
/// @param[out] size The size of the returned array.
char **server_address_list(size_t *size)
FUNC_ATTR_NONNULL_ALL
{
    if((*size = (size_t)watchers.ga_len) == 0)
    {
        return NULL;
    }

    char **addrs = xcalloc((size_t)watchers.ga_len, sizeof(const char *));

    for(int i = 0; i < watchers.ga_len; i++)
    {
        addrs[i] = xstrdup(((socket_watcher_st **)watchers.ga_data)[i]->addr);
    }

    return addrs;
}

static void connection_cb(socket_watcher_st *watcher,
                          int result,
                          void *FUNC_ARGS_UNUSED_MATCH(data))
{
    if(result)
    {
        ERROR_LOG("Failed to accept connection: %s", uv_strerror(result));
        return;
    }

    channel_from_connection(watcher);
}

static void free_server(socket_watcher_st *watcher,
                        void *FUNC_ARGS_UNUSED_MATCH(data))
{
    xfree(watcher);
}
