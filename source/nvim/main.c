/// @file nvim/main.c

#define EXTERN
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <msgpack.h>

#include "nvim/ascii.h"
#include "nvim/vim.h"
#include "nvim/main.h"
#include "nvim/diff.h"
#include "nvim/eval.h"
#include "nvim/fold.h"
#include "nvim/iconv.h"
#include "nvim/ascii.h"
#include "nvim/fileio.h"
#include "nvim/buffer.h"
#include "nvim/option.h"
#include "nvim/charset.h"
#include "nvim/ex_cmds.h"
#include "nvim/getchar.h"
#include "nvim/hashtab.h"
#include "nvim/ex_cmds2.h"
#include "nvim/if_cscope.h"

#ifdef HAVE_HDR_LOCALE_H
    #include <locale.h>
#endif

#include "nvim/mark.h"
#include "nvim/mbyte.h"
#include "nvim/memline.h"
#include "nvim/message.h"
#include "nvim/misc1.h"
#include "nvim/garray.h"
#include "nvim/log.h"
#include "nvim/memory.h"
#include "nvim/move.h"
#include "nvim/mouse.h"
#include "nvim/normal.h"
#include "nvim/ops.h"
#include "nvim/os_unix.h"
#include "nvim/os/os_defs.h"
#include "nvim/path.h"
#include "nvim/profile.h"
#include "nvim/quickfix.h"
#include "nvim/screen.h"
#include "nvim/state.h"
#include "nvim/strings.h"
#include "nvim/syntax.h"
#include "nvim/ui.h"
#include "nvim/version.h"
#include "nvim/window.h"
#include "nvim/shada.h"
#include "nvim/os/input.h"
#include "nvim/os/os.h"
#include "nvim/os/time.h"
#include "nvim/event/loop.h"
#include "nvim/os/signal.h"
#include "nvim/cmd_line_args.h"
#include "nvim/event/process.h"
#include "nvim/msgpack/helpers.h"
#include "nvim/msgpack/server.h"
#include "nvim/msgpack/channel.h"
#include "nvim/api/private/defs.h"
#include "nvim/api/private/helpers.h"
#include "nvim/api/private/handle.h"
#include "nvim/api/private/dispatch.h"

#include "generated/config/config.h"
#include "generated/config/gkideenvs.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "main.c.generated.h"
#endif

/// the main libuv event-loop
main_loop_st main_loop;

static char *argv0 = NULL;

static void event_init(void)
{
    loop_init(&main_loop, NULL);

    // early msgpack-rpc initialization
    rpc_init_method_table();
    rpc_helpers_init();

    // Initialize input events
    input_init();

    // Timer to wake the event loop if a timeout
    // argument is passed to 'event_poll' signals
    signal_init();

    // finish mspgack-rpc initialization
    channel_init();
    server_init();
    terminal_init();
}

char *programme_name(void)
{
    return argv0;
}

void event_teardown(void)
{
    if(!main_loop.events)
    {
        return;
    }

    multiqueue_process_events(main_loop.events);
    input_stop();
    channel_teardown();
    process_teardown(&main_loop);
    timer_teardown();
    server_teardown();
    signal_teardown();
    terminal_teardown();
    loop_close(&main_loop, true);
}

/// Performs early initialization.
///
/// Needed for unit tests. Must be called after time_init().
static void early_init(void)
{
    log_init();
    fs_init();
    handle_init();
    eval_init(); // init global variables
    init_path(programme_name());
    init_normal_cmds(); // Init the table of Normal mode commands.

#if defined(HAVE_HDR_LOCALE_H)
    // Setup to use the current locale (for ctype() and many other things).
    // NOTE: Translated messages with encodings other than latin1 will not
    // work until init_options_part_1() has been called!
    init_locale();
#endif

    // Allocate the first window and buffer.
    // Can't do anything without it, exit when it fails.
    if(!win_alloc_first())
    {
        mch_exit(kNEStatusWinAllocateFailed);
    }

    init_yank(); // init yank buffers
    alist_init(&g_arglist); // Init the argument list to empty.
    g_arglist.id = 0;

    // Find out the gkide user home directory
    if(!init_gkide_usr_home())
    {
        mch_exit(kNEStatusNoUserHome);
    }

    // Set the default values for the options.
    // NOTE: Non-latin1 translated messages are working only after this,
    // because this is where "has_mbyte" will be set, which is used by
    // msg_outtrans_len_attr().
    init_options_part_1();
    set_lang_var(); // set v:lang and v:ctype
    TIME_MSG("early_init");
}

