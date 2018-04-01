/// @file nvim/cmd_line_args.c

#include "nvim/vim.h"
#include "nvim/main.h"
#include "nvim/eval.h"
#include "nvim/path.h"
#include "nvim/ascii.h"
#include "nvim/buffer.h"
#include "nvim/option.h"
#include "nvim/version.h"
#include "nvim/getchar.h"
#include "nvim/message.h"
#include "nvim/globals.h"
#include "nvim/os_unix.h"
#include "nvim/ex_docmd.h"
#include "nvim/os/os.h"
#include "nvim/os/signal.h"
#include "nvim/cmd_line_args.h"
#include "nvim/msgpack/helpers.h"
#include "nvim/msgpack/server.h"
#include "nvim/msgpack/channel.h"
#include "nvim/api/private/defs.h"
#include "nvim/api/private/helpers.h"
#include "nvim/api/private/handle.h"
#include "nvim/api/private/dispatch.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "cmd_line_args.c.generated.h"
#endif

// Error messages
static const char *err_arg_missing = N_("Argument missing after");
static const char *err_opt_garbage = N_("Garbage after option argument");
static const char *err_opt_unknown = N_("Unknown option argument");
static const char *err_too_many_args = N_("Too many edit arguments");
static const char *err_extra_cmd =
    N_("Too many \"+command\", \"-c command\" or \"--cmd command\" arguments");

/// Prints help message for <b>$ nvim -h</b> or <b>$ nvim --help</b>
void cmd_line_usage(void)
{
    signal_stop(); // kill us with CTRL-C here, if you like

    mch_msg(_("GKIDE Nvim Usage:\n"));
    mch_msg(_("  nvim [arguments] [file ...]      Edit specified file(s)\n"));
    mch_msg(_("  nvim [arguments] -               Read text from stdin\n"));
    mch_msg(_("  nvim [arguments] -t <tag>        Edit file where tag is defined\n"));
    mch_msg(_("  nvim [arguments] -q [errorfile]  Edit file with first error\n\n"));
    mch_msg(_("Arguments:\n"));
    mch_msg(_("  --                    Only file names after this\n"));

#if !defined(UNIX)
    mch_msg(_("  --literal             Don't expand wildcards\n"));
#endif

    mch_msg(_("  -e                    Ex mode\n"));
    mch_msg(_("  -E                    Improved Ex mode\n"));
    mch_msg(_("  -s                    Silent (batch) mode (only for ex mode)\n"));
    mch_msg(_("  -d                    Diff mode\n"));
    mch_msg(_("  -R                    Read-only mode\n"));
    mch_msg(_("  -Z                    Restricted mode\n"));
    mch_msg(_("  -m                    Modifications (writing files) not allowed\n"));
    mch_msg(_("  -M                    Modifications in text not allowed\n"));
    mch_msg(_("  -b                    Binary mode\n"));
    mch_msg(_("  -l                    Lisp mode\n"));
    mch_msg(_("  -A                    Arabic mode\n"));
    mch_msg(_("  -F                    Farsi mode\n"));
    mch_msg(_("  -H                    Hebrew mode\n"));
    mch_msg(_("  -V[N][file]           Be verbose [level N][log messages to file]\n"));
    mch_msg(_("  -D                    Debugging mode\n"));
    mch_msg(_("  -n                    No swap file, use memory only\n"));
    mch_msg(_("  -r, -L                List swap files and exit\n"));
    mch_msg(_("  -r <file>             Recover crashed session\n"));
    mch_msg(_("  -u <vimrc>            Use <vimrc> instead of the default\n"));
    mch_msg(_("  -i <shada>            Use <shada> instead of the default\n"));
    mch_msg(_("  --noplugin            Don't load plugin scripts\n"));
    mch_msg(_("  -o[N]                 Open N windows (default: one for each file)\n"));
    mch_msg(_("  -O[N]                 Like -o but split vertically\n"));
    mch_msg(_("  -p[N]                 Open N tab pages (default: one for each file)\n"));
    mch_msg(_("  +                     Start at end of file\n"));
    mch_msg(_("  +<linenum>            Start at line <linenum>\n"));
    mch_msg(_("  +/<pattern>           Start at first occurrence of <pattern>\n"));
    mch_msg(_("  --cmd <command>       Execute <command> before loading any vimrc\n"));
    mch_msg(_("  -c <command>          Execute <command> after loading the first file\n"));
    mch_msg(_("  -S <session>          Source <session> after loading the first file\n"));
    mch_msg(_("  -s <scriptin>         Read Normal mode commands from <scriptin>\n"));
    mch_msg(_("  -w <scriptout>        Append all typed characters to <scriptout>\n"));
    mch_msg(_("  -W <scriptout>        Write all typed characters to <scriptout>\n"));
    mch_msg(_("  --startuptime <file>  Write startup timing messages to <file>\n"));
    mch_msg(_("  --api-info            Dump API metadata serialized to msgpack and exit\n"));
    mch_msg(_("  --embed               Use stdin/stdout as a msgpack-rpc channel\n"));
    mch_msg(_("  --server [addr:port]  Start nvim server, do not start the TUI\n"));
    mch_msg(_("  --headless            Don't start a user interface\n"));
    mch_msg(_("  -v, --version         Print version information and exit\n"));
    mch_msg(_("  -h, --help            Print this help message and exit\n"));
}

