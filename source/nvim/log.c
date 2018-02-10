/// @file nvim/log.c

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nvim/log.h"
#include "nvim/types.h"
#include "nvim/os/os.h"
#include "nvim/os/time.h"

#include "generated/config/config.h"
#include "generated/config/gkideenvs.h"

/// First location of the log file used by log_path_init()
#define USR_LOG_FILE          "$" ENV_GKIDE_NVIM_RTMLOG

/// Fall back location of the log file used by log_path_init()
#define USR_LOG_FILE_DEFAULT  "$" ENV_GKIDE_USR_HOME OS_PATH_SEP_STR "nvim.log"

/// Cached location of the log file set by log_path_init()
static char expanded_log_file_path[MAXPATHL + 1] = { 0 };

static uv_mutex_t log_mutex;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "log.c.generated.h"
#endif

/// Initialize path to log file
///
/// Tries to use USR_LOG_FILE, then falls back USR_LOG_FILE_DEFAULT.
/// Path to log file is cached, so only the first call has effect,
/// unless first call was not successful. To make initialization not
/// succeed either a bug in expand_env() is needed or both
/// @b $NVIM_LOG_FILE and @b $HOME environment variables undefined.
///
/// @return true if path was initialized, false otherwise.
static bool log_path_init(void)
{
    if(expanded_log_file_path[0])
    {
        return true;
    }

    expand_env((uchar_kt *)USR_LOG_FILE,
               (uchar_kt *)expanded_log_file_path,
               sizeof(expanded_log_file_path) - 1);

    // if the log file path expansion failed then fall back to stderr
    if(strcmp(USR_LOG_FILE, expanded_log_file_path) == 0)
    {
        memset(expanded_log_file_path, 0, sizeof(expanded_log_file_path));

        expand_env((uchar_kt *)USR_LOG_FILE_DEFAULT,
                   (uchar_kt *)expanded_log_file_path,
                   sizeof(expanded_log_file_path) - 1);

        if(strcmp(USR_LOG_FILE_DEFAULT, expanded_log_file_path) == 0)
        {
            memset(expanded_log_file_path, 0, sizeof(expanded_log_file_path));
            return false;
        }
    }

    INFO_MSG("nvim runtime logfile is %s", expanded_log_file_path);
    return true;
}

void log_init(void)
{
    uv_mutex_init(&log_mutex);
}

void log_lock(void)
{
    uv_mutex_lock(&log_mutex);
}

void log_unlock(void)
{
    uv_mutex_unlock(&log_mutex);
}

bool do_log(int log_level,
            const char *func_name,
            int line_num,
            bool eol,
            const char *fmt, ...)
FUNC_ATTR_UNUSED
{
    log_lock();
    bool ret = false;
    FILE *log_file = open_log_file();

    if(log_file == NULL)
    {
        goto do_log_exit;
    }

    va_list args;
    va_start(args, fmt);

    ret = v_do_log_to_file(log_file,
                           log_level,
                           func_name,
                           line_num,
                           eol,
                           fmt,
                           args);

    va_end(args);

    if(log_file != stderr && log_file != stdout)
    {
        fclose(log_file);
    }

do_log_exit:

    log_unlock();

    return ret;
}

/// Open the log file for appending.
///
/// @return
/// The FILE* specified by the USR_LOG_FILE path or stderr in case of error
FILE *open_log_file(void)
{
    static bool opening_log_file = false;

    // check if it's a recursive call
    if(opening_log_file)
    {
        do_log_to_file(stderr, ERROR_LOG_LEVEL, __func__, __LINE__, true,
                       "Trying to do logging recursively! Please fix it.");

        return stderr;
    }

    // expand USR_LOG_FILE if needed and open the file
    FILE *log_file = NULL;
    opening_log_file = true;

    if(log_path_init())
    {
        log_file = fopen(expanded_log_file_path, "a");
    }

    opening_log_file = false;

    if(log_file != NULL)
    {
        return log_file;
    }

    do_log_to_file(stderr, ERROR_LOG_LEVEL, __func__, __LINE__, true,
                   "Couldn't open $GKIDE_NVIM_LOG_FILE or $HOME/.gkide/nvim.log, "
                   "logging to stderr! This may be caused by attempting to do logging "
                   "before the initialization functions are called (e.g. init_gkide_usr_home()).");
    return stderr;
}

static bool do_log_to_file(FILE *log_file,
                           int log_level,
                           const char *func_name,
                           int line_num,
                           bool eol,
                           const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    bool ret = v_do_log_to_file(log_file,
                                log_level,
                                func_name,
                                line_num,
                                eol,
                                fmt,
                                args);

    va_end(args);
    return ret;
}

static bool v_do_log_to_file(FILE *log_file,
                             int log_level,
                             const char *func_name,
                             int line_num,
                             bool eol,
                             const char *fmt,
                             va_list args)
{
    static const char *log_levels[] =
    {
        [TRACE_LOG_LEVEL] = "TRACE",
        [DEBUG_LOG_LEVEL] = "DEBUG",
        [STATE_LOG_LEVEL] = "STATE",
        [ALERT_LOG_LEVEL] = "ALERT",
        [ERROR_LOG_LEVEL] = "ERROR",
        [FATAL_LOG_LEVEL] = "FATAL"
    };

    assert(log_level >= TRACE_LOG_LEVEL && log_level <= FATAL_LOG_LEVEL);
    struct tm local_time; // format current timestamp in local time

    if(os_get_localtime(&local_time) == NULL)
    {
        return false;
    }

    char date_time[20];

    if(strftime(date_time, sizeof(date_time),
                "%Y/%m/%d %H:%M:%S", &local_time) == 0)
    {
        return false;
    }

    // print the log message prefixed by the current timestamp and pid
    int64_t pid = os_get_pid();

    // pid, date time, log level, function name, line number, message
    if(fprintf(log_file, "%" PRId64 "  %s  %s  %s@%d: ", pid,
               date_time, log_levels[log_level], func_name, line_num) < 0)
    {
        return false;
    }

    if(vfprintf(log_file, fmt, args) < 0)
    {
        return false;
    }

    if(eol)
    {
        fputc('\n', log_file);
    }

    if(fflush(log_file) == EOF)
    {
        return false;
    }

    return true;
}