int main(int argc, char **argv)
{
    argv0 = argv[0];

    // various parameters passed between main() and other functions.
    main_args_st params;

    uchar_kt *cwd = NULL; // current workding dir on startup
    uchar_kt *fname = NULL; // file name from command line

    time_init();
    init_cmd_line_args(&params, argc, argv);
    early_cmd_line_args_scan(&params);
    early_init();
    check_and_set_isatty(&params);
    event_init();

    // Process the command line arguments.
    // File names are put in the global argument list "g_arglist".
    cmd_line_args_parser(&params);

    // Get filename from command line, if any.
    if(g_arglist.al_ga.ga_len > 0)
    {
        fname = get_cmd_line_fname(&params, cwd);
    }

    TIME_MSG("expanding arguments");

    // open up to 3 windows
    if(params.diff_mode && params.window_count == -1)
    {
        params.window_count = 0;
    }

    // Don't redraw until much later.
    ++RedrawingDisabled;

    // When listing swap file names, don't
    // do cursor positioning et al.
    if(recoverymode && fname == NULL)
    {
        params.want_full_screen = FALSE;
    }

    setbuf(stdout, NULL);
    full_screen = true;
    check_tty(&params);

    // Set the default values for the
    // options that use Rows and Columns.
    win_init_size();

    // Set the 'diff' option now, so that it can be checked
    // for in a vimrc file. There is no buffer yet though.
    if(params.diff_mode)
    {
        diff_win_options(firstwin, FALSE);
    }

    assert(p_ch >= 0 && Rows >= p_ch && Rows - p_ch <= INT_MAX);
    cmdline_row = (int)(Rows - p_ch);
    msg_row = cmdline_row;
    screenalloc(false); // allocate screen buffers
    init_options_part_2(headless_mode);
    TIME_MSG("init_options_part_2");

    msg_scroll = TRUE;
    no_wait_return = TRUE;

    // set the default highlight groups
    init_highlight(TRUE, FALSE);
    TIME_MSG("init_highlight");

    // Set the break level after the terminal is initialized.
    debug_break_level = params.debug_break_level;

    bool reading_input = !headless_mode && (params.input_isatty
                                            || params.output_isatty
                                            || params.err_isatty);
    if(reading_input)
    {
        // One of the startup commands (arguments, sourced scripts or
        // plugins) may prompt the user, so start reading from a tty now.
        int fd = fileno(stdin);

        if(!params.input_isatty || params.edit_type == kEditTypeStdin)
        {
            // Use stderr or stdout since stdin is not a tty and/or
            // could be used to read the "-" file (eg: cat file | nvim -)
            fd = params.err_isatty ? fileno(stderr) : fileno(stdout);
        }

        input_start(fd);
    }

    // open terminals when opening files that start with term://
#define PROTO "term://"
    do_cmdline_cmd("augroup nvim_terminal");
    do_cmdline_cmd("autocmd!");
    do_cmdline_cmd("autocmd BufReadCmd " PROTO "* nested "
                   ":if !exists('b:term_title')|call termopen( "
                   // Capture the command string
                   "matchstr(expand(\"<amatch>\"), "
                   "'\\c\\m" PROTO "\\%(.\\{-}//\\%(\\d\\+:\\)\\?\\)\\?\\zs.*'), "
                   // capture the working directory
                   "{'cwd': get(matchlist(expand(\"<amatch>\"), "
                   "'\\c\\m" PROTO "\\(.\\{-}\\)//'), 1, '')})"
                   "|endif");
    do_cmdline_cmd("augroup END");
#undef PROTO

    // Reset 'loadplugins' for "-u NONE" before "--cmd" arguments.
    // Allows for setting 'loadplugins' there.
    if(params.use_nvimrc != NULL && strcmp(params.use_nvimrc, "NONE") == 0)
    {
        p_lpl = false;
    }

    // Execute --cmd arguments.
    exe_pre_commands(&params);

    // Source startup scripts.
    source_startup_scripts(&params);

    // If using the runtime (-u is not NONE), enable syntax & filetype plugins.
    if(params.use_nvimrc == NULL || strcmp(params.use_nvimrc, "NONE") != 0)
    {
        // Does ":filetype plugin indent on".
        filetype_maybe_enable();

        // Sources syntax/syntax.vim, which calls `:filetype on`.
        syn_maybe_on();
    }

    // Read all the plugin files.
    // Only when compiled with +eval, since most plugins need it.
    load_plugins();

    // Decide about window layout for diff mode after reading vimrc.
    set_window_layout(&params);

    // Recovery mode without a file name: List swap files. This uses
    // the 'dir' option, therefore it must be after the initializations.
    if(recoverymode && fname == NULL)
    {
        recover_names(NULL, TRUE, 0, NULL);
        mch_exit(kNEStatusNoRecoveryFile);
    }

    // Set a few option defaults after reading vimrc files:
    // 'title' and 'icon', Unix: 'shellpipe' and 'shellredir'.
    init_options_part_3();
    TIME_MSG("init_options_part_3");

    // "-n" argument: Disable swap file by setting 'updatecount' to 0.
    // Note that this overrides anything from a vimrc file.
    if(params.no_swap_file)
    {
        p_uc = 0;
    }

    if(curwin->w_o_curbuf.wo_rl && p_altkeymap)
    {
        p_fkmap = TRUE; // Set the Farsi keymap mode
        p_hkmap = FALSE; // Reset the Hebrew keymap mode
        curwin->w_o_curbuf.wo_arab = FALSE; // Reset the Arabic keymap mode
    }

    // Read in registers, history etc, from the ShaDa file.
    // This is where v:oldfiles gets filled.
    if(*p_shada != NUL)
    {
        shada_read_everything(NULL, false, true);
        TIME_MSG("reading ShaDa");
    }

    // It's better to make v:oldfiles an empty list than NULL.
    if(get_vim_var_list(VV_OLDFILES) == NULL)
    {
        set_vim_var_list(VV_OLDFILES, tv_list_alloc());
    }

    //"-q errorfile": Load the error file now.
    // If the error file can't be read, exit before doing anything else.
    handle_quickfix(&params);

    //Start putting things on the screen.
    // Scroll screen down before drawing over it
    // Clear screen now, so file message will not be cleared.
    starting = NO_BUFFERS;
    no_wait_return = FALSE;

    if(!exmode_active)
    {
        msg_scroll = FALSE;
    }

    // If "-" argument given: Read file from stdin.
    // Do this before starting Raw mode, because it may change things that the
    // writing end of the pipe doesn't like, e.g., in case stdin and stderr
    // are the same terminal: "cat | vim -".
    // Using autocommands here may cause trouble...
    if(params.edit_type == kEditTypeStdin && !recoverymode)
    {
        read_data_from_stdin();
    }

    if(reading_input && (need_wait_return || msg_didany))
    {
        // Since at this point there's no UI instance running yet, error
        // messages would have been printed to stdout. Before starting
        // (which can result in a alternate screen buffer being shown) we
        // need confirmation that the user has seen the messages and that
        // is done with a call to wait_return.
        TIME_MSG("waiting for return");
        wait_return(TRUE);
    }

    if(!headless_mode)
    {
        // Stop reading from input stream,
        // the UI layer will take over now.
        input_stop();
        ui_builtin_start();
    }

    setmouse(); // may start using the mouse
    ui_reset_scroll_region(); // In case Rows changed

    // Don't clear the screen when
    // starting in Ex mode, unless using the GUI.
    if(exmode_active)
    {
        must_redraw = CLEAR;
    }
    else
    {
        screenclear(); // clear screen
        TIME_MSG("clearing screen");
    }

    no_wait_return = TRUE;

    // Create the requested number of windows and edit buffers in them.
    // Also does recovery if "recoverymode" set.
    create_windows(&params);
    TIME_MSG("opening buffers");

    // clear v:swapcommand
    set_vim_var_string(VV_SWAPCOMMAND, NULL, -1);

    // Ex starts at last line of the file
    if(exmode_active)
    {
        curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
    }

    apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
    TIME_MSG("BufEnter autocommands");
    setpcmark();

    // When started with "-q errorfile" jump to first error now.
    if(params.edit_type == kEditTypeQkfx)
    {
        qf_jump(NULL, 0, 0, FALSE);
        TIME_MSG("jump to first error");
    }

    // If opened more than one window, start
    // editing files in the other windows.
    edit_buffers(&params, cwd);
    xfree(cwd);

    if(params.diff_mode)
    {
        // set options in each window for "nvim -d".
        FOR_ALL_WINDOWS_IN_TAB(wp, curtab)
        {
            diff_win_options(wp, TRUE);
        }
    }

    // Shorten any of the filenames, but only when absolute.
    shorten_fnames(FALSE);

    // Need to jump to the tag before executing the '-c command'.
    // Makes "vim -c '/return' -t main" work.
    handle_tag(params.tagname);

    // Execute any "+", "-c" and "-S" arguments.
    if(params.cmd_num > 0)
    {
        exe_commands(&params);
    }

    RedrawingDisabled = 0;
    redraw_all_later(NOT_VALID);
    no_wait_return = FALSE;
    starting = 0;

    // 'autochdir' has been postponed.
    do_autochdir();

    // start in insert mode
    if(p_im)
    {
        need_start_insertmode = TRUE;
    }

    set_vim_var_nr(VV_VIM_DID_ENTER, 1L);
    apply_autocmds(EVENT_VIMENTER, NULL, NULL, false, curbuf);
    TIME_MSG("VimEnter autocommands");

    // Adjust default register name for "unnamed" in 'clipboard'. Can only be
    // done after the clipboard is available and all initial commands that may
    // modify the 'clipboard' setting have run; i.e. just before entering the
    // main loop.
    set_reg_var(get_default_register_name());

    // When a startup script or session file setup for diff'ing and
    // scrollbind, sync the scrollbind now.
    if(curwin->w_o_curbuf.wo_diff && curwin->w_o_curbuf.wo_scb)
    {
        update_topline();
        check_scrollbind((linenum_kt)0, 0L);
        TIME_MSG("diff scrollbinding");
    }

    // If ":startinsert" command used, stuff a dummy command to be able to
    // call normal_cmd(), which will then start Insert mode.
    if(restart_edit != 0)
    {
        stuffcharReadbuff(K_NOP);
    }

    // WORKAROUND(mhi): #3023
    if(cb_flags & CB_UNNAMEDMASK)
    {
        (void)eval_has_provider("clipboard");
    }

    TIME_MSG("starting main loop");
    STATE_LOG("starting main loop");

    // Call the main command loop.
    // This never returns.
    normal_enter(false, false);

    return 0;
}

