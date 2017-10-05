/// @file nvim/profile.h

#ifndef NVIM_PROFILE_H
#define NVIM_PROFILE_H

#include <time.h>
#include <stdint.h>

typedef uint64_t proftime_T;

#define TIME_MSG(s)                            \
    do                                         \
    {                                          \
        if(time_fd != NULL) time_msg(s, NULL); \
    } while(0)

#define DEV_MSG(fmt, ...)                                     \
    do                                                        \
    {                                                         \
        if(time_fd != NULL)                                   \
        {                                                     \
            char debug_msg_buf[1024] = {0};                   \
            snprintf(debug_msg_buf, 1024, fmt, ##__VA_ARGS__);\
            time_msg(debug_msg_buf, NULL);                    \
        }                                                     \
    } while(0)

#ifndef NVIM_DEVMSG_ENABLE
    #undef  DEV_MSG(fmt, ...)
    #define DEV_MSG(fmt, ...)
#endif

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "profile.h.generated.h"
#endif

#endif // NVIM_PROFILE_H
