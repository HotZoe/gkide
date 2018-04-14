/// @file nvim/version.c
///
/// Nvim was forked from Vim 7.4.160.
/// Vim originated from Stevie version 3.6 (Fish disk 217) by GRWalter (Fred).

#include <inttypes.h>
#include <assert.h>
#include <limits.h>

#include "nvim/api/private/helpers.h"
#include "nvim/nvim.h"
#include "nvim/ascii.h"
#include "nvim/iconv.h"
#include "nvim/version.h"
#include "nvim/charset.h"
#include "nvim/macros.h"
#include "nvim/memline.h"
#include "nvim/memory.h"
#include "nvim/message.h"
#include "nvim/screen.h"
#include "nvim/utils.h"

// version and source info generated by the build system
#include "generated/config/buildinfo.h"
#include "generated/config/gkideenvs.h"
#include "generated/config/gkideversion.h"
#include "generated/config/confignvim.h"

#define NVIM_VERSION_LONG                               \
    "nvim v" NVIM_VERSION_BASIC "-" NVIM_RELEASE_TYPE   \
    ", API(v" TO_STRING(NVIM_API_VERSION) ")"

// ":version", "$ nvim --version"
#define NVIM_MODIFY_TIME \
    GIT_COMMIT_DATE " " GIT_COMMIT_TIME " " GIT_COMMIT_ZONE

char *nvim_version_long =
    "NVIM v" NVIM_VERSION_BASIC " (GKIDE v" GKIDE_RELEASE_VERSION ")";

#define BUILD_HOST_OS_INFO \
    BUILD_ON_HOST "(" BUILD_OS_NAME ", v" BUILD_OS_VERSION ", " BUILD_OS_ARCH ")"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "version.c.generated.h"
#endif

// feature name no more than 15 chars for pretty
static char *features[] =
{
#ifdef NVIM_BUILTIN_TUI_ENABLE
    "+tui",
#else
    "-tui",
#endif

#if (defined(HAVE_HDR_ICONV_H) && defined(USE_ICONV)) || defined(DYNAMIC_ICONV)
    #ifdef DYNAMIC_ICONV
    "+iconv/dyn",
    #else
    "+iconv",
    #endif
#else
    "-iconv",
#endif

#ifdef FOUND_WORKING_JEMALLOC
    "+jemalloc",
#else
    "-jemalloc",
#endif

    NULL
};

#define PATCH_INFO(id, desc)    \
    [id] = {                    \
        .patch_id = id,         \
        .patch_desc = desc,     \
    }
typedef struct patch_info_s
{
    int patch_id;
    char *patch_desc;
} patch_info_st;

static patch_info_st extra_patches[] =
{
    // patch description message no more than 80 chars for pretty
    // add patch description below
    PATCH_INFO(0, NULL),
};

/// Compares a version string to the current GKIDE version.
///
/// @param version Version string like "1.3.42"
///
/// @return true if GKIDE is at or above the version.
bool has_gkide_version(const char *const version_str)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_NONNULL_ALL
{
    const char *p = version_str;
    int nvim_major = 0;
    int nvim_minor = 0;
    int nvim_patch = 0;

    if(!ascii_isdigit(*p))
    {
        return false;
    }

    nvim_major = atoi(p);
    p = strchr(p, '.'); // Find the next dot.

    if(p)
    {
        p++; // Advance past the dot.

        if(!ascii_isdigit(*p))
        {
            return false;
        }

        nvim_minor = atoi(p);
        p = strchr(p, '.');

        if(p)
        {
            p++;

            if(!ascii_isdigit(*p))
            {
                return false;
            }

            nvim_patch = atoi(p);
        }
    }

    return (nvim_major < NVIM_VERSION_MAJOR
            || (nvim_major == NVIM_VERSION_MAJOR
                && (nvim_minor < NVIM_VERSION_MINOR
                    || (nvim_minor == NVIM_VERSION_MINOR
                        && nvim_patch <= NVIM_VERSION_PATCH))));
}

