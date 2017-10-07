/// @file nvim/os/env.c
///
/// Environment inspection

#include <assert.h>
#include <uv.h>

#include "nvim/vim.h"
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

#include "config.h"

#ifdef HOST_OS_WINDOWS
    #include "nvim/mbyte.h"  // for utf8_to_utf16, utf16_to_utf8
#endif

#ifdef HAVE__NSGETENVIRON
    #include <crt_externs.h>
#endif

#ifdef HAVE_HDR_SYS_UTSNAME_H
    #include <sys/utsname.h>
#endif

/// Like getenv(), but returns NULL if the variable is empty.
const char *os_getenv(const char *name) FUNC_ATTR_NONNULL_ALL
{
    const char *e = getenv(name);
    return (e == NULL || *e == NUL) ? NULL : e;
}

/// Returns `true` if the environment variable, `name`, has been defined, even if empty.
bool os_env_exists(const char *name) FUNC_ATTR_NONNULL_ALL
{
    return getenv(name) != NULL;
}

int os_setenv(const char *name, const char *value, int overwrite) FUNC_ATTR_NONNULL_ALL
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
/// For systems where unsetenv() is not available the value will be set as an
/// empty string
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

    char *name = (char *)vim_strnsave((char_u *)str, namesize);
    return name;
}

/// Get the process ID of the Neovim process.
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
        xstrlcpy(hostname, vutsname.nodename, size);
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

    xstrlcpy(hostname, host_utf8, size);
    xfree(host_utf8);
#else
    EMSG("os_get_hostname failed: missing uname()");
    *hostname = '\0';
#endif
}

/// home directory
static char_u *homedir = NULL;

char *gkide_sys_home = NULL; /// gkide system  home directory, runtime fixed
char *gkide_usr_home = NULL; /// gkide user    home directory, runtime fixed
char *gkide_dyn_home = NULL; /// gkide dynamic home directory, runtime can changed

/// To get the real user home directory: get value of $GKIDE_USR_HOME
/// Here used this environment is because $HOME is used everywhere, so that we can changed its
/// value which will affect others, now, use another one, if you need, then just do it!
///
/// For all:
/// - check $GKIDE_USR_HOME, if not set, then
/// - check $HOME, if set, then reset $GKIDE_USR_HOME to $HOME
///   This also works with mounts and links.
///
/// For unix:
/// - go to that directory
/// - do os_dirname() to get the real name of that directory.
///   Don't do this for Windows, it will change the current directory for a drive.
///
/// For windows:
/// - if $GKIDE_USR_HOME and $HOME both not set, then check $HOMEDRIVE and $HOMEPATH for
///   Windows NT platforms, then reset $GKIDE_USR_HOME
void init_gkide_usr_home(void)
{
    if(gkide_usr_home)
    {
        // In case we are called a second time.
        xfree(gkide_usr_home);
        gkide_usr_home = NULL;
    }

    bool usr_home_reset = false; // need to reset gkide usr home env ?
    const char *usr_home = os_getenv(ENV_GKIDE_USR_HOME);

    if(usr_home == NULL)
    {
        // if GKIDE_USR_HOME not set, then check HOME
        usr_home = os_getenv("HOME");
        usr_home_reset = true;
    }

    #ifdef HOST_OS_WINDOWS
    // Typically, $HOME is not defined on Windows, unless the user has specifically defined it
    // for Vim's sake. However, on Windows NT platforms, $HOMEDRIVE and $HOMEPATH are automatically
    // defined for each user. Try constructing $HOME from these.
    if(usr_home == NULL)
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
                usr_home = os_buf;
            }
        }
    }
    #endif

    if(usr_home == NULL)
    {
        TIME_MSG("GKIDE_USR_HOME is NULL, this should be fixed");
        return;
    }

    if(usr_home_reset)
    {
        // make '.gkide' directory first, then reset gkide usr home env
        snprintf(NameBuff, MAXPATHL, "%s" _PATHSEPSTR ".gkide", usr_home);
    }
    else
    {
        // gkide usr home env already set, check if it exist, if not, try to create it.
        snprintf(NameBuff, MAXPATHL, "%s", usr_home);
    }

    if(os_path_exists((char_u *)NameBuff))
    {
        // already exists, do not need to create
        usr_home = (char *)NameBuff;
    }
    else
    {
        TIME_MSG("try to create $GKIDE_USR_HOME directory");
        int ret = os_mkdir((char *)NameBuff, 0755);
        if(ret == 0)
        {
            // if can not created it, then just ignore
            usr_home = (char *)NameBuff;
        }
    }

    #if(defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS))
    // Change to the home directory and get the actual path.
    // This resolves links. Don't do it when we can't return.
    if(os_dirname((char_u *)os_buf, MAXPATHL) == OK && os_chdir(os_buf) == 0)
    {
        // change to home directory
        if(!os_chdir(usr_home) && os_dirname(IObuff, IOSIZE) == OK)
        {
            usr_home = (char *)IObuff;
        }

        // go back
        if(os_chdir(os_buf) != 0)
        {
            EMSG(_(e_prev_dir));
        }
    }
    #endif
    gkide_usr_home = vim_strsave((char_u *)usr_home);

    if(usr_home_reset)
    {
        vim_setenv(ENV_GKIDE_USR_HOME, gkide_usr_home);
    }

    INFO_MSG("GKIDE_USR_HOME => %s", gkide_usr_home);
}

