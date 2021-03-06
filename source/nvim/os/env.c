/// @file nvim/os/env.c
///
/// Environment inspection

#include <assert.h>
#include <uv.h>

#include "nvim/nvim.h"
#include "nvim/ascii.h"
#include "nvim/charset.h"
#include "nvim/fileio.h"
#include "nvim/os/os.h"
#include "nvim/memory.h"
#include "nvim/message.h"
#include "nvim/path.h"
#include "nvim/strings.h"
#include "nvim/eval.h"
#include "nvim/ex_getln.h"
#include "nvim/version.h"

#include "generated/config/config.h"

#ifdef HOST_OS_WINDOWS
    // for utf8_to_utf16, utf16_to_utf8
    #include "nvim/mbyte.h"
#endif

#ifdef HAVE__NSGETENVIRON
    #include <crt_externs.h>
#endif

#ifdef HAVE_HDR_SYS_UTSNAME_H
    #include <sys/utsname.h>
#endif

/// Like getenv(), but returns NULL if the variable is empty.
const char *os_getenv(const char *name)
FUNC_ATTR_NONNULL_ALL
{
    const char *e = getenv(name);
    return (e == NULL || *e == NUL) ? NULL : e;
}

/// Returns true if the environment variable,
/// @b name, has been defined, even if empty.
bool os_env_exists(const char *name)
FUNC_ATTR_NONNULL_ALL
{
    return getenv(name) != NULL;
}

int os_setenv(const char *name,
              const char *value,
              int FUNC_ARGS_UNUSED_MAYBE(overwrite))
FUNC_ATTR_NONNULL_ALL
{
#ifdef HOST_OS_WINDOWS
    size_t envbuflen = strlen(name) + strlen(value) + 2;
    char *envbuf = xmalloc(envbuflen);
    snprintf(envbuf, envbuflen, "%s=%s", name, value);

    WCHAR *p;
    utf8_to_utf16(envbuf, &p);
    xfree(envbuf);

    if(p == NULL)
    {
        return -1;
    }

    // Unlike Unix systems, we can free the string for _wputenv().
    _wputenv(p);
    xfree(p);

    return 0;
#elif defined(HAVE_FUN_SETENV)
    return setenv(name, value, overwrite);
#elif defined(HAVE_FUN_PUTENV_S)
    if(!overwrite && os_getenv(name) != NULL)
    {
        return 0;
    }

    if(_putenv_s(name, value) == 0)
    {
        return 0;
    }

    return -1;
#else
    #error "This system has no implementation available for os_setenv()"
#endif
}

/// Unset environment variable
///
/// For systems where unsetenv() is not available
/// the value will be set as an empty string
int os_unsetenv(const char *name)
{
#ifdef HAVE_FUN_UNSETENV
    return unsetenv(name);
#else
    return os_setenv(name, "", 1);
#endif
}

char *os_getenvname_at_index(size_t index)
{
#if defined(HAVE__NSGETENVIRON)
    char **environ = *_NSGetEnviron();
#elif !defined(__WIN32__)
    // Borland C++ 5.2 has this in a header file.
    extern char **environ;
#endif

    // check if index is inside the environ array
    for(size_t i = 0; i < index; i++)
    {
        if(environ[i] == NULL)
        {
            return NULL;
        }
    }

    char *str = environ[index];

    if(str == NULL)
    {
        return NULL;
    }

    size_t namesize = 0;

    while(str[namesize] != '=' && str[namesize] != NUL)
    {
        namesize++;
    }

    char *name = (char *)ustrndup((uchar_kt *)str, namesize);
    return name;
}

/// Get the process ID of the Nvim process.
///
/// @return the process ID.
int64_t os_get_pid(void)
{
#ifdef _WIN32
    return (int64_t)GetCurrentProcessId();
#else
    return (int64_t)getpid();
#endif
}