/// Compares a version string to the current Nvim version.
///
/// @param version Version string like "1.3.42"
///
/// @return true if Nvim is at or above the version.
bool has_nvim_version(const char *const version_str)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_NONNULL_ALL
{
    const char *p = version_str;
    int nvim_major = 0;
    int nvim_minor = 0;
    int nvim_patch = 0;

    if(!ascii_isdigit(*p))
    {
        return false;
    }

    nvim_major = atoi(p);
    p = strchr(p, '.'); // Find the next dot.

    if(p)
    {
        p++; // Advance past the dot.

        if(!ascii_isdigit(*p))
        {
            return false;
        }

        nvim_minor = atoi(p);
        p = strchr(p, '.');

        if(p)
        {
            p++;

            if(!ascii_isdigit(*p))
            {
                return false;
            }

            nvim_patch = atoi(p);
        }
    }

    return (nvim_major < NVIM_VERSION_MAJOR
            || (nvim_major == NVIM_VERSION_MAJOR
                && (nvim_minor < NVIM_VERSION_MINOR
                    || (nvim_minor == NVIM_VERSION_MINOR
                        && nvim_patch <= NVIM_VERSION_PATCH))));
}

/// Checks whether a Vim patch has been included.
///
/// @param n Patch number.
///
/// @return true if patch `n` has been included.
bool has_nvim_patch(int n)
{
    int cnt = ARRAY_SIZE(extra_patches);
    for(int i = 0; i < cnt; i++)
    {
        if(extra_patches[i].patch_id == n)
        {
            return true;
        }
    }

    return false;
}

/// generated nvim version dictionary info for remote API object
Dictionary gen_version_dict(void)
{
    Dictionary d = ARRAY_DICT_INIT;

    PUT(d, "major", INTEGER_OBJ(NVIM_VERSION_MAJOR));
    PUT(d, "minor", INTEGER_OBJ(NVIM_VERSION_MINOR));
    PUT(d, "patch", INTEGER_OBJ(NVIM_VERSION_PATCH));
    PUT(d, "api_level", INTEGER_OBJ(NVIM_API_VERSION));
    PUT(d, "api_compatible", INTEGER_OBJ(NVIM_API_COMPATIBLE));
    PUT(d, "api_prerelease", BOOLEAN_OBJ(NVIM_API_PRERELEASE));

    PUT(d, "build_reversion", STRING_OBJ(cstr_to_string(GIT_COMMIT_HASH)));
    PUT(d, "build_timestamp", STRING_OBJ(cstr_to_string(BUILD_TIMESTAMP)));
    PUT(d, "build_by_user", STRING_OBJ(cstr_to_string(BUILD_BY_USER)));
    PUT(d, "build_on_host", STRING_OBJ(cstr_to_string(BUILD_ON_HOST)));
    PUT(d, "build_os_name", STRING_OBJ(cstr_to_string(BUILD_OS_NAME)));
    PUT(d, "build_os_arch", STRING_OBJ(cstr_to_string(BUILD_OS_ARCH)));
    PUT(d, "build_os_version", STRING_OBJ(cstr_to_string(BUILD_OS_VERSION)));
    PUT(d, "build_release_type", STRING_OBJ(cstr_to_string(NVIM_RELEASE_TYPE)));

    return d;
}

void ex_version(exargs_st *eap)
{
    // Ignore a ":version 9.99" command.
    if(*eap->arg == NUL)
    {
        msg_putchar('\n');
        list_version();
    }
}

// Print the list of extra patch descriptions if there is at least one.
static void list_patches(void)
{
    int cnt = ARRAY_SIZE(extra_patches);
    if(extra_patches[0].patch_desc != NULL)
    {
        MSG_PUTS("\n\nExtra patches:");

        for(int i = 0; i < cnt; ++i)
        {
            int id = extra_patches[i].patch_id;
            char *desc = extra_patches[i].patch_desc;
            char data[20] = { 0 };

            msg_putchar('\n');
            version_msg(num_to_str(id, 10, data));
            version_msg(" - ");
            version_msg(desc);
        }
    }
}

