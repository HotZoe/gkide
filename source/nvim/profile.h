/// @file nvim/profile.h

#ifndef NVIM_PROFILE_H
#define NVIM_PROFILE_H

#include <time.h>
#include <stdint.h>

typedef uint64_t proftime_kt;

/// trace mainly startup logging messages
#define TIME_MSG(s)                            \
    do                                         \
    {                                          \
        if(time_fd != NULL) time_msg(s, NULL); \
    } while(0)

/// trace mainly startup logging messages
#define INFO_MSG(fmt, ...)                              \
    do                                                  \
    {                                                   \
        if(time_fd != NULL)                             \
        {                                               \
            char msg_buf[256] = { 0 };                  \
            snprintf(msg_buf, 256, fmt, ##__VA_ARGS__); \
            time_msg(msg_buf, NULL);                    \
        }                                               \
    } while(0)

// for MinRelease, disable all loggings
#ifdef NVIM_LOGGING_DISABLE
    #undef  TIME_MSG
    #undef  INFO_MSG
    #define TIME_MSG(s)
    #define INFO_MSG(fmt, ...)
#endif

#ifndef DEV_TRACE_MSG
    /// For understanding and tracing the program execution
    /// in very details. This will given very details startup
    /// log messages, slow down the execution
    #define DEV_TRACE_MSG(fmt, ...)
#endif

#define DO_DEV_TRACE_MSG(fmt, ...)                  \
    do                                              \
    {                                               \
        if(time_fd != NULL)                         \
        {                                           \
            char msg_buf[1024] = { 0 };             \
            snprintf(msg_buf, 1024, "<%s,%d> "fmt,  \
                     __func__, __LINE__,            \
                     ##__VA_ARGS__);                \
            time_msg(msg_buf, NULL);                \
        }                                           \
    } while(0)

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "profile.h.generated.h"
#endif

#endif // NVIM_PROFILE_H