void init_gkide_dyn_home(void)
{}

/// todo: remove this func, no use $HOME any more
void init_homedir(void)
{
    xfree(homedir);
    homedir = NULL; // In case we are called a second time.

    const char *var = os_getenv("HOME");

    #ifdef HOST_OS_WINDOWS
    // Typically, $HOME is not defined on Windows, unless the user has specifically defined it
    // for Vim's sake. However, on Windows NT platforms, $HOMEDRIVE and $HOMEPATH are automatically
    // defined for each user. Try constructing $HOME from these.
    if(var == NULL)
    {
        const char *homedrive = os_getenv("HOMEDRIVE");
        const char *homepath = os_getenv("HOMEPATH");

        if(homepath == NULL)
        {
            homepath = "\\";
        }

        if(homedrive != NULL && strlen(homedrive) + strlen(homepath) < MAXPATHL)
        {
            snprintf(os_buf, MAXPATHL, "%s%s", homedrive, homepath);

            if(os_buf[0] != NUL)
            {
                var = os_buf;
                vim_setenv("HOME", os_buf);
            }
        }
    }
    #endif

    if(var != NULL)
    {
        #if(defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS))
        // Change to the home directory and get the actual path. This resolves links.
        // Don't do it when we can't return.
        if(os_dirname((char_u *)os_buf, MAXPATHL) == OK && os_chdir(os_buf) == 0)
        {
            // change to home directory
            if(!os_chdir(var) && os_dirname(IObuff, IOSIZE) == OK)
            {
                var = (char *)IObuff;
            }

            // go back
            if(os_chdir(os_buf) != 0)
            {
                EMSG(_(e_prev_dir));
            }
        }
        #endif
        homedir = vim_strsave((char_u *)var);
    }
}

#if defined(EXITFREE)
void free_homedir(void)
{
    xfree(homedir);
}
#endif

/// Call expand_env() and store the result in an allocated string.
/// This is not very memory efficient, this expects the result to be freed again soon.
///
/// @param src String containing environment variables to expand
/// @see {expand_env}
char_u *expand_env_save(char_u *src)
{
    return expand_env_save_opt(src, false);
}