// List all features aligned in columns
static void list_features(void)
{
    version_msg("\n\nOptional features included (+) or excluded (-):\n");

    // for pretty show, each line has 5-items
    // and each item take 15-char-spaces
    int cnt = 0;
    for(int i = 0; !got_int && features[i] != NULL; ++i)
    {
        int len = (int)ustrlen(features[i]);
        int space_to_feat = 15 - len;

        cnt++;
        version_msg(features[i]);

        if(5 == cnt)
        {
            cnt = 0;
            msg_putchar('\n');
            continue;
        }

        while(space_to_feat--)
        {
            msg_putchar(' ');
        }
    }
}

void list_version(void)
{
    MSG_PUTS("      Version: " NVIM_VERSION_LONG "\n");
    MSG_PUTS("     Build at: " BUILD_TIMESTAMP "\n");
    MSG_PUTS("  Modified at: " NVIM_MODIFY_TIME "\n");
    MSG_PUTS("  Compiled by: " BUILD_BY_USER "@" BUILD_HOST_OS_INFO "\n");
    MSG_PUTS("GKIDE Package: " GKIDE_PACKAGE_NAME "\n");

    size_t len;
    char msg_buf[MAXPATHL] = { 0 };

    // gkide system home
    snprintf(msg_buf, MAXPATHL, "\n    $%s: ", ENV_GKIDE_SYS_HOME);
    len = strlen(msg_buf);
    if(gkide_sys_home != NULL)
    {
        snprintf(msg_buf + len, MAXPATHL, "%s", gkide_sys_home);
    }
    version_msg(msg_buf);

    // gkide user home
    snprintf(msg_buf, MAXPATHL, "\n    $%s: ", ENV_GKIDE_USR_HOME);
    len = strlen(msg_buf);
    if(gkide_usr_home != NULL)
    {
        snprintf(msg_buf + len, MAXPATHL, "%s", gkide_usr_home);
    }
    version_msg(msg_buf);

    // directories layout
    version_msg("\n     Default Layout: bin, etc, plg, doc, mis\n");
    version_msg("\n      System config: $" ENV_GKIDE_SYS_CONFIG "/config.nvl");
    version_msg("\n        User config: $" ENV_GKIDE_USR_CONFIG "/config");

    list_features();
    list_patches();
}

/// Output a string for the version message. If it's going to wrap,
/// output a newline, unless the message is too long to fit on the
/// screen anyway.
///
/// @param s
static void version_msg(char *s)
{
    int len = (int)ustrlen(s);

    if(!got_int
       && (len < (int)Columns)
       && (msg_col + len >= (int)Columns)
       && (*s != '\n'))
    {
        msg_putchar('\n');
    }

    if(!got_int)
    {
        MSG_PUTS(s);
    }
}


/// Show the intro message when not editing a file.
void maybe_intro_message(void)
{
    if(bufempty()
       && (curbuf->b_fname == NULL)
       && (firstwin->w_next == NULL)
       && (ustrchr(p_shm, SHM_INTRO) == NULL))
    {
        intro_message(FALSE);
    }
}

