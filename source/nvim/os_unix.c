/// @file nvim/os_unix.c
///
/// code for all flavors of Unix (BSD, SYSV, SVR4, POSIX, ...)
///
/// A lot of this file was originally written by Juergen Weigert and later
/// changed beyond recognition.

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "nvim/api/private/handle.h"
#include "nvim/nvim.h"
#include "nvim/ascii.h"
#include "nvim/os_unix.h"
#include "nvim/buffer.h"
#include "nvim/charset.h"
#include "nvim/eval.h"
#include "nvim/ex_cmds.h"
#include "nvim/fileio.h"
#include "nvim/getchar.h"
#include "nvim/main.h"
#include "nvim/mbyte.h"
#include "nvim/memline.h"
#include "nvim/memory.h"
#include "nvim/message.h"
#include "nvim/misc1.h"
#include "nvim/mouse.h"
#include "nvim/garray.h"
#include "nvim/path.h"
#include "nvim/screen.h"
#include "nvim/strings.h"
#include "nvim/syntax.h"
#include "nvim/ui.h"
#include "nvim/types.h"
#include "nvim/os/os.h"
#include "nvim/os/time.h"
#include "nvim/os/input.h"
#include "nvim/os/shell.h"
#include "nvim/os/signal.h"
#include "nvim/msgpack/helpers.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "os_unix.c.generated.h"
#endif

void mch_exit(int r)
FUNC_ATTR_NORETURN
{
    exiting = true;

    ui_builtin_stop();
    ui_flush();
    ml_close_all(true); // remove all memfiles

    event_teardown();

    // normalize stream (#2598)
    stream_set_blocking(input_global_fd(), true);

#ifdef EXITFREE
    free_all_mem();
#endif

    exit(r);
}

#define SHELL_SPECIAL (uchar_kt *)"\t \"&'$;<>()\\|"

/// Does wildcard pattern matching using the shell.
///
/// @param num_pat
/// is the number of input patterns.
///
/// @param pat
/// is an array of pointers to input patterns.
///
/// @param[out] num_file
/// is pointer to number of matched file names.
/// Set to the number of pointers in *file.
///
/// @param[out] file
/// is pointer to array of pointers to matched file names.
/// Memory pointed to by the initial value of *file will not be freed.
/// Set to NULL if FAIL is returned. Otherwise points to allocated memory.
/// @param flags
/// is a combination of EW_* flags used in expand_wildcards().
/// If matching fails but EW_NOTFOUND is set in flags or there
/// are no wildcards, the patterns from pat are copied into *file.
///
/// @returns
/// OK for success or FAIL for error.
int mch_expand_wildcards(int num_pat,
                         uchar_kt **pat,
                         int *num_file,
                         uchar_kt ***file,
                         int flags)