// skip to next argument
#define SKIP_TO_NEXT    -1

/// Parse the command line arguments.
void cmd_line_args_parser(main_args_st *parmp)
{
    // skip the programe name itself
    int argc = parmp->argc - 1;
    char **argv = parmp->argv + 1;

    int argv_idx = 1; // index in argv[n][]
    int want_optval = FALSE; // option argument with argument
    int had_minmin = FALSE; // found "--" argument

    long n;
    uchar_kt *p = NULL;

    while(argc > 0)
    {
        if(argv[0][0] == '+' && !had_minmin)
        {
            process_cmd_plus(parmp, argv);
            argv_idx = SKIP_TO_NEXT;
        }
        else if(argv[0][0] == '-' && !had_minmin)
        {
            // Optional argument
            want_optval = FALSE;
            int cmd_id = argv[0][argv_idx++];

            switch(cmd_id)
            {
                case NUL:
                {
                    process_cmd_minus_only(parmp, argv);
                    argv_idx = SKIP_TO_NEXT;
                    break;
                }
                case '-':

                    // "--" don't take any more option arguments
                    // "--help" give help message
                    // "--version" give version message
                    // "--literal" take files literally
                    // "--noplugin[s]" skip plugins
                    // "--cmd <cmd>" execute cmd before vimrc
                    if(STRICMP(argv[0] + argv_idx, "help") == 0)
                    {
                        cmd_line_usage();
                        mch_exit(0);
                    }
                    else if(STRICMP(argv[0] + argv_idx, "version") == 0)
                    {
                        show_version();
                        mch_exit(0);
                    }
                    else if(STRICMP(argv[0] + argv_idx, "api-info") == 0)
                    {
                        msgpack_sbuffer *b = msgpack_sbuffer_new();

                        msgpack_packer *p =
                            msgpack_packer_new(b, msgpack_sbuffer_write);

                        Object md = DICTIONARY_OBJ(api_metadata());

                        rpc_from_object(md, p);

                        for(size_t i = 0; i < b->size; i++)
                        {
                            putchar(b->data[i]);
                        }

                        msgpack_packer_free(p);
                        mch_exit(0);
                    }
                    else if(STRICMP(argv[0] + argv_idx, "headless") == 0)
                    {
                        parmp->headless = true;
                    }
                    else if(STRICMP(argv[0] + argv_idx, "embed") == 0)
                    {
                        embedded_mode = true;
                        parmp->headless = true;
                        channel_from_stdio();
                    }
                    else if(STRNICMP(argv[0] + argv_idx, "literal", 7) == 0)
                    {
                        #if !defined(UNIX)
                        parmp->literal = TRUE;
                        #endif
                    }
                    else if(STRNICMP(argv[0] + argv_idx, "noplugin", 8) == 0)
                    {
                        p_lpl = FALSE;
                    }
                    else if(STRNICMP(argv[0] + argv_idx, "cmd", 3) == 0)
                    {
                        want_optval = TRUE;
                        argv_idx += 3;
                    }
                    else if(STRNICMP(argv[0] + argv_idx, "startuptime", 11) == 0)
                    {
                        want_optval = TRUE;
                        argv_idx += 11;
                    }
                    else
                    {
                        if(argv[0][argv_idx])
                        {
                            cmd_args_err_exit(err_opt_unknown, argv[0]);
                        }

                        had_minmin = TRUE;
                    }

                    if(!want_optval)
                    {
                        argv_idx = -1; // skip to next argument
                    }

                    break;

                case 'A':
                    // "-A" start in Arabic mode.
                    set_option_value("arabic", 1L, NULL, 0);
                    break;

                case 'b':
                    // "-b" binary mode.
                    // Needs to be effective before expanding file names,
                    // because for Win32 this makes us edit a shortcut file
                    // itself, instead of the file it links to.
                    set_options_bin(curbuf->b_p_bin, 1, 0);
                    curbuf->b_p_bin = 1; // Binary file I/O.
                    break;

                case 'e':
                    // "-e" Ex mode
                    exmode_active = EXMODE_NORMAL;
                    break;

                case 'E':
                    // "-E" Improved Ex mode
                    exmode_active = EXMODE_VIM;
                    break;

                case 'f':
                    // "-f"  GUI: run in foreground.
                    break;

                case 'F':
                    // "-F" start in Farsi mode: rl + fkmap set.
                    p_fkmap = true;
                    set_option_value("rl", 1L, NULL, 0);
                    break;

                case 'h':
                    // "-h" give help message
                    cmd_line_usage();
                    mch_exit(0);

                case 'H':
                    // "-H" start in Hebrew mode: rl + hkmap set.
                    p_hkmap = true;
                    set_option_value("rl", 1L, NULL, 0);
                    break;

                case 'l':
                    // "-l" lisp mode, 'lisp' and 'showmatch' on.
                    set_option_value("lisp", 1L, NULL, 0);
                    p_sm = true;
                    break;

                case 'M':
                    // "-M"  no changes or writing of files
                    reset_modifiable();

                    FALL_THROUGH_ATTRIBUTE;

                case 'm':
                    // "-m"  no writing of files
                    p_write = FALSE;
                    break;

                case 'N':
                    // "-N"  Nocompatible
                    // No-op
                    break;

                case 'n':
                    // "-n" no swap file
                    parmp->no_swap_file = TRUE;
                    break;

                case 'p':
                    // "-p[N]" open N tab pages
                    // default is 0: open window for each file
                    parmp->window_count = get_number_arg(argv[0], &argv_idx, 0);
                    parmp->window_layout = kWinLayoutTabpage;
                    break;

                case 'o':
                    // "-o[N]" open N horizontal split windows
                    // default is 0: open window for each file
                    parmp->window_count = get_number_arg(argv[0], &argv_idx, 0);
                    parmp->window_layout = kWinLayoutHorizontal;
                    break;

                case 'O':
                    // "-O[N]" open N vertical split windows
                    // default is 0: open window for each file
                    parmp->window_count = get_number_arg(argv[0], &argv_idx, 0);
                    parmp->window_layout = kWinLayoutVertical;
                    break;

                case 'q':

                    // "-q" QuickFix mode
                    if(parmp->edit_type != kEditTypeNone)
                    {
                        cmd_args_err_exit(err_too_many_args, argv[0]);
                    }

                    parmp->edit_type = kEditTypeQkfx;

                    if(argv[0][argv_idx])
                    {
                        // "-q{errorfile}"
                        parmp->err_file = (uchar_kt *)argv[0] + argv_idx;
                        argv_idx = -1;
                    }
                    else if(argc > 1)
                    {
                        // "-q {errorfile}"
                        want_optval = TRUE;
                    }

                    break;

                case 'R':
                    // "-R" readonly mode
                    readonlymode = TRUE;
                    curbuf->b_p_ro = TRUE;
                    p_uc = 10000; // don't update very often
                    break;

                case 'r': // "-r" recovery mode
                case 'L': // "-L" recovery mode
                    recoverymode = 1;
                    break;

                case 's':
                    if(exmode_active)
                    {
                        // "-s" silent (batch) mode
                        silent_mode = TRUE;
                    }
                    else
                    {
                        // "-s {scriptin}" read from script file
                        want_optval = TRUE;
                    }

                    break;

                case 't':

                    // "-t {tag}" or "-t{tag}" jump to tag
                    if(parmp->edit_type != kEditTypeNone)
                    {
                        cmd_args_err_exit(err_too_many_args, argv[0]);
                    }

                    parmp->edit_type = kEditTypeTag;

                    if(argv[0][argv_idx])
                    {
                        // "-t{tag}"
                        parmp->tagname = (uchar_kt *)argv[0] + argv_idx;
                        argv_idx = -1;
                    }
                    else
                    {
                        // "-t {tag}"
                        want_optval = TRUE;
                    }

                    break;

                case 'D':
                    // "-D" Debugging
                    parmp->debug_break_level = 9999;
                    break;

                case 'd':
                    // "-d" 'diff'
                    parmp->diff_mode = TRUE;
                    break;

                case 'v':
                    show_version();
                    mch_exit(0);

                case 'V':
                    // "-V{N}"  Verbose level
                    // default is 10: a little bit verbose
                    p_verbose = get_number_arg(argv[0], &argv_idx, 10);

                    if(argv[0][argv_idx] != NUL)
                    {
                        set_option_value("verbosefile", 0L,
                                         argv[0] + argv_idx, 0);

                        argv_idx = (int)STRLEN(argv[0]);
                    }

                    break;

                case 'w':

                    // "-w{number}" set window height
                    // "-w {scriptout}" write to script
                    if(ascii_isdigit(((uchar_kt *)argv[0])[argv_idx]))
                    {
                        n = get_number_arg(argv[0], &argv_idx, 10);
                        set_option_value("window", n, NULL, 0);
                        break;
                    }

                    want_optval = TRUE;
                    break;

                case 'Z':
                    // "-Z"  restricted mode
                    restricted = TRUE;
                    break;

                case 'c':

                    // "-c{command}" or "-c {command}" execute command
                    if(argv[0][argv_idx] != NUL)
                    {
                        if(parmp->cmd_num >= MAX_CMDS_NUM)
                        {
                            cmd_args_err_exit(err_extra_cmd, NULL);
                        }

                        parmp->cmd_args[parmp->cmd_num++] = argv[0] + argv_idx;

                        argv_idx = -1;
                        break;
                    }

                    FALL_THROUGH_ATTRIBUTE;

                case 'S': // "-S {file}" execute Vim script
                case 'i': // "-i {shada}" use for ShaDa file
                case 'u': // "-u {vimrc}" vim inits file
                case 'U': // "-U {gvimrc}" gvim inits file
                case 'W': // "-W {scriptout}" overwrite
                    want_optval = TRUE;
                    break;

                default:
                    cmd_args_err_exit(err_opt_unknown, argv[0]);
            }

            // Handle option arguments with argument.
            if(want_optval)
            {
                // Check for garbage immediately after the option letter.
                if(argv[0][argv_idx] != NUL)
                {
                    cmd_args_err_exit(err_opt_garbage, argv[0]);
                }

                --argc;

                // -S has an optional argument
                if(argc < 1 && cmd_id != 'S')
                {
                    cmd_args_err_exit(err_arg_missing, argv[0]);
                }

                ++argv;
                argv_idx = -1;

                switch(cmd_id)
                {
                    case 'c': // "-c {command}" execute command
                    case 'S': // "-S {file}" execute Vim script
                        if(parmp->cmd_num >= MAX_CMDS_NUM)
                        {
                            cmd_args_err_exit(err_extra_cmd, NULL);
                        }

                        if(cmd_id == 'S')
                        {
                            char *a;

                            if(argc < 1)
                            {
                                // "-S" without argument:
                                // use default session file name.
                                a = SESSION_FILE;
                            }
                            else if(argv[0][0] == '-')
                            {
                                // "-S" followed by another option:
                                // use default session file name.
                                a = SESSION_FILE;
                                ++argc;
                                --argv;
                            }
                            else
                            {
                                a = argv[0];
                            }

                            char *s = xmalloc(STRLEN(a) + 4);
                            sprintf(s, "so %s", a);
                            parmp->cmds_tofree[parmp->cmd_num] = TRUE;
                            parmp->cmd_args[parmp->cmd_num++] = s;
                        }
                        else
                        {
                            parmp->cmd_args[parmp->cmd_num++] = argv[0];
                        }

                        break;

                    case '-':
                        if(argv[-1][2] == 'c')
                        {
                            // "--cmd {command}" execute command
                            if(parmp->pre_cmd_num >= MAX_CMDS_NUM)
                            {
                                cmd_args_err_exit(err_extra_cmd, NULL);
                            }

                            parmp->pre_cmd_args[parmp->pre_cmd_num++] = argv[0];
                        }

                        // "--startuptime <file>" already handled
                        break;

                    case 'q':
                        // "-q {errorfile}" QuickFix mode
                        parmp->err_file = (uchar_kt *)argv[0];
                        break;

                    case 'i':
                        // "-i {shada}" use for shada
                        used_shada_file = argv[0];
                        break;

                    case 's':

                        // "-s {scriptin}" read from script file
                        if(scriptin[0] != NULL)
                        {
                scripterror:
                            mch_errmsg(_("Attempt to open script file again: \""));
                            mch_errmsg(argv[-1]);
                            mch_errmsg(" ");
                            mch_errmsg(argv[0]);
                            mch_errmsg("\"\n");
                            mch_exit(2);
                        }

                        if((scriptin[0] = mch_fopen(argv[0], READBIN)) == NULL)
                        {
                            mch_errmsg(_("Cannot open for reading: \""));
                            mch_errmsg(argv[0]);
                            mch_errmsg("\"\n");
                            mch_exit(2);
                        }

                        save_typebuf();
                        break;

                    case 't':
                        //"-t {tag}"
                        parmp->tagname = (uchar_kt *)argv[0];
                        break;

                    case 'u':
                        // "-u {vimrc}" vim inits file
                        parmp->use_nvimrc = argv[0];
                        break;

                    case 'U':
                        // "-U {gvimrc}" gvim inits file
                        break;

                    case 'w':

                        // "-w {nr}" 'window' value
                        // "-w {scriptout}" append to script file
                        if(ascii_isdigit(*((uchar_kt *)argv[0])))
                        {
                            argv_idx = 0;
                            n = get_number_arg(argv[0], &argv_idx, 10);
                            set_option_value("window", n, NULL, 0);
                            argv_idx = -1;
                            break;
                        }

                        FALL_THROUGH_ATTRIBUTE;

                    case 'W':

                        // "-W {scriptout}" overwrite script file
                        if(scriptout != NULL)
                        {
                            goto scripterror;
                        }

                        if((scriptout = mch_fopen(argv[0],
                                                  cmd_id == 'w'
                                                  ? APPENDBIN
                                                  : WRITEBIN)) == NULL)
                        {
                            mch_errmsg(_("Cannot open for script output: \""));
                            mch_errmsg(argv[0]);
                            mch_errmsg("\"\n");
                            mch_exit(2);
                        }

                        break;
                }
            }
        }
        else
        {
            // File name argument.
            argv_idx = -1; // skip to next argument

            // Check for only one type of editing.
            if(parmp->edit_type != kEditTypeNone && parmp->edit_type != kEditTypeFile)
            {
                cmd_args_err_exit(err_too_many_args, argv[0]);
            }

            parmp->edit_type = kEditTypeFile;
            // Add the file to the global argument list.
            ga_grow(&global_alist.al_ga, 1);
            p = vim_strsave((uchar_kt *)argv[0]);

            if(parmp->diff_mode
               && os_isdir(p)
               && GARGCOUNT > 0
               && !os_isdir(alist_name(&GARGLIST[0])))
            {
                char *fn = (char *)path_tail(alist_name(&GARGLIST[0]));
                uchar_kt *r = (uchar_kt *)concat_fnames((char *)p, fn, TRUE);
                xfree(p);
                p = r;
            }

        #ifdef USE_FNAME_CASE
            // Make the case of the file name match the actual file.
            path_fix_case(p);
        #endif

        #if !defined(HOST_OS_LINUX) && !defined(HOST_OS_MACOS)
            int buf_nr = parmp->literal ? 2 : 0; // add buffer nr after exp.
        #else
            int buf_nr = 2; // add buffer number now and use curbuf
        #endif

            alist_add(&global_alist, p, buf_nr);
        }

        // If there are no more letters after the current "-",
        // go to next argument. argv_idx is set to -1 when the
        // current argument is to be skipped.
        if(argv_idx <= 0 || argv[0][argv_idx] == NUL)
        {
            --argc;
            ++argv;
            argv_idx = 1;
        }
    }

    // If there is a "+123" or "-c" command,
    // set v:swapcommand to the first one.
    if(parmp->cmd_num > 0)
    {
        const size_t swcmd_len = STRLEN(parmp->cmd_args[0]) + 3;
        char *const swcmd = xmalloc(swcmd_len);
        snprintf(swcmd, swcmd_len, ":%s\r", parmp->cmd_args[0]);
        set_vim_var_string(VV_SWAPCOMMAND, swcmd, -1);
        xfree(swcmd);
    }

    TIME_MSG("cmd_line_args_parser");
}