/// Give an introductory message about Vim.
/// Only used when starting Vim on an empty file, without a file name.
/// Or with the ":intro" command (for Sven :-).
///
/// @param colon TRUE for ":intro"
void intro_message(int colon)
{
    static char *(lines[]) = {
        N_("NVIM v" NVIM_VERSION_BASIC),
        "",
        N_("by Charlie WONG et al."),
        N_("nvim is open source and freely distributable"),
        N_("https://github.com/gkide/gkide"),
        "",
        N_("type :help nvim<Enter>       if you are new  "),
        N_("type :CheckHealth<Enter>     to optimize nvim"),
        N_("type :q<Enter>               to exit         "),
        N_("type :help<Enter>            for help        "),
        "",
        N_("Help poor children in Uganda!"),
        N_("type :help iccf<Enter>       for information "),
    };

    // blanklines = screen height - # message lines
    size_t lines_size = ARRAY_SIZE(lines);
    assert(lines_size <= LONG_MAX);
    long blanklines = Rows - ((long)lines_size - 1l);

    // Don't overwrite a statusline. Depends on 'cmdheight'.
    if(p_ls > 1)
    {
        blanklines -= Rows - topframe->fr_height;
    }

    if(blanklines < 0)
    {
        blanklines = 0;
    }

    // start displaying the message lines after half of the blank lines
    long row_num = blanklines / 2;

    // Show the empty message, 4/8
    // Show the uganda message, 2/8
    // Show the sponsor message, 1/8
    // Show the register message, 1/8
    int magic = (int)time(NULL);

    if(((row_num >= 2) && (Columns >= 50)) || colon)
    {
        char *p = NULL;

        for(int i=0, replace=0; i < (int)lines_size; ++i, replace=0)
        {
            p = lines[i];

            // check the last two lines of messages
            if((i+1)==(int)lines_size || (i+2)==(int)lines_size)
            {
                if((magic & 1) == 0)
                {
                    p = ""; // show nothing, 4/8
                }
                else
                {
                    if(strstr(p, "children") != NULL)
                    {
                        replace = 1;
                    }

                    if(!replace && strstr(p, "iccf") != NULL)
                    {
                        replace = 2;
                    }

                    if((magic & 2) == 0)
                    {
                        if((magic & 4) == 0)
                        {
                            // show sponsor message, 1/8
                            if(replace==1)
                            {
                                p = N_("Become a registered Vim user!");
                            }
                            else if(replace==2)
                            {
                                p = N_("type :help register<Enter>   for information ");
                            }
                        }
                        else
                        {
                            // show register message, 1/8
                            if(replace==1)
                            {
                                p = N_("Sponsor Vim development!");
                            }
                            else if(replace==2)
                            {
                                p = N_("type :help sponsor<Enter>    for information ");
                            }
                        }
                    }
                    else
                    {
                        // show uganda message, 2/8
                    }
                }
            }

            if(*p != NUL)
            {
                do_intro_line(row_num, (uchar_kt *)p, 0);
            }

            row_num++;
        }
    }

    for(int i=0; i < blanklines/2; ++i)
    {
        do_intro_line(row_num, (uchar_kt *)"", 0);
        row_num++;
    }

    // Make the wait-return message appear just below the text.
    if(colon)
    {
        assert(row_num <= INT_MAX);
        msg_row = (int)row_num;
    }
}

static void do_intro_line(long row, uchar_kt *mesg, int attr)
{
    long col;
    uchar_kt *p;
    int l;
    int clen;

    // Center the message horizontally.
    col = ustr_scrsize(mesg);
    col = (Columns - col) / 2;

    if(col < 0)
    {
        col = 0;
    }

    // Split up in parts to highlight <> items differently.
    for(p = mesg; *p != NUL; p += l)
    {
        clen = 0;

        for(l = 0;
            p[l] != NUL && (l == 0 || (p[l] != '<' && p[l - 1] != '>'));
            ++l)
        {
            if(has_mbyte)
            {
                clen += ptr2cells(p + l);
                l += (*mb_ptr2len)(p + l) - 1;
            }
            else
            {
                clen += byte2cells(p[l]);
            }
        }

        assert(row <= INT_MAX && col <= INT_MAX);

        screen_puts_len(p, l, (int)row, (int)col,
                        *p == '<' ? hl_attr(HLF_8) : attr);
        col += clen;
    }
}

/// ":intro": clear screen, display intro screen and wait for return.
///
/// @param eap
void ex_intro(exargs_st *FUNC_ARGS_UNUSED_REALY(eap))
{
    screenclear();
    intro_message(TRUE);
    wait_return(TRUE);
}