// Exit nvim properly
void exit_nvim_properly(int exitval)
{
    tabpage_st *tp;
    tabpage_st *next_tp;
    exiting = TRUE;

    // When running in Ex mode an error causes us
    // to exit with a non-zero exit code. POSIX requires
    // this, although it's not 100% clear from the standard.
    if(exmode_active)
    {
        exitval += ex_exitval;
    }

    set_vim_var_nr(VV_EXITING, exitval);

    // Position the cursor on the last screen line, below all the text
    ui_cursor_goto((int)Rows - 1, 0);

    // Optionally print hashtable efficiency.
    hash_debug_results();

    if(get_vim_var_nr(VV_DYING) <= 1)
    {
        // Trigger BufWinLeave for all windows, but only once per buffer.
        for(tp = first_tabpage; tp != NULL; tp = next_tp)
        {
            next_tp = tp->tp_next;
            FOR_ALL_WINDOWS_IN_TAB(wp, tp)
            {
                if(wp->w_buffer == NULL)
                {
                    // Autocmd must have close the buffer already, skip.
                    continue;
                }

                filebuf_st *buf = wp->w_buffer;

                if(buf->b_changedtick != -1)
                {
                    apply_autocmds(EVENT_BUFWINLEAVE,
                                   buf->b_fname,
                                   buf->b_fname,
                                   false,
                                   buf);

                    // note that we did it already
                    buf_set_changedtick(buf, -1);

                    // start all over, autocommands may mess up the lists
                    next_tp = first_tabpage;

                    break;
                }
            }
        }

        // Trigger BufUnload for buffers that are loaded
        FOR_ALL_BUFFERS(buf)
        {
            if(buf->b_ml.ml_mfp != NULL)
            {
                bufref_st bufref;
                set_bufref(&bufref, buf);

                apply_autocmds(EVENT_BUFUNLOAD,
                               buf->b_fname,
                               buf->b_fname,
                               false, buf);

                if(!bufref_valid(&bufref))
                {
                    break; // Autocmd deleted the buffer.
                }
            }
        }

        apply_autocmds(EVENT_VIMLEAVEPRE, NULL, NULL, FALSE, curbuf);
    }

    if(p_shada && *p_shada != NUL)
    {
        // Write out the registers, history, marks etc, to the ShaDa file
        shada_write_file(NULL, false);
    }

    if(get_vim_var_nr(VV_DYING) <= 1)
    {
        apply_autocmds(EVENT_VIMLEAVE, NULL, NULL, FALSE, curbuf);
    }

    profile_dump();

    if(did_emsg)
    {
        // give the user a chance to read the (error) message
        no_wait_return = FALSE;
        wait_return(FALSE);
    }

    // Position the cursor again, the autocommands may have moved it
    ui_cursor_goto((int)Rows - 1, 0);

#if defined(USE_ICONV) && defined(DYNAMIC_ICONV)
    iconv_end();
#endif

    cs_end();

    if(garbage_collect_at_exit)
    {
        garbage_collect(false);
    }

    mch_exit(exitval);
}

