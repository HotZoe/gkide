/// @file nvim/log.h

#ifndef NVIM_LOG_H
#define NVIM_LOG_H

#include <stdio.h>
#include <stdbool.h>

#define TRACE_LOG_LEVEL     0
#define DEBUG_LOG_LEVEL     1
#define STATE_LOG_LEVEL     2
#define ALERT_LOG_LEVEL     3
#define ERROR_LOG_LEVEL     4
#define FATAL_LOG_LEVEL     5

#define TRACE_LOG(...)
#define TRACE_LOGN(...) /* Trace */
#define DEBUG_LOG(...)
#define DEBUG_LOGN(...) /* Debug */
#define STATE_LOG(...)
#define STATE_LOGN(...) /* State */
#define ALERT_LOG(...)
#define ALERT_LOGN(...) /* Alert */
#define ERROR_LOG(...)
#define ERROR_LOGN(...) /* Error */
#define FATAL_LOG(...)
#define FATAL_LOGN(...) /* Fatal */

// Logging is disabled if NDEBUG or NVIM_LOGGING_DISABLE is defined.
#if !defined(NVIM_LOGGING_DISABLE) && defined(NDEBUG)
    #define NVIM_LOGGING_DISABLE
#endif

// NVIM_LOG_LEVEL_MIN can be defined during compilation to adjust the desired level of logging.
// STATE_LOG_LEVEL is used by default
#ifndef NVIM_LOG_LEVEL_MIN
    #define NVIM_LOG_LEVEL_MIN   STATE_LOG_LEVEL
#endif

#ifndef NVIM_LOGGING_DISABLE
    #if NVIM_LOG_LEVEL_MIN <= TRACE_LOG_LEVEL
        #undef  TRACE_LOG
        #undef  TRACE_LOGN
        #define TRACE_LOG(...)  (void)do_log(TRACE_LOG_LEVEL, __func__, __LINE__, true,  __VA_ARGS__)
        #define TRACE_LOGN(...) (void)do_log(TRACE_LOG_LEVEL, __func__, __LINE__, false, __VA_ARGS__)
    #endif

    #if NVIM_LOG_LEVEL_MIN <= DEBUG_LOG_LEVEL
        #undef  DEBUG_LOG
        #undef  DEBUG_LOGN
        #define DEBUG_LOG(...)  (void)do_log(DEBUG_LOG_LEVEL, __func__, __LINE__, true,  __VA_ARGS__)
        #define DEBUG_LOGN(...) (void)do_log(DEBUG_LOG_LEVEL, __func__, __LINE__, false, __VA_ARGS__)
    #endif

    #if NVIM_LOG_LEVEL_MIN <= STATE_LOG_LEVEL
        #undef  STATE_LOG
        #undef  STATE_LOGN
        #define STATE_LOG(...)  (void)do_log(STATE_LOG_LEVEL, __func__, __LINE__, true,  __VA_ARGS__)
        #define STATE_LOGN(...) (void)do_log(STATE_LOG_LEVEL, __func__, __LINE__, false, __VA_ARGS__)
    #endif

    #if NVIM_LOG_LEVEL_MIN <= ALERT_LOG_LEVEL
        #undef  ALERT_LOG
        #undef  ALERT_LOGN
        #define ALERT_LOG(...)  (void)do_log(ALERT_LOG_LEVEL, __func__, __LINE__, true,  __VA_ARGS__)
        #define ALERT_LOGN(...) (void)do_log(ALERT_LOG_LEVEL, __func__, __LINE__, false, __VA_ARGS__)
    #endif

    #if NVIM_LOG_LEVEL_MIN <= ERROR_LOG_LEVEL
        #undef  ERROR_LOG
        #undef  ERROR_LOGN
        #define ERROR_LOG(...)  (void)do_log(ERROR_LOG_LEVEL, __func__, __LINE__, true,  __VA_ARGS__)
        #define ERROR_LOGN(...) (void)do_log(ERROR_LOG_LEVEL, __func__, __LINE__, false, __VA_ARGS__)
    #endif

    #if NVIM_LOG_LEVEL_MIN <= FATAL_LOG_LEVEL
        #undef  FATAL_LOG
        #undef  FATAL_LOGN
        #define FATAL_LOG(...)  (void)do_log(FATAL_LOG_LEVEL, __func__, __LINE__, true,  __VA_ARGS__)
        #define FATAL_LOGN(...) (void)do_log(FATAL_LOG_LEVEL, __func__, __LINE__, false, __VA_ARGS__)
    #endif
#endif

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "log.h.generated.h"
#endif

#endif // NVIM_LOG_H
