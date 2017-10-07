/// @file nvim/profile.h

#ifndef NVIM_PROFILE_H
#define NVIM_PROFILE_H

#include <time.h>
#include <stdint.h>

typedef uint64_t proftime_T;

/// trace mainly startup logging messages
#define TIME_MSG(s)                            \
    do                                         \
    {                                          \
        if(time_fd != NULL) time_msg(s, NULL); \
    } while(0)

/// trace mainly startup logging messages
#define INFO_MSG(fmt, ...)                                    \
    do                                                        \
    {                                                         \
        if(time_fd != NULL)                                   \
        {                                                     \
            char debug_msg_buf[256] = {0};                    \
            snprintf(debug_msg_buf, 256, fmt, ##__VA_ARGS__); \
            time_msg(debug_msg_buf, NULL);                    \
        }                                                     \
    } while(0)

// for MinRelease, disable all loggings
#ifdef NVIM_LOGGING_DISABLE
    #undef  TIME_MSG
    #undef  INFO_MSG
    #define TIME_MSG(s)
    #define INFO_MSG(fmt, ...)
#endif

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "profile.h.generated.h"
#endif

#endif // NVIM_PROFILE_H