/// Similar to expand_env_save() but when "one" is `true` handle the string as
/// one file name, i.e. only expand "~" at the start.
///
/// @param src String containing environment variables to expand
/// @param one Should treat as only one file name
///
/// @see {expand_env}
char_u *expand_env_save_opt(char_u *src, bool one)
{
    char_u *p = xmalloc(MAXPATHL);
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
void expand_env(char_u *src, char_u *dst, int dstlen)
{
    expand_env_esc(src, dst, dstlen, false, false, NULL);
}

/// Expand environment variable with path name and escaping.
/// @see expand_env
///
/// @param srcp       Input string e.g. "$HOME/vim.hlp"
/// @param dst[out]   Where to put the result
/// @param dstlen     Maximum length of the result
/// @param esc        Escape spaces in expanded variables
/// @param one        `srcp` is a single filename
/// @param prefix     Start again after this (can be NULL)
void expand_env_esc(char_u *restrict srcp,
                    char_u *restrict dst,
                    int dstlen,
                    bool esc,
                    bool one,
                    char_u *prefix)
{
    char_u *tail;
    char_u *var;
    bool copy_char;
    bool mustfree;         // var was allocated, need to free it later
    bool at_start = true;  // at start of a name
    int prefix_len = (prefix == NULL) ? 0 : (int)STRLEN(prefix);
    char_u *src = skipwhite(srcp);
    dstlen--;  // leave one char space for "\,"

    while(*src && dstlen > 0)
    {
        // Skip over `=expr`.
        if(src[0] == '`' && src[1] == '=')
        {
            var = src;
            src += 2;
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
            if(*src != '~')
            {
                // environment var
                tail = src + 1;
                var = dst;
                int c = dstlen - 1;
#if defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS)

                // Unix has ${var-name} type environment vars
                if(*tail == '{' && !vim_isIDc('{'))
                {
                    tail++; // ignore '{'

                    while(c-- > 0 && *tail != NUL && *tail != '}')
                    {
                        *var++ = *tail++;
                    }
                }
                else
#endif
                {
                    while (c-- > 0 && *tail != NUL && vim_isIDc(*tail))
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
                    *var = NUL;
                    var = (char_u *)vim_getenv((char *)dst);
                    mustfree = true;
#if defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS)
                }

#endif
            }
            else if(src[1] == NUL // home directory
                    || vim_ispathsep(src[1])
                    || vim_strchr((char_u *)" ,\t\n", src[1]) != NULL)
            {
                var = homedir;
                tail = src + 1;
            }
            else
            {
                // user directory
#if defined(HOST_OS_LINUX) || defined(HOST_OS_MACOS)
                // Copy ~user to dst[], so we can put a NUL after it.
                tail = src;
                var = dst;
                int c = dstlen - 1;

                while(c-- > 0
                        && *tail
                        && vim_isfilec(*tail)
                        && !vim_ispathsep(*tail))
                {
                    *var++ = *tail++;
                }

                *var = NUL;
                // Use os_get_user_directory() to get the user directory.
                // If this function fails, the shell is used to expand ~user.
                // This is slower and may fail if the shell does not support ~user
                // (old versions of /bin/sh).
                var = (char_u *)os_get_user_directory((char *)dst + 1);
                mustfree = true;

                if(var == NULL)
                {
                    expand_T xpc;
                    ExpandInit(&xpc);
                    xpc.xp_context = EXPAND_FILES;
                    var = ExpandOne(&xpc,
                                    dst,
                                    NULL,
                                    WILD_ADD_SLASH|WILD_SILENT,
                                    WILD_EXPAND_FREE);
                    mustfree = true;
                }

#else
                // cannot expand user's home directory, so don't try
                var = NULL;
                tail = (char_u *)"";  // for gcc
#endif
            }

#ifdef BACKSLASH_IN_FILENAME

            // If 'shellslash' is set change backslashes to forward slashes.
            // Can't use slash_adjust(), p_ssl may be set temporarily.
            if(p_ssl && var != NULL && vim_strchr(var, '\\') != NULL)
            {
                char_u  *p = vim_strsave(var);

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
            if(esc && var != NULL && vim_strpbrk(var, (char_u *)" \t") != NULL)
            {
                char_u  *p = vim_strsave_escaped(var, (char_u *)" \t");

                if(mustfree)
                {
                    xfree(var);
                }

                var = p;
                mustfree = true;
            }

            if(var != NULL && *var != NUL
                    && (STRLEN(var) + STRLEN(tail) + 1 < (unsigned)dstlen))
            {
                STRCPY(dst, var);
                dstlen -= (int)STRLEN(var);
                int c = (int)STRLEN(var);

                // if var[] ends in a path separator and tail[] starts with it, skip a character
                if(*var != NUL && after_pathsep((char *)dst, (char *)dst + c)
#if defined(BACKSLASH_IN_FILENAME)
                        && dst[-1] != ':'
#endif
                        && vim_ispathsep(*tail))
                {
                    ++tail;
                }

                dst += c;
                src = tail;
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

            if(prefix != NULL && src - prefix_len >= srcp
                    && STRNCMP(src - prefix_len, prefix, prefix_len) == 0)
            {
                at_start = true;
            }
        }
    }

    *dst = NUL;
}

/// If `dirname + "/"` precedes `pend` in the path, return the pointer to
/// `dirname + "/" + pend`.  Otherwise return `pend`.
///
/// Examples (path = /usr/local/share/nvim/runtime/doc/help.txt):
///
///   pend    = help.txt
///   dirname = doc
///   -> doc/help.txt
///
///   pend    = doc/help.txt
///   dirname = runtime
///   -> runtime/doc/help.txt
///
///   pend    = runtime/doc/help.txt
///   dirname = vim74
///   -> runtime/doc/help.txt
///
/// @param path    Path to a file
/// @param pend    A suffix of the path
/// @param dirname The immediate path fragment before the pend
///
/// @return The new pend including dirname or just pend
static char *remove_tail(char *path, char *pend, char *dirname)
{
    size_t len = STRLEN(dirname);
    char *new_tail = pend - len - 1;

    if(new_tail >= path
            && fnamencmp((char_u *)new_tail, (char_u *)dirname, len) == 0
            && (new_tail == path || after_pathsep(path, new_tail)))
    {
        return new_tail;
    }

    return pend;
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
FUNC_ATTR_NONNULL_ARG(2, 4, 5) FUNC_ATTR_WARN_UNUSED_RESULT
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
        *len = (size_t) (dirend - varval);
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
FUNC_ATTR_NONNULL_ARG(2, 4, 5) FUNC_ATTR_WARN_UNUSED_RESULT
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
        *len = (size_t) (varend - colon);
        return colon - 1;
    }
}

/// nvim's version of getenv().
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

/// Replace home directory by **~** in each space or comma separated file name in **src**
/// If anything fails (except when out of space) dst equals src.
///
/// @param buf    When not NULL, check for help files
/// @param src    Input file name
/// @param dst    Where to put the result
/// @param dstlen Maximum length of the result
/// @param one    If true, only replace one file name, including spaces and commas in the file name
void home_replace(const buf_T *const buf, const char_u *src, char_u *dst, size_t dstlen, bool one)
{
    size_t dirlen = 0, envlen = 0;
    size_t len;

    if(src == NULL)
    {
        *dst = NUL;
        return;
    }

    // If the file is a help file, remove the path completely.
    if(buf != NULL && buf->b_help)
    {
        xstrlcpy((char *)dst, (char *)path_tail(src), dstlen);
        return;
    }

    // We check both the value of the $HOME environment variable and the "real" home directory.
    if(homedir != NULL)
    {
        dirlen = STRLEN(homedir);
    }

    char_u *homedir_env = (char_u *)os_getenv("HOME");
    bool must_free = false;

    if(homedir_env != NULL && vim_strchr(homedir_env, '~') != NULL)
    {
        must_free = true;
        size_t usedlen = 0;
        size_t flen = STRLEN(homedir_env);
        char_u *fbuf = NULL;
        (void)modify_fname((char_u *)":p", &usedlen, &homedir_env, &fbuf, &flen);
        flen = STRLEN(homedir_env);

        // Remove the trailing / that is added to a directory.
        if(flen > 0 && vim_ispathsep(homedir_env[flen - 1]))
        {
            homedir_env[flen - 1] = NUL;
        }
    }

    if(homedir_env != NULL)
    {
        envlen = STRLEN(homedir_env);
    }

    if(!one)
    {
        src = skipwhite(src);
    }

    while(*src && dstlen > 0)
    {
        // Here we are at the beginning of a file name.
        // First, check to see if the beginning of the file name matches
        // $HOME or the "real" home directory. Check that there is a '/'
        // after the match (so that if e.g. the file is "/home/pieter/bla",
        // and the home directory is "/home/piet", the file does not end up
        // as "~er/bla" (which would seem to indicate the file "bla" in user
        // er's home directory)).
        char_u *p = homedir;
        len = dirlen;

        for(;;)
        {
            if(len && fnamencmp(src, p, len) == 0
                    && (vim_ispathsep(src[len])
                        || (!one && (src[len] == ',' || src[len] == ' '))
                        || src[len] == NUL))
            {
                src += len;

                if(--dstlen > 0)
                {
                    *dst++ = '~';
                }

                // If it's just the home directory, add  "/"
                if(!vim_ispathsep(src[0]) && --dstlen > 0)
                {
                    *dst++ = '/';
                }

                break;
            }

            if(p == homedir_env)
            {
                break;
            }

            p = homedir_env;
            len = envlen;
        }

        // if (!one) skip to separator: space or comma
        while(*src && (one || (*src != ',' && *src != ' ')) && --dstlen > 0)
        {
            *dst++ = *src++;
        }

        // skip separator
        while((*src == ' ' || *src == ',') && --dstlen > 0)
        {
            *dst++ = *src++;
        }
    }

    // if (dstlen == 0) out of space, what to do?
    *dst = NUL;

    if(must_free)
    {
        xfree(homedir_env);
    }
}

/// Like home_replace, store the replaced string in allocated memory.
///
/// @param buf When not NULL, check for help files
/// @param src Input file name
char_u *home_replace_save(buf_T *buf, char_u *src) FUNC_ATTR_NONNULL_RET
{
    // space for "~/" and trailing NUL
    size_t len = 3;

    // just in case
    if(src != NULL)
    {
        len += STRLEN(src);
    }

    char_u *dst = xmalloc(len);
    home_replace(buf, src, dst, len, true);

    return dst;
}

/// Our portable version of setenv().
void vim_setenv(const char *name, const char *val)
{
    (void)os_setenv(name, val, 1); // overwrite if exist
}

/// Function given to ExpandGeneric() to obtain an environment variable name.
char_u *get_env_name(expand_T *xp, int idx)
{
#define ENVNAMELEN  100
    // this static buffer is needed to avoid a memory leak in ExpandGeneric
    static char_u name[ENVNAMELEN];
    assert(idx >= 0);
    char *envname = os_getenvname_at_index((size_t)idx);

    if(envname)
    {
        STRLCPY(name, envname, ENVNAMELEN);
        xfree(envname);
        return name;
    }

    return NULL;
}

/// Appends the head of `fname` to $PATH and sets it in the environment.
///
/// @param fname  Full path whose parent directory will be appended to $PATH.
///
/// @return true if `path` was appended-to
bool os_setenv_append_path(const char *fname) FUNC_ATTR_NONNULL_ALL
{
#ifdef HOST_OS_WINDOWS
#   define MAX_ENVPATHLEN  8192     ///< 8191 plus NUL is considered the practical maximum.
#else
#   define MAX_ENVPATHLEN  INT_MAX  ///< No prescribed maximum on unix.
#endif

    if(!path_is_absolute_path((char_u *)fname))
    {
        EMSG2(_(e_intern2), "os_setenv_append_path()");
        return false;
    }

    const char *tail = (char *)path_tail_with_sep((char_u *)fname);
    size_t dirlen = (size_t)(tail - fname);
    assert(tail >= fname &&dirlen + 1 < sizeof(os_buf));
    xstrlcpy(os_buf, fname, dirlen + 1);

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
            xstrlcpy(temp, path, newlen);
            xstrlcat(temp, ENV_SEPSTR, newlen);
        }

        xstrlcat(temp, os_buf, newlen);
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

    if(termprg && striequal(termprg, "iTerm.app"))
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
bool os_shell_is_cmdexe(const char *sh) FUNC_ATTR_NONNULL_ALL
{
    if(*sh == NUL)
    {
        return false;
    }

    if(striequal(sh, "$COMSPEC"))
    {
        const char *comspec = os_getenv("COMSPEC");
        return striequal("cmd.exe", (char *)path_tail((char_u *)comspec));
    }

    if(striequal(sh, "cmd.exe") || striequal(sh, "cmd"))
    {
        return true;
    }

    return striequal("cmd.exe", (char *)path_tail((char_u *)sh));
}