static void process_cmd_plus(main_args_st *parmp, char **argv)
{
    if(parmp->cmd_num >= MAX_CMDS_NUM)
    {
        cmd_args_err_exit(err_extra_cmd, NULL);
    }

    if(argv[0][1] == NUL)
    {
        // "+", start at end of file
        parmp->cmd_args[parmp->cmd_num++] = "$";
    }
    else
    {
        // "+{number}", "+/{pat}" or "+{command}"
        parmp->cmd_args[parmp->cmd_num++] = &(argv[0][1]);
    }
}

static void process_cmd_minus_only(main_args_st *parmp, char **argv)
{
    // "vim -"  read from stdin
    if(exmode_active)
    {
        // "ex -" silent mode
        silent_mode = TRUE;
    }
    else
    {
        if(parmp->edit_type != kEditTypeNone)
        {
            cmd_args_err_exit(err_too_many_args, argv[0]);
        }

        parmp->edit_type = kEditTypeStdin;
    }
}

/// Prints the following then exits:
/// - An error message @b errstr
/// - A string @b str if not null
///
/// @param errstr  string containing an error message
/// @param str     string to append to the primary error message, or NULL
static void cmd_args_err_exit(const char *errstr, const char *str)
{
    char *prgname = (char *)path_tail((uchar_kt *)programme_name());
    signal_stop(); // kill us with CTRL-C here, if you like

    mch_errmsg(prgname);
    mch_errmsg(": ");
    mch_errmsg(_(errstr));

    if(str != NULL)
    {
        mch_errmsg(": \"");
        mch_errmsg(str);
        mch_errmsg("\"");
    }

    mch_errmsg(_("\nMore info with \""));
    mch_errmsg(prgname);
    mch_errmsg(" -h\"\n");

    mch_exit(1);
}

/// Prints version information for
/// <b>$ nvim -v</b> or <b>$ nvim --version</b>
static void show_version(void)
{
    info_message = TRUE; // use mch_msg(), not mch_errmsg()
    list_version();
    msg_putchar('\n');
    msg_didout = FALSE;
}

/// Gets the integer value of a numeric command
/// line argument if given, such as '-o10'.
///
/// @param[in]      p    pointer to argument
/// @param[in, out] idx  pointer to index in argument, is incremented
/// @param[in]      def  default value
///
/// @return def unmodified if:
/// - argument isn't given
/// - argument is non-numeric
///
/// @return argument's numeric value otherwise
static int get_number_arg(const char *p, int *idx, int def)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(ascii_isdigit(p[*idx]))
    {
        def = atoi(&(p[*idx]));

        while(ascii_isdigit(p[*idx]))
        {
            *idx = *idx + 1;
        }
    }

    return def;
}