/// Gets the hostname of the current machine.
///
/// @param hostname   Buffer to store the hostname.
/// @param size       Size of `hostname`.
void os_get_hostname(char *hostname, size_t size)
{
#ifdef HAVE_HDR_SYS_UTSNAME_H
    struct utsname vutsname;

    if(uname(&vutsname) < 0)
    {
        *hostname = '\0';
    }
    else
    {
        xstrncpy(hostname, vutsname.nodename, size);
    }
#elif defined(HOST_OS_WINDOWS)
    WCHAR host_utf16[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD host_wsize = sizeof(host_utf16) / sizeof(host_utf16[0]);

    if(GetComputerNameW(host_utf16, &host_wsize) == 0)
    {
        *hostname = '\0';
        DWORD err = GetLastError();
        EMSG2("GetComputerNameW failed: %d", err);
        return;
    }

    host_utf16[host_wsize] = '\0';
    char *host_utf8;
    int conversion_result = utf16_to_utf8(host_utf16, &host_utf8);

    if(conversion_result != 0)
    {
        EMSG2("utf16_to_utf8 failed: %d", conversion_result);
        return;
    }

    xstrncpy(hostname, host_utf8, size);
    xfree(host_utf8);
#else
    EMSG("os_get_hostname failed: missing uname()");
    *hostname = '\0';
#endif
}

/// Nvim layout subdirectory check flags
typedef enum
{
    kNLC_SYS = 1, ///< if set, do check $GKIDE_SYS_HOME
    kNLC_USR = 2, ///< if set, do check $GKIDE_USR_HOME
    kNLC_FLG = 4  ///< if set, check sequence: SYS->USR, otherwise, the reverse
} NvimLayoutCheck;

/// To get the user home directory: get the value of $GKIDE_USR_HOME
/// Here used this environment is because $HOME is used everywhere, so
/// that if change its value which will affect others, now, use another
/// value, if you need, then just change it!
///
/// For all:
/// - check $GKIDE_USR_HOME, if not set, then
/// - check $HOME, if set, then set $GKIDE_USR_HOME to $HOME
///   This also works with mounts and links.
///
/// For unix:
/// - go to that directory
/// - do os_dirname() to get the real name of that directory. Don't do this
///   for Windows, it will change the current directory for a drive.
bool init_gkide_usr_home(void)
{
    const char *std_home = NULL; // host system standard user home
#if 0
    std_home = os_getenv("HOME");
#ifdef HOST_OS_WINDOWS
    // Typically, $HOME is not defined on Windows, unless the user has
    // specifically defined it for Vim's sake. However, on Windows NT
    // platforms, $HOMEDRIVE and $HOMEPATH are automatically defined
    // for each user. Try constructing $HOME from these.
    if(std_home == NULL)
    {
        const char *homedrive = os_getenv("HOMEDRIVE");
        const char *homepath = os_getenv("HOMEPATH");

        if(homepath == NULL)
        {
            homepath = "\\";
        }

        if(homedrive != NULL && strlen(homedrive) + strlen(homepath) < MAXPATHL)
        {
            snprintf(os_buf, OS_BUF_SIZE, "%s%s", homedrive, homepath);

            if(os_buf[0] != NUL)
            {
                std_home = os_buf;
            }
        }
    }
#endif
#else
    size_t buf_len = MAXPATHL; // get host standard home directory

    if(kLibuvSuccess == get_os_home_dir(os_buf, &buf_len))
    {
        std_home = os_buf;
    }
#endif

    if(std_home == NULL)
    {
        // can not get host system user home, to be fixed
        TIME_MSG("EXIT(0): can not get host system user home");
        return false;
    }

    bool set_usr_home_env = false; // need to set $GKIDE_USR_HOME
    const char *usr_home = os_getenv(ENV_GKIDE_USR_HOME);

    if(usr_home == NULL)
    {
        set_usr_home_env = true;
    }

    if(usr_home && !path_is_absolute_path((uchar_kt *)usr_home))
    {
        // $GKIDE_USR_HOME should be set to absolute path
        INFO_MSG("ignore relative path of $GKIDE_USR_HOME: %s", usr_home);
        set_usr_home_env = true; // use the default value and reset it.
    }

    if(set_usr_home_env)
    {
        // Default value
#ifdef HOST_OS_WINDOWS
        snprintf((char *)NameBuff, MAXPATHL, "%s\\Documents\\gkide", std_home);
#else
        snprintf((char *)NameBuff, MAXPATHL, "%s/.gkide", std_home);
#endif
    }
    else
    {
        // $GKIDE_USR_HOME already set, check it
        // take care of '~', because afterwards, its meaning changed
        if(usr_home[0] == '~')
        {
            set_usr_home_env = true; // expend to absolute path
            strcpy((char *)NameBuff, std_home);
            size_t std_len = strlen((char *)NameBuff);

            if(usr_home[1] == OS_PATH_SEP_CHAR)
            {
                // "~/..." also copy the terminating NUL char
                memcpy((char *)NameBuff+std_len,
                       usr_home+1, strlen(usr_home));
            }
            else
            {
                if(usr_home[1] != NUL)
                {
                    // $GKIDE_USR_HOME has illegal value, for example: "~foo"
                    INFO_MSG("ignore illegal value of $GKIDE_USR_HOME: %s",
                             usr_home);
                }

                // just "~", have same effect as not set, use default
            #ifdef HOST_OS_WINDOWS
                snprintf((char *)NameBuff+std_len, MAXPATHL, "\\gkide");
            #else
                snprintf((char *)NameBuff+std_len, MAXPATHL, "/.gkide");
            #endif
            }
        }
        else
        {
            // a different gkide-usr-home from the default standard user home
            snprintf((char *)NameBuff, MAXPATHL, "%s", usr_home);
        }
    }

    // check if the home directory exist
    if(!os_path_exists((uchar_kt *)NameBuff))
    {
        char *failed_dir = NULL;
        INFO_MSG("try to create user home: %s", (char *)NameBuff);

        if(os_mkdir_recurse((char *)NameBuff, 0755, &failed_dir) != kLibuvSuccess)
        {
            INFO_MSG("EXIT(0): can not create user home: %s", failed_dir);
            return false;
        }
    }

    usr_home = (char *)NameBuff;

    if(usr_home[strlen(usr_home)-1] == OS_PATH_SEP_CHAR)
    {
        char *ptr = (char *)usr_home + strlen(usr_home) - 1;
        *ptr = NUL; // make sure no trailling path separator
    }

#if(defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS))
    // Change to the home directory and get the actual path.
    // This resolves links. Do not do it when we can not return.
    if(os_dirname((uchar_kt *)os_buf, MAXPATHL) == OK
       && os_chdir(os_buf) == kLibuvSuccess)
    {
        // change to home directory, resolves links.
        if(os_chdir(usr_home) == kLibuvSuccess
           && os_dirname(IObuff, IOSIZE) == OK)
        {
            usr_home = (char *)IObuff;
        }

        // go back
        if(os_chdir(os_buf) != kLibuvSuccess)
        {
            EMSG(_(e_prev_dir));
        }
    }
#endif

    if(gkide_usr_home)
    {
        // In case we are called a second time.
        xfree(gkide_usr_home);
        gkide_usr_home = NULL;
    }

    gkide_usr_home = (char *)ustrdup((uchar_kt *)usr_home);

    if(set_usr_home_env)
    {
        vim_setenv(ENV_GKIDE_USR_HOME, gkide_usr_home);
    }

    INFO_MSG("$GKIDE_USR_HOME=%s", gkide_usr_home);
    return true;
}

/// Call expand_env() and store the result in an allocated string.
/// This is not very memory efficient, this expects the result to
/// be freed again soon.
///
/// @param src String containing environment variables to expand
/// @see {expand_env}
uchar_kt *expand_env_save(uchar_kt *src)
{
    return expand_env_save_opt(src, false);
}

/// Similar to expand_env_save() but when @b one is true handle the string as
/// one file name, i.e. only expand "~" at the start.
///
/// @param src String containing environment variables to expand
/// @param one Should treat as only one file name
///
/// @see {expand_env}
uchar_kt *expand_env_save_opt(uchar_kt *src, bool one)
{
    uchar_kt *p = xmalloc(MAXPATHL);
    expand_env_esc(src, p, MAXPATHL, false, one, NULL);
    return p;
}

/// Expand environment variable with path name.
/// "~/" is also expanded, using $HOME. For Unix "~user/" is expanded.
/// Skips over "\ ", "\~" and "\$" (not for Win32 though).
/// If anything fails no expansion is done and dst equals src.
///
/// @param src        Input string e.g. "$HOME/vim.hlp"
/// @param dst[out]   Where to put the result
/// @param dstlen     Maximum length of the result
void expand_env(uchar_kt *src, uchar_kt *dst, int dstlen)
{
    expand_env_esc(src, dst, dstlen, false, false, NULL);
}

/// Expand environment variable with path name and escaping.
/// @see expand_env
///
/// @param[in]  srcp       Input string e.g. "$GKIDE_USR_HOME/help.nvim"
/// @param[out] dst        Where to put the result
/// @param[in]  dstlen     Maximum length of the result
/// @param[in]  esc        Escape spaces in expanded variables
/// @param[in]  one        **srcp** is a single filename
/// @param[in]  prefix     Start again after this (can be NULL)
void expand_env_esc(uchar_kt *restrict srcp,
                    uchar_kt *restrict dst,
                    int dstlen,
                    bool esc,
                    bool one,
                    uchar_kt *prefix)
{
    uchar_kt *tail;
    uchar_kt *var;
    bool copy_char;
    bool mustfree; // var was allocated, need to free it later
    bool at_start = true; // at start of a name
    int prefix_len = (prefix == NULL) ? 0 : (int)ustrlen(prefix);

    // should leave one char space for comma at the end, which is ','
    dstlen--; // now turn to index
    DEV_TRACE_MSG("srcp=%s", srcp);
    uchar_kt *src = skipwhite(srcp);

    while(*src && dstlen > 0)
    {
        // Skip over `=expr`.
        if(src[0] == '`' && src[1] == '=')
        {
            var = src;
            src += 2;
            // skip 'expr'
            (void)skip_expr(&src);

            if(*src == '`')
            {
                src++;
            }

            size_t len = (size_t)(src - var);

            if(len > (size_t)dstlen)
            {
                len = (size_t)dstlen;
            }

            memcpy((char *)dst, (char *)var, len);
            dst += len;
            dstlen -= (int)len;
            continue;
        }

        copy_char = true;

        if((*src == '$') || (*src == '~' && at_start))
        {
            mustfree = false;

            // The variable name is copied into dst temporarily, because it may
            // be a string in read-only memory and a NUL needs to be appended.
            if(*src != '~') // environment var
            {
                tail = src + 1; // begin of the env-var name
                var = dst;
                int c = dstlen - 1; // index for comma

            #if defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS)
                // Unix has ${var-name} type environment vars
                if(*tail == '{' && !is_id_char('{'))
                {
                    tail++; // ignore '{'

                    // c now is the string length count
                    while(c-- > 0 && *tail != NUL && *tail != '}')
                    {
                        *var++ = *tail++;
                    }
                }
                else
            #endif
                {
                    // $VarName type of environment vars
                    // c now is the string length count
                    while(c-- > 0 && *tail != NUL && is_id_char(*tail))
                    {
                        *var++ = *tail++;
                    }
                }

            #if defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS)
                // Verify that we have found the end of a Unix ${VAR} style variable
                if(src[1] == '{' && *tail != '}')
                {
                    var = NULL;
                }
                else
                {
                    if(src[1] == '{')
                    {
                        ++tail;
                    }
            #endif
                    // set the 'dst' last char to NUL
                    *var = NUL; // then get the env-var value
                    var = (uchar_kt *)vim_getenv((char *)dst);
                    mustfree = true; // var point to new allocate memory
                    DEV_TRACE_MSG("env=%s", dst);
                    DEV_TRACE_MSG("val=%s", var);

            #if defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS)
                }
            #endif
            }
            else if(src[1] == NUL // home directory, "~"
                    || vim_ispathsep(src[1]) // "~/" or "~\"
                    || ustrchr((uchar_kt *)" ,\t\n", src[1]) != NULL)
            {
                var = (uchar_kt *)gkide_usr_home; // ~
                tail = src + 1; // the left
            }
            else // user directory, like ~user etc.
            {
            #if defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS)
                // Copy ~user to dst[], so we can put a NUL after it.
                tail = src;
                var = dst;
                int c = dstlen - 1;

                while(c-- > 0
                      && *tail
                      && is_file_name_char(*tail)
                      && !vim_ispathsep(*tail))
                {
                    *var++ = *tail++;
                }

                *var = NUL;
                // Use os_get_user_directory() to get the user directory.
                // If this function fails, the shell is used to expand ~user.
                // This is slower and may fail if the shell does not support ~user
                // (old versions of /bin/sh).
                var = (uchar_kt *)os_get_user_directory((char *)dst + 1);
                mustfree = true;

                if(var == NULL)
                {
                    expand_st xpc;
                    ExpandInit(&xpc);
                    xpc.xp_context = EXPAND_FILES;
                    var = ExpandOne(&xpc, dst, NULL, WILD_ADD_SLASH | WILD_SILENT, WILD_EXPAND_FREE);
                    mustfree = true;
                }

            #else
                var = NULL; // cannot expand user's home directory, so don't try
                tail = (uchar_kt *)""; // for gcc
            #endif
            }

        #ifdef BACKSLASH_IN_FILENAME
            // If 'shellslash' is set change backslashes to forward slashes.
            // Can't use slash_adjust(), p_ssl may be set temporarily.
            if(p_ssl && var != NULL && ustrchr(var, '\\') != NULL)
            {
                uchar_kt *p = ustrdup(var);

                if(mustfree)
                {
                    xfree(var);
                }

                var = p;
                mustfree = true;
                forward_slash(var);
            }
        #endif

            // If "var" contains white space, escape it with a backslash.
            // Required for ":e ~/tt" when $HOME includes a space.
            if(esc && var != NULL && xstrpbrk(var, (uchar_kt *)" \t") != NULL)
            {
                uchar_kt *p = ustrdup_escape(var, (uchar_kt *)" \t");

                if(mustfree)
                {
                    xfree(var);
                }

                var = p;
                mustfree = true;
            }

            // copy the env-var value and the left to 'dst'
            // if var != NULL && *var != NUL, then it is new mallocate env-var value
            if(var != NULL && *var != NUL && (ustrlen(var) + ustrlen(tail) + 1 < (unsigned)dstlen))
            {
                ustrcpy(dst, var); // copy env-var value
                dstlen -= (int)ustrlen(var);
                int c = (int)ustrlen(var); // next beginning index for 'dst'

                // if var[] ends in a path separator and tail[] starts with it, skip a character
                if(*var != NUL && after_pathsep((char *)dst, (char *)dst + c)
                #if defined(BACKSLASH_IN_FILENAME)
                   && dst[-1] != ':'
                #endif
                   && vim_ispathsep(*tail))
                {
                    ++tail;
                }

                dst += c; // the next beginning index
                src = tail; // the left strings beginning
                copy_char = false;
            }

            if(mustfree)
            {
                xfree(var);
            }
        }

        if(copy_char)
        {
            // copy at least one char
            // Recognize the start of a new name, for '~'.
            // Don't do this when "one" is true, to avoid expanding "~" in
            // ":edit foo ~ foo".
            at_start = false;

            if(src[0] == '\\' && src[1] != NUL)
            {
                *dst++ = *src++;
                --dstlen;
            }
            else if((src[0] == ' ' || src[0] == ',') && !one)
            {
                at_start = true;
            }

            *dst++ = *src++;
            --dstlen;

            if(prefix != NULL
               && src - prefix_len >= srcp
               && ustrncmp(src - prefix_len, prefix, prefix_len) == 0)
            {
                at_start = true;
            }
        }
    }

    *dst = NUL;
}

/// Iterate over a delimited list.
///
/// @note Environment variables must not be modified during iteration.
///
/// @param[in]   delim Delimiter character.
/// @param[in]   val   Value of the environment variable to iterate over.
/// @param[in]   iter  Pointer used for iteration. Must be NULL on first
///                    iteration.
/// @param[out]  dir   Location where pointer to the start of the current
///                    directory name should be saved. May be set to NULL.
/// @param[out]  len   Location where current directory length should be saved.
///
/// @return Next iter argument value or NULL when iteration should stop.
const void *vim_env_iter(const char delim,
                         const char *const val,
                         const void *const iter,
                         const char **const dir,
                         size_t *const len)
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_NONNULL_ARG(2, 4, 5)
{
    const char *varval = (const char *) iter;

    if(varval == NULL)
    {
        varval = val;
    }

    *dir = varval;
    const char *const dirend = strchr(varval, delim);

    if(dirend == NULL)
    {
        *len = strlen(varval);
        return NULL;
    }
    else
    {
        *len = (size_t)(dirend - varval);
        return dirend + 1;
    }
}

/// Iterate over a delimited list in reverse order.
///
/// @note Environment variables must not be modified during iteration.
///
/// @param[in]   delim Delimiter character.
/// @param[in]   val   Value of the environment variable to iterate over.
/// @param[in]   iter  Pointer used for iteration. Must be NULL on first
///                    iteration.
/// @param[out]  dir   Location where pointer to the start of the current
///                    directory name should be saved. May be set to NULL.
/// @param[out]  len   Location where current directory length should be saved.
///
/// @return Next iter argument value or NULL when iteration should stop.
const void *vim_env_iter_rev(const char delim,
                             const char *const val,
                             const void *const iter,
                             const char **const dir,
                             size_t *const len)
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_NONNULL_ARG(2, 4, 5)
{
    const char *varend = (const char *) iter;

    if(varend == NULL)
    {
        varend = val + strlen(val) - 1;
    }

    const size_t varlen = (size_t)(varend - val) + 1;
    const char *const colon = xmemrchr(val, (uint8_t)delim, varlen);

    if(colon == NULL)
    {
        *len = varlen;
        *dir = val;
        return NULL;
    }
    else
    {
        *dir = colon + 1;
        *len = (size_t)(varend - colon);
        return colon - 1;
    }
}

/// nvim's version of getenv(), need to call xfree()
///
/// @param name Environment variable to expand
char *vim_getenv(const char *name)
{
    // init_path() should have been called before now.
    assert(get_vim_var_str(VV_PROGPATH)[0] != NUL);
    const char *env_val = os_getenv(name);

    if(env_val != NULL)
    {
        return xstrdup(env_val);
    }

    return NULL;
}

/// Replace user home directory by ~ in each space or comma separated
/// file name in @b src, if anything fails (except when out of space)
/// @b dst equals @b src.
///
/// @param buf      When not NULL, check for help files
/// @param src      Input file name, to check and do replace
/// @param dst      Where to put the result
/// @param dstlen   Maximum length of the result
void usr_home_replace(const filebuf_st *const buf,
                      const uchar_kt *src,
                      uchar_kt *dst,
                      size_t dstlen)
{
    if(src == NULL)
    {
        *dst = NUL; // make sure c-string
        return;
    }

    // If the file is a help file, remove the path completely.
    if(buf != NULL && buf->b_help)
    {
        xstrncpy((char *)dst, (char *)path_tail(src), dstlen);
        return;
    }

    assert(gkide_usr_home != NULL);
    const size_t usr_home_len = ustrlen(gkide_usr_home);

    while(src[0] && dstlen > 0)
    {
        // Here we are at the beginning of a file name.
        //
        // check to see if the beginning of the file name matches
        // $GKIDE_USR_HOME directory and make sure that there is a '/'
        // after the match, so that if e.g. the file is "/home/pieter/bla",
        // and the home directory is "/home/piet", the file does not end up
        // as "~er/bla"
        if(fnamencmp(src, gkide_usr_home, usr_home_len) == 0
           && (vim_ispathsep(src[usr_home_len])
               || src[usr_home_len] == ','
               || src[usr_home_len] == ' '
               || src[usr_home_len] == NUL))
        {
            src += usr_home_len;

            if(--dstlen > 0)
            {
                *dst++ = '~';
            }

            // not ending in path separator, add it
            if(!vim_ispathsep(src[0]) && --dstlen > 0)
            {
                *dst++ = OS_PATH_SEP_CHAR;
            }
        }

        // the rest of part to copy
        while(*src && *src != ',' && *src != ' ' && --dstlen > 0)
        {
            *dst++ = *src++;
        }

        // copy separator, point to next item
        while(*src && (*src == ' ' || *src == ','))
        {
            *dst++ = *src++;
        }
    }

    // make sure c-string
    *dst = NUL;
}

/// Like usr_home_replace(), store the replaced string in allocated memory.
///
/// @param buf  When not NULL, check for help files
/// @param src  Input file name
///
/// @return allocated c-string, which has the user home replaced
uchar_kt *usr_home_replace_malloc(filebuf_st *buf, uchar_kt *src)
FUNC_ATTR_NONNULL_RETURN
{
    if(NULL == src)
    {
        return NULL;
    }

    size_t old_len = ustrlen(src);
    uchar_kt *dst = xmalloc(old_len);
    usr_home_replace(buf, src, dst, old_len);

    size_t new_len = ustrlen(dst);

    if(new_len < old_len && NUL == dst[new_len])
    {
        // need to free the extra not used space
        void *ptr = dst + new_len + 1;
        xfree(ptr);
    }

    return dst;
}

/// Our portable version of setenv().
void vim_setenv(const char *name, const char *val)
{
    (void)os_setenv(name, val, 1); // overwrite if exist
}

/// Function given to ExpandGeneric() to obtain an environment variable name.
uchar_kt *get_env_name(expand_st *FUNC_ARGS_UNUSED_MATCH(xp), int idx)
{
#define ENVNAMELEN  100
    // this static buffer is needed to avoid a memory leak in ExpandGeneric
    static uchar_kt name[ENVNAMELEN];
    assert(idx >= 0);
    char *envname = os_getenvname_at_index((size_t)idx);

    if(envname)
    {
        ustrlcpy(name, envname, ENVNAMELEN);
        xfree(envname);
        return name;
    }

    return NULL;
}

/// Appends the head of @b fname to $PATH and sets it in the environment.
///
/// @param fname  Full path whose parent directory will be appended to $PATH.
///
/// @return true if @b path was appended-to $PATH, otherwise false.
bool os_setenv_append_path(const char *fname)
FUNC_ATTR_NONNULL_ALL
{
#ifdef HOST_OS_WINDOWS
    /// 8191 plus NUL is considered the practical maximum.
    #define MAX_ENVPATHLEN  8192
#else
    /// No prescribed maximum on unix.
    #define MAX_ENVPATHLEN  INT_MAX
#endif

    if(!path_is_absolute_path((uchar_kt *)fname))
    {
        EMSG2(_(e_intern2), "os_setenv_append_path()");
        return false;
    }

    const char *tail = (char *)path_tail_with_sep((uchar_kt *)fname);
    size_t dirlen = (size_t)(tail - fname);

    assert(tail >= fname &&dirlen + 1 < sizeof(os_buf));

    xstrncpy(os_buf, fname, dirlen + 1);

    const char *path = os_getenv("PATH");
    const size_t pathlen = path ? strlen(path) : 0;
    const size_t newlen = pathlen + dirlen + 2;

    if(newlen < MAX_ENVPATHLEN)
    {
        char *temp = xmalloc(newlen);

        if(pathlen == 0)
        {
            temp[0] = NUL;
        }
        else
        {
            xstrncpy(temp, path, newlen);
            xstrncat(temp, ENV_SEPSTR, newlen);
        }

        xstrncat(temp, os_buf, newlen);
        os_setenv("PATH", temp, 1);
        xfree(temp);
        return true;
    }

    return false;
}

/// Returns true if the terminal can be assumed to silently ignore unknown
/// control codes.
bool os_term_is_nice(void)
{
#if defined(__APPLE__) || defined(HOST_OS_WINDOWS)
    return true;
#else
    const char *vte_version = os_getenv("VTE_VERSION");

    if((vte_version && atoi(vte_version) >= 3900)
       || os_getenv("KONSOLE_PROFILE_NAME")
       || os_getenv("KONSOLE_DBUS_SESSION"))
    {
        return true;
    }

    const char *termprg = os_getenv("TERM_PROGRAM");

    if(termprg && xstriequal(termprg, "iTerm.app"))
    {
        return true;
    }

    const char *term = os_getenv("TERM");

    if(term && strncmp(term, "rxvt", 4) == 0)
    {
        return true;
    }

    return false;
#endif
}

/// Returns true if `sh` looks like it resolves to "cmd.exe".
bool os_shell_is_cmdexe(const char *sh)
FUNC_ATTR_NONNULL_ALL
{
    if(*sh == NUL)
    {
        return false;
    }

    if(xstriequal(sh, "$COMSPEC"))
    {
        const char *comspec = os_getenv("COMSPEC");
        return xstriequal("cmd.exe", (char *)path_tail((uchar_kt *)comspec));
    }

    if(xstriequal(sh, "cmd.exe") || xstriequal(sh, "cmd"))
    {
        return true;
    }

    return xstriequal("cmd.exe", (char *)path_tail((uchar_kt *)sh));
}