FUNC_ATTR_NONNULL_ARG(3)
FUNC_ATTR_NONNULL_ARG(4)
{
    int i;
    size_t len;
    uchar_kt *p;
    bool dir;
    uchar_kt *extra_shell_arg = NULL;
    shellopt_st shellopts = kShellOptExpand | kShellOptSilent;
    int j;
    uchar_kt *tempname;
    uchar_kt *command;
    FILE *fd;
    uchar_kt *buffer;

#define STYLE_ECHO      0  // use "echo", the default
#define STYLE_GLOB      1  // use "glob", for csh
#define STYLE_VIMGLOB   2  // use "vimglob", for Posix sh
#define STYLE_PRINT     3  // use "print -N", for zsh
#define STYLE_BT        4  // `cmd` expansion, execute the pattern directly

    int shell_style = STYLE_ECHO;
    int check_spaces;
    static bool did_find_nul = false;
    bool ampersent = false;

    // vimglob() function to define for Posix shell
    static char *sh_vimglob_func =
    "vimglob() { while [ $# -ge 1 ]; do echo \"$1\"; shift; done }; vimglob >";

    bool is_fish_shell =
    #if defined(UNIX)
        ustrncmp(invocation_path_tail(p_sh, NULL), "fish", 4) == 0;
    #else
        false;
    #endif

    *num_file = 0; // default: no files found
    *file = NULL;

    // If there are no wildcards, just copy the names to allocated memory.
    // Saves a lot of time, because we don't have to start a new shell.
    if(!have_wildcard(num_pat, pat))
    {
        save_patterns(num_pat, pat, num_file, file);
        return OK;
    }

    // Don't allow any shell command in the sandbox.
    if(sandbox != 0 && check_secure())
    {
        return FAIL;
    }

    // Don't allow the use of backticks in secure and restricted mode.
    if(secure || restricted)
    {
        for(i = 0; i < num_pat; i++)
        {
            if(ustrchr(pat[i], '`') != NULL
               && (check_restricted() || check_secure()))
            {
                return FAIL;
            }
        }
    }

    // get a name for the temp file
    if((tempname = vim_tempname()) == NULL)
    {
        EMSG(_(e_notmp));
        return FAIL;
    }

    // Let the shell expand the patterns and write the result
    // into the temp file.
    //
    // STYLE_BT: NL separated
    //     If expanding 'cmd' execute it directly.
    //
    // STYLE_GLOB: NUL separated
    //     If we use *csh, "glob" will work better than "echo".
    //
    // STYLE_PRINT: NL or NUL separated
    //     If we use *zsh, "print -N" will work better than "glob".
    //
    // STYLE_VIMGLOB: NL separated
    //     If we use *sh*, we define "vimglob()".
    //
    // STYLE_ECHO: space separated.
    //     A shell we don't know, stay safe and use "echo".
    if(num_pat == 1 && *pat[0] == '`'
       && (len = ustrlen(pat[0])) > 2
       && *(pat[0] + len - 1) == '`')
    {
        shell_style = STYLE_BT;
    }
    else if((len = ustrlen(p_sh)) >= 3)
    {
        if(ustrcmp(p_sh + len - 3, "csh") == 0)
        {
            shell_style = STYLE_GLOB;
        }
        else if(ustrcmp(p_sh + len - 3, "zsh") == 0)
        {
            shell_style = STYLE_PRINT;
        }
    }

    if(shell_style == STYLE_ECHO
       && strstr((char *)path_tail(p_sh), "sh") != NULL)
    {
        shell_style = STYLE_VIMGLOB;
    }

    // Compute the length of the command.
    // We need 2 extra bytes: for the optional '&' and for the NUL.
    // Worst case: "unset nonomatch; print -N >" plus two is 29
    len = ustrlen(tempname) + 29;

    if(shell_style == STYLE_VIMGLOB)
    {
        len += ustrlen(sh_vimglob_func);
    }

    for(i = 0; i < num_pat; i++)
    {
        // Count the length of the patterns in the
        // same way as they are put in "command" below.
        len++; // add space

        for(j = 0; pat[i][j] != NUL; j++)
        {
            if(ustrchr(SHELL_SPECIAL, pat[i][j]) != NULL)
            {
                len++; // may add a backslash
            }

            len++;
        }
    }

    if(is_fish_shell)
    {
        len += sizeof("egin;"" end") - 1;
    }

    command = xmalloc(len);

    // Build the shell command:
    // - Set $nonomatch depending on EW_NOTFOUND
    //   (hopefully the shell recognizes this).
    // - Add the shell command to print the expanded names.
    // - Add the temp file name.
    // - Add the file name patterns.
    if(shell_style == STYLE_BT)
    {
        // change `command; command& ` to (command; command )
        if(is_fish_shell)
        {
            ustrcpy(command, "begin; ");
        }
        else
        {
            ustrcpy(command, "(");
        }

        ustrcat(command, pat[0] + 1); // exclude first backtick
        p = command + ustrlen(command) - 1;

        if(is_fish_shell)
        {
            *p-- = ';';
            ustrcat(command, " end");
        }
        else
        {
            *p-- = ')'; // remove last backtick
        }

        while(p > command && ascii_iswhite(*p))
        {
            p--;
        }

        if(*p == '&') // remove trailing '&'
        {
            ampersent = true;
            *p = ' ';
        }

        ustrcat(command, ">");
    }
    else
    {
        if(flags & EW_NOTFOUND)
        {
            ustrcpy(command, "set nonomatch; ");
        }
        else
        {
            ustrcpy(command, "unset nonomatch; ");
        }

        if(shell_style == STYLE_GLOB)
        {
            ustrcat(command, "glob >");
        }
        else if(shell_style == STYLE_PRINT)
        {
            ustrcat(command, "print -N >");
        }
        else if(shell_style == STYLE_VIMGLOB)
        {
            ustrcat(command, sh_vimglob_func);
        }
        else
        {
            ustrcat(command, "echo >");
        }
    }

    ustrcat(command, tempname);

    if(shell_style != STYLE_BT)
    {
        for(i = 0; i < num_pat; i++)
        {
            // Put a backslash before special
            // characters, except inside ``.
            bool intick = false;
            p = command + ustrlen(command);
            *p++ = ' ';

            for(j = 0; pat[i][j] != NUL; j++)
            {
                if(pat[i][j] == '`')
                {
                    intick = !intick;
                }
                else if(pat[i][j] == '\\' && pat[i][j + 1] != NUL)
                {
                    // Remove a backslash, take char literally.
                    // But keep backslash inside backticks, before
                    // a special character and before a backtick.
                    if(intick
                       || ustrchr(SHELL_SPECIAL, pat[i][j + 1]) != NULL
                       || pat[i][j + 1] == '`')
                    {
                        *p++ = '\\';
                    }

                    j++;
                }
                else if(!intick
                        && ((flags & EW_KEEPDOLLAR) == 0 || pat[i][j] != '$')
                        && ustrchr(SHELL_SPECIAL, pat[i][j]) != NULL)
                {
                    // Put a backslash before a special character,
                    // but not when inside ``. And not for $var when
                    // EW_KEEPDOLLAR is set.
                    *p++ = '\\';
                }

                // Copy one character.
                *p++ = pat[i][j];
            }

            *p = NUL;
        }
    }

    if(flags & EW_SILENT)
    {
        shellopts |= kShellOptHideMess;
    }

    if(ampersent)
    {
        ustrcat(command, "&"); // put the '&' after the redirection
    }

    // Using zsh -G: If a pattern has no matches, it is just deleted from
    // the argument list, otherwise zsh gives an error message and doesn't
    // expand any other pattern.
    if(shell_style == STYLE_PRINT)
    {
        extra_shell_arg = (uchar_kt *)"-G"; // Use zsh NULL_GLOB option

        // If we use -f then shell variables set in .cshrc won't get
        // expanded. vi can do it, so we will too, but it is only
        // necessary if there is a "$" in one of the patterns, otherwise
        // we can still use the fast option.
    }
    else if(shell_style == STYLE_GLOB && !have_dollars(num_pat, pat))
    {
        extra_shell_arg = (uchar_kt *)"-f"; // Use csh fast option
    }

    // execute the shell command
    i = call_shell(command, shellopts, extra_shell_arg);

    // When running in the background, give it some time to
    // create  the temp file, but don't wait for it to finish.
    if(ampersent)
    {
        os_delay(10L, true);
    }

    xfree(command);

    if(i) // os_call_shell() failed
    {
        os_remove((char *)tempname);
        xfree(tempname);

        // With interactive completion,
        // the error message is not printed.
        if(!(flags & EW_SILENT))
        {
            redraw_later_clear(); // probably messed up screen
            msg_putchar('\n'); // clear bottom line quickly

        #if HOST_SIZEOF_LONG > HOST_SIZEOF_INT
            assert(Rows <= (long)INT_MAX + 1);
        #endif

            cmdline_row = (int)(Rows - 1); // continue on last line
            MSG(_(e_wildexpand));
            msg_start(); // don't overwrite this message
        }

        // If a `cmd` expansion failed, don't list `cmd` as
        // a match, even when EW_NOTFOUND is given
        if(shell_style == STYLE_BT)
        {
            return FAIL;
        }

        goto notfound;
    }

    // read the names from the file into memory
    fd = fopen((char *)tempname, READBIN);

    if(fd == NULL)
    {
        // Something went wrong, perhaps a
        // file name with a special char.
        if(!(flags & EW_SILENT))
        {
            MSG(_(e_wildexpand));
            msg_start(); // don't overwrite this message
        }

        xfree(tempname);
        goto notfound;
    }

    int fseek_res = fseek(fd, 0L, SEEK_END);

    if(fseek_res < 0)
    {
        xfree(tempname);
        fclose(fd);
        return FAIL;
    }

    int64_t templen = ftell(fd); // get size of temp file

    if(templen < 0)
    {
        xfree(tempname);
        fclose(fd);
        return FAIL;
    }

#if HOST_SIZEOF_LONG_LONG > HOST_SIZEOF_SIZE_T
    assert(templen <= (long long)SIZE_MAX);
#endif

    len = (size_t)templen;
    fseek(fd, 0L, SEEK_SET);
    buffer = xmalloc(len + 1);

    // fread() doesn't terminate buffer with NUL;
    // appropiate termination (not always NUL) is done below.
    size_t readlen = fread((char *)buffer, 1, len, fd);
    fclose(fd);
    os_remove((char *)tempname);

    if(readlen != len)
    {
        // unexpected read error
        EMSG2(_(e_notread), tempname);
        xfree(tempname);
        xfree(buffer);
        return FAIL;
    }

    xfree(tempname);

    // file names are separated with Space
    if(shell_style == STYLE_ECHO)
    {
        buffer[len] = '\n'; // make sure the buffer ends in NL
        p = buffer;

        for(i = 0; *p != '\n'; i++) // count number of entries
        {
            while(*p != ' ' && *p != '\n')
            {
                p++;
            }

            p = skipwhite(p); // skip to next entry
        }

        // file names are separated with NL
    }
    else if(shell_style == STYLE_BT || shell_style == STYLE_VIMGLOB)
    {
        buffer[len] = NUL; // make sure the buffer ends in NUL
        p = buffer;

        for(i = 0; *p != NUL; i++) // count number of entries
        {
            while(*p != '\n' && *p != NUL)
            {
                p++;
            }

            if(*p != NUL)
            {
                p++;
            }

            p = skipwhite(p); // skip leading white space
        }

        // file names are separated with NUL
    }
    else
    {
        // Some versions of zsh use spaces instead of NULs to separate
        // results. Only do this when there is no NUL before the end of the
        // buffer, otherwise we would never be able to use file names with
        // embedded spaces when zsh does use NULs. When we found a NUL once,
        // we know zsh is OK, set did_find_nul and don't check for spaces again.
        check_spaces = false;

        if(shell_style == STYLE_PRINT && !did_find_nul)
        {
            // If there is a NUL, set did_find_nul, else set check_spaces
            buffer[len] = NUL;

            if(len && (int)ustrlen(buffer) < (int)len)
            {
                did_find_nul = true;
            }
            else
            {
                check_spaces = true;
            }
        }

        // Make sure the buffer ends with a NUL. For STYLE_PRINT there
        // already is one, for STYLE_GLOB it needs to be added.
        if(len && buffer[len - 1] == NUL)
        {
            len--;
        }
        else
        {
            buffer[len] = NUL;
        }

        i = 0;

        for(p = buffer; p < buffer + len; p++)
        {
            if(*p == NUL || (*p == ' ' && check_spaces)) // count entry
            {
                i++;
                *p = NUL;
            }
        }

        if(len)
        {
            i++; // count last entry
        }
    }

    assert(buffer[len] == NUL || buffer[len] == '\n');

    if(i == 0)
    {
        // Can happen when using /bin/sh and typing ":e $NO_SUCH_VAR^I".
        // /bin/sh will happily expand it to nothing rather than returning
        // an error; and hey, it's good to check anyway
        xfree(buffer);
        goto notfound;
    }

    *num_file = i;
    *file = xmalloc(sizeof(uchar_kt *) * (size_t)i);

    // Isolate the individual file names.
    p = buffer;

    for(i = 0; i < *num_file; ++i)
    {
        (*file)[i] = p;

        // Space or NL separates
        if(shell_style == STYLE_ECHO
           || shell_style == STYLE_BT
           || shell_style == STYLE_VIMGLOB)
        {
            while(!(shell_style == STYLE_ECHO && *p == ' ')
                  && *p != '\n' && *p != NUL)
            {
                p++;
            }

            if(p == buffer + len) // last entry
            {
                *p = NUL;
            }
            else
            {
                *p++ = NUL;
                p = skipwhite(p); // skip to next entry
            }
        }
        else // NUL separates
        {
            while(*p && p < buffer + len) // skip entry
            {
                p++;
            }

            p++; // skip NUL
        }
    }

    // Move the file names to allocated memory.
    for(j = 0, i = 0; i < *num_file; i++)
    {
        // Require the files to exist. Helps when using /bin/sh
        if(!(flags & EW_NOTFOUND) && !os_path_exists((*file)[i]))
        {
            continue;
        }

        // check if this entry should be included
        dir = (os_isdir((*file)[i]));

        if((dir && !(flags & EW_DIR)) || (!dir && !(flags & EW_FILE)))
        {
            continue;
        }

        // Skip files that are not executable if we check for that.
        if(!dir
           && (flags & EW_EXEC)
           && !os_can_exe((*file)[i], NULL, !(flags & EW_SHELLCMD)))
        {
            continue;
        }

        p = xmalloc(ustrlen((*file)[i]) + 1 + dir);
        ustrcpy(p, (*file)[i]);

        if(dir)
        {
            add_pathsep((char *)p); // add '/' to a directory name
        }

        (*file)[j++] = p;
    }

    xfree(buffer);
    *num_file = j;

    if(*num_file == 0) // rejected all entries
    {
        xfree(*file);
        *file = NULL;
        goto notfound;
    }

    return OK;

notfound:

    if(flags & EW_NOTFOUND)
    {
        save_patterns(num_pat, pat, num_file, file);
        return OK;
    }

    return FAIL;
}


static void save_patterns(int num_pat,
                          uchar_kt **pat,
                          int *num_file,
                          uchar_kt ***file)
{
    int i;
    uchar_kt *s;
    *file = xmalloc((size_t)num_pat * sizeof(uchar_kt *));

    for(i = 0; i < num_pat; i++)
    {
        s = ustrdup(pat[i]);

        // Be compatible with expand_filename():
        // halve the number of backslashes.
        backslash_halve(s);

        (*file)[i] = s;
    }

    *num_file = num_pat;
}

static bool have_wildcard(int num, uchar_kt **file)
{
    int i;

    for(i = 0; i < num; i++)
    {
        if(path_has_wildcard(file[i]))
        {
            return true;
        }
    }

    return false;
}

static bool have_dollars(int num, uchar_kt **file)
{
    int i;

    for(i = 0; i < num; i++)
    {
        if(ustrchr(file[i], '$') != NULL)
        {
            return true;
        }
    }

    return false;
}
