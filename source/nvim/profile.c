/// @file nvim/profile.c

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "nvim/profile.h"
#include "nvim/os/time.h"
#include "nvim/func_attr.h"
#include "nvim/os/os_defs.h"

// for the global 'time_fd' (startuptime)
#include "nvim/globals.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "profile.c.generated.h"
#endif

/// @todo: what is this ?
static proftime_T prof_wait_time;

/// return the current time
///
/// @return the current time
proftime_T profile_start(void)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return os_hrtime();
}

/// compute the time elapsed
///
/// @return the elapsed time from @b tm until now.
proftime_T profile_end(proftime_T tm)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return os_hrtime() - tm;
}

/// return a string that represents the time in @b tm
///
/// @warning
/// Do not modify or free this string, not multithread-safe.
///
/// @param tm
/// The time to be represented
///
/// @return
/// a static string representing @b tm in the form "seconds.microseconds".
const char *profile_msg(proftime_T tm)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    static char buf[50];

    snprintf(buf, sizeof(buf), "%10.6lf", (double)tm / 1000000000.0);

    return buf;
}

/// return the time @b msec into the future
///
/// @param msec
/// milliseconds, the maximum number of milliseconds
/// is (2^63 / 10^6) - 1 = 9.223372e+12.
///
/// @return
/// if msec > 0, returns the time msec past now.
/// Otherwise returns the zero time.
proftime_T profile_setlimit(int64_t msec)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(msec <= 0)
    {
        // no limit
        return profile_zero();
    }

    assert(msec <= (INT64_MAX / 1000000LL) - 1);

    proftime_T nsec = (proftime_T) msec * 1000000ULL;
    return os_hrtime() + nsec;
}

/// check if current time has passed @b tm
///
/// @return
/// true if the current time is past @b tm,
/// false if not or if the timer was not set.
bool profile_passed_limit(proftime_T tm)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(tm == 0)
    {
        // timer was not set
        return false;
    }

    return profile_cmp(os_hrtime(), tm) < 0;
}

/// obtain the zero time
///
/// @return the zero time
proftime_T profile_zero(void)
FUNC_ATTR_CONST
{
    return 0;
}

/// divide the time @b tm by @b count.
///
/// @return
/// 0 if count <= 0, otherwise @b tm / @b count
proftime_T profile_divide(proftime_T tm, int count)
FUNC_ATTR_CONST
{
    if(count <= 0)
    {
        return profile_zero();
    }

    return (proftime_T) round((double) tm / (double) count);
}

/// add the time @b tm2 to @b tm1
///
/// @return @b tm1 + @b tm2
proftime_T profile_add(proftime_T tm1, proftime_T tm2)
FUNC_ATTR_CONST
{
    return tm1 + tm2;
}

/// subtract @b tm2 from @b tm1
///
/// @return @b tm1 - @b tm2
proftime_T profile_sub(proftime_T tm1, proftime_T tm2)
FUNC_ATTR_CONST
{
    return tm1 - tm2;
}

/// add the @b self time from the total time and the children's time
///
/// @return
/// if @b total <= @b children, then self, otherwise
/// @b self + @b total - @b children
proftime_T profile_self(proftime_T self,
                        proftime_T total,
                        proftime_T children)
FUNC_ATTR_CONST
{
    // check that the result won't be negative,
    // which can happen with recursive calls.
    if(total <= children)
    {
        return self;
    }

    // add the total time to self and subtract the children's time from self
    return profile_sub(profile_add(self, total), children);
}

/// get the current waittime
///
/// @return the current waittime
proftime_T profile_get_wait(void)
FUNC_ATTR_PURE
{
    return prof_wait_time;
}

/// set the current waittime
void profile_set_wait(proftime_T wait)
{
    prof_wait_time = wait;
}

/// subtract the passed waittime since @b tm
///
/// @return @b tma - (waittime - @b tm)
proftime_T profile_sub_wait(proftime_T tm, proftime_T tma)
FUNC_ATTR_PURE
{
    proftime_T tm3 = profile_sub(profile_get_wait(), tm);
    return profile_sub(tma, tm3);
}