#if defined(HAVE_HDR_LOCALE_H)
/// Setup to use the current locale, for ctype() and many other things.
static void init_locale(void)
{
    // each part of the locale that should be modified is set according
    // to the environment variables, see '$ man setlocale' for details.
    setlocale(LC_ALL, "");

#ifdef LC_NUMERIC
    // Make sure strtod() uses a decimal point, not a comma.
    setlocale(LC_NUMERIC, "C");
#endif

    // the default local root directory for nvim, which is
    // $GKIDE_SYS_HOME/mis/language
    vim_snprintf((char *)NameBuff, MAXPATHL,
                 "%s" OS_PATH_SEP_STR "mis" OS_PATH_SEP_STR "language",
                 gkide_sys_home);

    // expand_env() doesn't work yet, because g_chartab[] is not
    // initialized yet, call vim_getenv() directly
    uchar_kt *p = (uchar_kt *)vim_getenv(ENV_GKIDE_NVIM_LOCALE);

    if(p != NULL && *p != NUL)
    {
        // user env settings comes first, overwrite
        vim_snprintf((char *)NameBuff, MAXPATHL, "%s", p);
    }

    xfree(p);

    if(!os_path_exists(NameBuff))
    {
        TIME_MSG("nvim local directory not exists");
        return; // skip bind to none exist directory
    }

    textdomain(GKIDE_NVIM_DOMAIN);
    bindtextdomain(GKIDE_NVIM_DOMAIN, (char *)NameBuff);
    INFO_MSG("nvim local bind to: %s", NameBuff);
}
#endif

/// Many variables are in @b paramp, so that we can pass
/// it to invoked functions without a lot of arguments.
static void init_cmd_line_args(main_args_st *paramp, int argc, char **argv)
{
    memset(paramp, 0, sizeof(*paramp));
    paramp->argc = argc;
    paramp->argv = argv;
    paramp->want_full_screen = true;
    paramp->debug_break_level = -1;
    paramp->window_count = -1;

    starttime = time(NULL);
}

/// Do early check cmd line arguments.
///
/// - if found "--startuptime nvim.log", initialize global startuptime file
/// - fi found "--server [addr:port]", init nvim server address info
static void early_cmd_line_args_scan(main_args_st *paramp)
{
    // early process command line arguments:
    // --startuptime [logfile]
    // --server [addr:port]
    int opt_to_found = 2;

    for(int i = 1; i < paramp->argc && opt_to_found; i++)
    {
        const char *opt_name = paramp->argv[i];
        const char *opt_value = NULL;

        if(i + 1 < paramp->argc
           && '-' != paramp->argv[i + 1][0]
           && '+' != paramp->argv[i + 1][0])
        {
            opt_value = paramp->argv[i + 1];
        }

        if(STRICMP(opt_name, "--startuptime") == 0)
        {
            opt_to_found--;
            if(NULL != opt_value)
            {
                // startup logfile
                time_fd = mch_fopen(opt_value, "a");
                time_start("--- NVIM STARTING ---");
            }
        }
        else if(STRICMP(opt_name, "--server") == 0)
        {
            opt_to_found--;
            init_server_addr_info(opt_value);
        }
    }
}

/// Check if we have an interactive window, if it does, set flags
static void check_and_set_isatty(main_args_st *paramp)
{
    paramp->input_isatty = os_isatty(fileno(stdin));
    paramp->output_isatty = os_isatty(fileno(stdout));
    paramp->err_isatty = os_isatty(fileno(stderr));
    TIME_MSG("check_and_set_isatty");
}

/// @todo, handle multi-char
static void init_gkide_sys_home(const char *exepath)
{
    if(exepath == NULL)
    {
        TIME_MSG("GKIDE_SYS_HOME is NULL, this should be fixed");
        return;
    }

    // also copy the NUL char
    memcpy((char *)NameBuff, exepath, strlen(exepath)+1);
    char *idx = (char *)NameBuff;

    while(*idx != NUL)
    {
        idx++;
    }

    do
    {
        *idx = NUL;
        idx--; // find the first last path separator
    } while(vim_ispathsep(*idx) == FAIL);

    // check if the current running 'nvim' is in directory named 'bin'
    // - if yes, what we want, default directory layout
    // - if not, the directory layout is unknown, and just use it
    idx = idx -4;

    if(idx[0] == idx[4] && idx[1] == 'b' && idx[2] == 'i' && idx[3] == 'n')
    {
        // default GKIDE directory layout: bin, etc, plg, doc, mis
        idx[0] = NUL; // no trailing path separator
    }
    else
    {
        // not default directory layout of GKIDE
        idx[4] = NUL; // no trailing path separator
    }

    if(gkide_sys_home)
    {
        // In case we are called a second time.
        xfree(gkide_sys_home);
        gkide_sys_home = NULL;
    }

    gkide_sys_home = xstrdup((char *)NameBuff);
    vim_setenv(ENV_GKIDE_SYS_HOME, gkide_sys_home);

    INFO_MSG("$GKIDE_SYS_HOME=%s", gkide_sys_home);
}