/// check if @b tm1 is equal to @b tm2
///
/// @return true if @b tm1 == @b tm2
bool profile_equal(proftime_T tm1, proftime_T tm2)
FUNC_ATTR_CONST
{
    return tm1 == tm2;
}

/// calculates the sign of a 64-bit integer
///
/// @return -1, 0, or +1
static inline int sgn64(int64_t x)
FUNC_ATTR_CONST
{
    return (int)((x > 0) - (x < 0));
}

/// compare profiling times
///
/// Only guarantees correct results if both input
/// times are not more than 150 years apart.
///
/// @return
/// <0, 0 or >0 if @b tm2 < @b tm1, @b tm2 == @b tm1 or @b tm2 > @b tm1
int profile_cmp(proftime_T tm1, proftime_T tm2)
FUNC_ATTR_CONST
{
    return sgn64((int64_t)(tm2 - tm1));
}

/// nvim startup time, init on startup and nerver
/// changing will running, for time_* funcs to use
static proftime_T g_start_time;

/// the start point of a period of time, e.g: how
/// long do we need to load a plugin
static proftime_T g_prev_time;

/// save the previous time before doing something that could nest
///
/// After calling this function, the static global ::g_prev_time will
/// contain the current time.
///
/// @param[out] rel   to the time elapsed so far
/// @param[out] start the current time
void time_push(proftime_T *rel, proftime_T *start)
{
    proftime_T now = profile_start();

    // subtract the previous time from now, store it in 'rel'
    *rel = profile_sub(now, g_prev_time);
    *start = now;

    // reset global 'g_prev_time' for the next call
    g_prev_time = now;
}

/// compute the prev time after doing something that could nest
///
/// Subtracts @b tp from the static global ::g_prev_time.
///
/// @param tp the time to subtract
void time_pop(proftime_T tp)
{
    g_prev_time -= tp;
}

/// print the difference between @b then and @b now
///
/// the format is "msec.usec".
static void time_diff(proftime_T then, proftime_T now)
{
    proftime_T diff = profile_sub(now, then);
    fprintf(time_fd, "%07.3lf", (double) diff / 1.0E6);
}

/// initialize the startuptime code
///
/// Needs to be called once before calling other startuptime code,
/// such as functions link: time_push(), time_pop(), time_msg() ...
///
/// @param message
/// the message that will be displayed
void time_start(const char *message)
{
    if(time_fd == NULL)
    {
        return;
    }

    // intialize the global time variables
    g_prev_time = g_start_time = profile_start();
    fprintf(time_fd, "nvim startup logging file, times in msec\n\n");
    fprintf(time_fd, "msg-fmt-1: timeline    self+source    self: sourcing script\n");
    fprintf(time_fd, "msg-fmt-2: timeline    elapsedtime        : startup message\n\n");
    time_msg(message, NULL);
}

/// print out timing info
///
/// @warning
/// don't forget to call time_start() once before calling this.
///
/// @param mesg
/// the message to display next to the timing information
///
/// @param start
/// only for do_source(): start time
void time_msg(const char *mesg, const proftime_T *start)
{
    if(time_fd == NULL)
    {
        return;
    }

    // print out the difference between 'g_start_time' and 'now'
    proftime_T now = profile_start();
    time_diff(g_start_time, now);

    // if 'start' was supplied, print the diff between 'start' and 'now'
    // this is used to measure the script/plugin load time
    if(start != NULL)
    {
        fprintf(time_fd, "  ");
        time_diff(*start, now);
    }

    // print the difference between the global 'g_prev_time' and 'now'
    fprintf(time_fd, "  ");
    time_diff(g_prev_time, now);

    // reset 'g_prev_time' and print the message
    g_prev_time = now;
    fprintf(time_fd, ": %s\n", mesg);
}