// Sets v:progname and v:progpath.
static void init_path(const char *exename)
FUNC_ATTR_NONNULL_ALL
{
    char exepath[MAXPATHL] = { 0 };
    size_t exepathlen = MAXPATHL;

    // Make v:progpath absolute.
    if(os_exepath(exepath, &exepathlen) != 0)
    {
        // Fall back to argv[0]. Missing procfs? #6734
        path_guess_exepath(exename, exepath, sizeof(exepath));
    }

    init_gkide_sys_home(exepath);
    assert(gkide_sys_home != NULL);

    set_vim_var_string(VV_PROGPATH, exepath, -1);
    set_vim_var_string(VV_PROGNAME, (char *)path_tail((uchar_kt *)exename), -1);
}

/// Get filename from command line, if any.
static uchar_kt *get_cmd_line_fname(main_args_st *FUNC_ARGS_UNUSED_MAYBE(parmp),
                           uchar_kt *FUNC_ARGS_UNUSED_MAYBE(cwd))
{
#if !defined(HOST_OS_LINUX) && !defined(HOST_OS_MACOS)
    // Expand wildcards in file names.
    if(!parmp->literal)
    {
        cwd = xmalloc(MAXPATHL);

        if(cwd != NULL)
        {
            os_dirname(cwd, MAXPATHL);
        }

        // Temporarily add '(' and ')' to 'isfname'. These are valid
        // filename characters but are excluded from 'isfname' to make
        // "gf" work on a file name in parenthesis (e.g.: see vim.h).
        do_cmdline_cmd(":set isf+=(,)");
        alist_expand(NULL, 0);
        do_cmdline_cmd(":set isf&");

        if(cwd != NULL)
        {
            os_chdir((char *)cwd);
        }
    }
#endif
    return alist_name(&garg_list[0]);
}

/// Decide about window layout for diff mode after reading vimrc.
static void set_window_layout(main_args_st *paramp)
{
    if(paramp->diff_mode && paramp->window_layout == 0)
    {
        if(diffopt_horizontal())
        {
            // use horizontal split
            paramp->window_layout = kWinLayoutHorizontal;
        }
        else
        {
            // use vertical split
            paramp->window_layout = kWinLayoutVertical;
        }
    }
}

/// Read all the plugin files.
/// Only when compiled with +eval, since most plugins need it.
static void load_plugins(void)
{
    if(p_lpl)
    {
        source_runtime((uchar_kt *)"plugin/**/*.vim", DIP_ALL | DIP_NOAFTER);
        TIME_MSG("loading plugins");

        ex_packloadall(NULL);
        TIME_MSG("loading packages");

        source_runtime((uchar_kt *)"plugin/**/*.vim", DIP_ALL | DIP_AFTER);
        TIME_MSG("loading after plugins");
    }
}

/// "-q errorfile": Load the error file now.
/// If the error file can't be read, exit before doing anything else.
static void handle_quickfix(main_args_st *paramp)
{
    if(paramp->edit_type == kEditTypeQkfx)
    {
        if(paramp->err_file != NULL)
        {
            set_string_option_direct((uchar_kt *)"ef",
                                     -1,
                                     paramp->err_file,
                                     OPT_FREE,
                                     SID_CARG);
        }

        vim_snprintf((char *)IObuff, IOSIZE, "cfile %s", p_ef);

        if(qf_init(NULL, p_ef, p_efm, true, IObuff) < 0)
        {
            ui_linefeed();
            mch_exit(kNEStatusQuickFixInitErr);
        }

        TIME_MSG("reading errorfile");
    }
}

/// Need to jump to the tag before executing the '-c command'.
/// Makes "vim -c '/return' -t main" work.
static void handle_tag(uchar_kt *tagname)
{
    if(tagname != NULL)
    {
        swap_exists_did_quit = FALSE;
        vim_snprintf((char *)IObuff, IOSIZE, "ta %s", tagname);
        do_cmdline_cmd((char *)IObuff);

        TIME_MSG("jumping to tag");

        // If the user doesn't want to edit the file then we quit here.
        if(swap_exists_did_quit)
        {
            exit_nvim_properly(kNEStatusFailure);
        }
    }
}

/// Print a warning if stdout is not a terminal.
///
/// When starting in Ex mode and commands come from a file, set Silent mode.
static void check_tty(main_args_st *parmp)
{
    if(headless_mode)
    {
        return;
    }

    // is active input a terminal?
    if(exmode_active)
    {
        if(!parmp->input_isatty)
        {
            silent_mode = true;
        }
    }

    if(!parmp->want_full_screen)
    {
        return;
    }

    if(!parmp->err_isatty
       && (!parmp->output_isatty || !parmp->input_isatty))
    {
        if(!parmp->output_isatty)
        {
            mch_errmsg(_("Vim: Warning: Output is not to a terminal\n"));
        }

        if(!parmp->input_isatty)
        {
            mch_errmsg(_("Vim: Warning: Input is not from a terminal\n"));
        }

        ui_flush();

        if(scriptin[0] == NULL)
        {
            os_delay(2000L, true);
        }

        TIME_MSG("Warning delay");
    }
}

/// Read text from stdin.
static void read_data_from_stdin(void)
{
    int i;
    // When getting the ATTENTION prompt here, use a dialog
    swap_exists_action = SEA_DIALOG;
    no_wait_return = TRUE;
    i = msg_didany;

    set_buflisted(TRUE);

    // create memfile and read file
    (void)open_buffer(TRUE, NULL, 0);

    no_wait_return = FALSE;
    msg_didany = i;

    TIME_MSG("reading stdin");

    check_swap_exists_action();
}

/// Create the requested number of windows and edit buffers in them.
/// Also does recovery if "recoverymode" set.
static void create_windows(main_args_st *parmp)
{
    int dorewind;
    int done = 0;

    // Create the number of windows that was requested.
    if(parmp->window_count == -1)
    {
        parmp->window_count = 1; // was not set
    }

    if(parmp->window_count == 0)
    {
        parmp->window_count = g_arglist.al_ga.ga_len;
    }

    if(parmp->window_count > 1)
    {
        // Don't change the windows if there was a
        // command in vimrc that already split some windows
        if(parmp->window_layout == 0)
        {
            parmp->window_layout = kWinLayoutHorizontal;
        }

        if(parmp->window_layout == kWinLayoutTabpage)
        {
            parmp->window_count = make_tabpages(parmp->window_count);
            TIME_MSG("making tab pages");
        }
        else if(firstwin->w_next == NULL)
        {
            parmp->window_count =
                make_windows(parmp->window_count,
                             parmp->window_layout == kWinLayoutVertical);

            TIME_MSG("making windows");
        }
        else
        {
            parmp->window_count = win_count();
        }
    }
    else
    {
        parmp->window_count = 1;
    }

    // do recover
    if(recoverymode)
    {
        msg_scroll = TRUE; // scroll message up
        ml_recover();

        if(curbuf->b_ml.ml_mfp == NULL)
        {
            exit_nvim_properly(kNEStatusFailure);
        }

        do_modelines(0); // do modelines
    }
    else
    {
        // Open a buffer for windows that don't have one yet.
        // Commands in the vimrc might have loaded a file or split
        // the window. Watch out for autocommands that delete a window.
        //
        // Don't execute Win/Buf Enter/Leave autocommands here
        ++autocmd_no_enter;
        ++autocmd_no_leave;
        dorewind = TRUE;

        while(done++ < 1000)
        {
            if(dorewind)
            {
                if(parmp->window_layout == kWinLayoutTabpage)
                {
                    goto_tabpage(1);
                }
                else
                {
                    curwin = firstwin;
                }
            }
            else if(parmp->window_layout == kWinLayoutTabpage)
            {
                if(curtab->tp_next == NULL)
                {
                    break;
                }

                goto_tabpage(0);
            }
            else
            {
                if(curwin->w_next == NULL)
                {
                    break;
                }

                curwin = curwin->w_next;
            }

            dorewind = FALSE;
            curbuf = curwin->w_buffer;

            if(curbuf->b_ml.ml_mfp == NULL)
            {
                // Set 'foldlevel' to 'foldlevelstart' if it's not negative.
                if(p_fdls >= 0)
                {
                    curwin->w_o_curbuf.wo_fdl = p_fdls;
                }

                // When getting the ATTENTION prompt here, use a dialog
                swap_exists_action = SEA_DIALOG;
                set_buflisted(TRUE);

                // create memfile, read file
                (void)open_buffer(FALSE, NULL, 0);

                if(swap_exists_action == SEA_QUIT)
                {
                    if(got_int || only_one_window())
                    {
                        // abort selected or quit and only one window
                        did_emsg = FALSE; // avoid hit-enter prompt
                        exit_nvim_properly(kNEStatusFailure);
                    }

                    // We can't close the window, it would disturb what
                    // happens next. Clear the file name and set the arg
                    // index to -1 to delete it later.
                    setfname(curbuf, NULL, NULL, FALSE);
                    curwin->w_arg_idx = -1;
                    swap_exists_action = SEA_NONE;
                }
                else
                {
                    handle_swap_exists(NULL);
                }

                dorewind = TRUE; // start again
            }

            os_breakcheck();

            if(got_int)
            {
                // only break the file loading, not the rest
                (void)vgetc();
                break;
            }
        }

        if(parmp->window_layout == kWinLayoutTabpage)
        {
            goto_tabpage(1);
        }
        else
        {
            curwin = firstwin;
        }

        curbuf = curwin->w_buffer;
        --autocmd_no_enter;
        --autocmd_no_leave;
    }
}

/// If opened more than one window, start editing files in the other
/// windows. make_windows() has already opened the windows.
static void edit_buffers(main_args_st *parmp, uchar_kt *cwd)
{
    int i;
    int arg_idx; // index in argument list
    int advance = TRUE;
    win_st *win;

    // Don't execute Win/Buf Enter/Leave autocommands here
    ++autocmd_no_enter;
    ++autocmd_no_leave;

    // When w_arg_idx is -1 remove the window (see create_windows()).
    if(curwin->w_arg_idx == -1)
    {
        win_close(curwin, TRUE);
        advance = FALSE;
    }

    arg_idx = 1;

    for(i = 1; i < parmp->window_count; ++i)
    {
        if(cwd != NULL)
        {
            os_chdir((char *)cwd);
        }

        // When w_arg_idx is -1 remove the
        // window (see create_windows()).
        if(curwin->w_arg_idx == -1)
        {
            ++arg_idx;
            win_close(curwin, TRUE);
            advance = FALSE;
            continue;
        }

        if(advance)
        {
            if(parmp->window_layout == kWinLayoutTabpage)
            {
                if(curtab->tp_next == NULL)
                {
                    break; // just checking
                }

                goto_tabpage(0);
            }
            else
            {
                if(curwin->w_next == NULL)
                {
                    break; // just checking
                }

                win_enter(curwin->w_next, false);
            }
        }

        advance = TRUE;

        // Only open the file if there is no file in this window
        // yet (that can happen when vimrc contains ":sall").
        if(curbuf == firstwin->w_buffer || curbuf->b_ffname == NULL)
        {
            curwin->w_arg_idx = arg_idx;

            // Edit file from arg list, if there is one. When
            // "Quit" selected at the ATTENTION prompt close the window.
            swap_exists_did_quit = FALSE;

            (void)do_ecmd(0,
                          arg_idx < g_arglist.al_ga.ga_len
                          ? alist_name(&garg_list[arg_idx]) : NULL,
                          NULL,
                          NULL,
                          ECMD_LASTL,
                          ECMD_HIDE,
                          curwin);

            if(swap_exists_did_quit)
            {
                // abort or quit selected
                if(got_int || only_one_window())
                {
                    // abort selected and only one window
                    did_emsg = FALSE; // avoid hit-enter prompt
                    exit_nvim_properly(kNEStatusFailure);
                }

                win_close(curwin, TRUE);
                advance = FALSE;
            }

            if(arg_idx == g_arglist.al_ga.ga_len - 1)
            {
                arg_had_last = TRUE;
            }

            ++arg_idx;
        }

        os_breakcheck();

        if(got_int)
        {
            // only break the file
            // loading, not the rest
            (void)vgetc();
            break;
        }
    }

    if(parmp->window_layout == kWinLayoutTabpage)
    {
        goto_tabpage(1);
    }

    --autocmd_no_enter;

    // make the first window the current window
    win = firstwin;

    // Avoid making a preview window the current window.
    while(win->w_o_curbuf.wo_pvw)
    {
        win = win->w_next;

        if(win == NULL)
        {
            win = firstwin;
            break;
        }
    }

    win_enter(win, false);
    --autocmd_no_leave;

    TIME_MSG("editing files in windows");

    if(parmp->window_count > 1 && parmp->window_layout != kWinLayoutTabpage)
    {
        win_equal(curwin, false, 'b'); // adjust heights
    }
}

/// Execute the commands from --cmd arguments "cmds[cnt]".
static void exe_pre_commands(main_args_st *parmp)
{
    int i;
    int cnt = parmp->pre_cmd_num;
    char **cmds = parmp->pre_cmd_args;

    if(cnt > 0)
    {
        curwin->w_cursor.lnum = 0; // just in case..
        sourcing_name = (uchar_kt *)_("pre-vimrc command line");
        current_SID = SID_CMDARG;

        for(i = 0; i < cnt; ++i)
        {
            do_cmdline_cmd(cmds[i]);
        }

        sourcing_name = NULL;
        current_SID = 0;
        TIME_MSG("--cmd commands");
    }
}

/// Execute "+", "-c" and "-S" arguments.
static void exe_commands(main_args_st *parmp)
{
    int i;

    // We start commands on line 0, make "vim +/pat file" match a
    // pattern on line 1. But don't move the cursor when an autocommand
    // with g`" was used.
    msg_scroll = TRUE;

    if(parmp->tagname == NULL && curwin->w_cursor.lnum <= 1)
    {
        curwin->w_cursor.lnum = 0;
    }

    sourcing_name = (uchar_kt *)"command line";
    current_SID = SID_CARG;

    for(i = 0; i < parmp->cmd_num; ++i)
    {
        do_cmdline_cmd(parmp->cmd_args[i]);

        if(parmp->cmds_tofree[i])
        {
            xfree(parmp->cmd_args[i]);
        }
    }

    sourcing_name = NULL;
    current_SID = 0;

    if(curwin->w_cursor.lnum == 0)
    {
        curwin->w_cursor.lnum = 1;
    }

    if(!exmode_active)
    {
        msg_scroll = FALSE;
    }

    // When started with "-q errorfile" jump to first error again.
    if(parmp->edit_type == kEditTypeQkfx)
    {
        qf_jump(NULL, 0, 0, FALSE);
    }

    TIME_MSG("executing command arguments");
}

/// Source vimrc or do other user initialization
///
/// Does one of the following things, stops after whichever succeeds:
///
/// 1. Execution of VIMINIT environment variable.
/// 2. Sourcing user vimrc file ($XDG_CONFIG_HOME/nvim/init.vim).
/// 3. Sourcing other vimrc files ($XDG_CONFIG_DIRS[1]/nvim/init.vim, …).
/// 4. Execution of EXINIT environment variable.
///
/// @return
/// True if it is needed to attempt to source exrc file according to
/// 'exrc' option definition.
static bool do_user_initialization(void)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    bool do_exrc = p_exrc;

    if(process_env("VIMINIT", true) == OK)
    {
        do_exrc = p_exrc;
        return do_exrc;
    }

    uchar_kt *user_vimrc = (uchar_kt *)stdpaths_user_conf_subpath("init.vim");

    if(do_source(user_vimrc, true, kLoadSftNvimrc|kLoadSfsUsr) != FAIL)
    {
        do_exrc = p_exrc;

        if(do_exrc)
        {
            do_exrc = (path_full_compare((uchar_kt *)VIMRC_FILE,
                                         user_vimrc,
                                         false) != kEqualFiles);
        }

        xfree(user_vimrc);
        return do_exrc;
    }

    xfree(user_vimrc);
    char *const config_dirs = stdpaths_get_xdg_var(kXDGConfigDirs);

    if(config_dirs != NULL)
    {
        const void *iter = NULL;

        do
        {
            const char *dir;
            size_t dir_len;
            iter = vim_env_iter(':', config_dirs, iter, &dir, &dir_len);

            if(dir == NULL || dir_len == 0)
            {
                break;
            }

            const char path_tail[] = {
                'n', 'v', 'i', 'm', OS_PATH_SEP_CHAR,
                'i', 'n', 'i', 't', '.', 'v', 'i', 'm', NUL
            };

            char *vimrc = xmalloc(dir_len + sizeof(path_tail) + 1);
            memmove(vimrc, dir, dir_len);
            vimrc[dir_len] = OS_PATH_SEP_CHAR;
            memmove(vimrc + dir_len + 1, path_tail, sizeof(path_tail));

            if(do_source((uchar_kt *) vimrc, true,
                         kLoadSftNvimrc|kLoadSfsUsr) != FAIL)
            {
                do_exrc = p_exrc;

                if(do_exrc)
                {
                    do_exrc = (path_full_compare((uchar_kt *)VIMRC_FILE,
                                                 (uchar_kt *)vimrc,
                                                 false) != kEqualFiles);
                }

                xfree(vimrc);
                xfree(config_dirs);
                return do_exrc;
            }

            xfree(vimrc);
        } while(iter != NULL);

        xfree(config_dirs);
    }

    if(process_env("EXINIT", false) == OK)
    {
        do_exrc = p_exrc;
        return do_exrc;
    }

    return do_exrc;
}

/// Source startup scripts
static void source_startup_scripts(const main_args_st *const parmp)
FUNC_ATTR_NONNULL_ALL
{
    TIME_MSG("============ startup sourcing beginning ============");

    // If -u argument given, use only the
    // initializations from that file and nothing else.
    if(parmp->use_nvimrc != NULL)
    {
        if(!(strcmp(parmp->use_nvimrc, "NONE") == 0
             || strcmp(parmp->use_nvimrc, "NORC") == 0))
        {
            if(do_source((uchar_kt *)parmp->use_nvimrc,
                         FALSE, kLoadSftNvimrc|kLoadSfsUsr) != OK)
            {
                EMSG2(_("E282: Cannot read from \"%s\""), parmp->use_nvimrc);
            }
        }
    }
    else if(!silent_mode)
    {
        // Get system wide defaults, if the file name is defined.
        (void)do_source((uchar_kt *)SYSINIT_NVIMRC,
                        false, kLoadSftNvimrc|kLoadSfsSys);

        if(do_user_initialization())
        {
            // Read initialization commands from ".vimrc" or ".exrc" in
            // current directory. This is only done if the 'exrc' option is set.
            // Because of security reasons we disallow shell and write commands
            // now, except for unix if the file is owned by the user or 'secure'
            // option has been reset in environment of global "exrc" or "vimrc".
            // Only do this if VIMRC_FILE is not the same as vimrc file sourced
            // in do_user_initialization.

        #if defined(UNIX)
            // If vimrc file is not owned by user, set 'secure' mode.
            if(!file_owned(VIMRC_FILE))
        #endif
                secure = p_secure;

            if(do_source((uchar_kt *)VIMRC_FILE, true,
                         kLoadSftNvimrc|kLoadSfsUsr) == FAIL)
            {
            #if defined(UNIX)
                // if ".exrc" is not owned by user set 'secure' mode
                if(!file_owned(EXRC_FILE))
                {
                    secure = p_secure;
                }
                else
                {
                    secure = 0;
                }
            #endif
                (void)do_source((uchar_kt *)EXRC_FILE, false, kLoadSftAuto);
            }
        }

        if(secure == 2)
        {
            need_wait_return = true;
        }

        secure = 0;
    }

    did_source_startup_scripts = true;

    TIME_MSG("============ startup sourcing files end ============");
}

/// Get an environment variable, and execute it as Ex commands.
///
/// @param env         environment variable to execute
/// @param is_viminit  when true, called for VIMINIT
///
/// @return FAIL if the environment variable was not executed, OK otherwise.
static int process_env(char *env, bool is_viminit)
{
    const char *initstr = os_getenv(env);

    if(initstr != NULL)
    {
        if(is_viminit)
        {
            // check and set user configuration nvimrc env-var
            check_and_set_usrnvimrc(NULL);
        }

        uchar_kt *save_sourcing_name = sourcing_name;
        linenum_kt save_sourcing_lnum = sourcing_lnum;
        sourcing_name = (uchar_kt *)env;
        sourcing_lnum = 0;

        script_id_kt save_sid = current_SID;
        current_SID = SID_ENV;
        do_cmdline_cmd((char *)initstr);
        sourcing_name = save_sourcing_name;
        sourcing_lnum = save_sourcing_lnum;
        current_SID = save_sid;

        return OK;
    }

    return FAIL;
}

#ifdef UNIX
/// Checks if user owns file.
/// Use both uv_fs_stat() and uv_fs_lstat() through os_fileinfo() and
/// os_fileinfo_link() respectively for extra security.
static bool file_owned(const char *fname)
{
    uid_t uid = getuid();
    fileinfo_st file_info;

    bool file_owned = os_fileinfo(fname, &file_info)
                      && file_info.stat.st_uid == uid;

    bool link_owned = os_fileinfo_link(fname, &file_info)
                      && file_info.stat.st_uid == uid;

    return file_owned && link_owned;
}
#endif

/// Check the result of the ATTENTION dialog:
///
// When "Quit" selected, exit Vim.
// When "Recover" selected, recover the file.
static void check_swap_exists_action(void)
{
    if(swap_exists_action == SEA_QUIT)
    {
        exit_nvim_properly(kNEStatusFailure);
    }

    handle_swap_exists(NULL);
}
