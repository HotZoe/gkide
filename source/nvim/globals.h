/// @file nvim/globals.h

#ifndef NVIM_GLOBALS_H
#define NVIM_GLOBALS_H

#include <stdbool.h>
#include <inttypes.h>

#include "nvim/macros.h"
#include "nvim/ex_eval.h"
#include "nvim/iconv.h"
#include "nvim/mbyte.h"
#include "nvim/menu.h"
#include "nvim/syntax_defs.h"
#include "nvim/types.h"
#include "nvim/event/loop.h"
#include "nvim/os/os_defs.h"

#include "config.h"
#include "envdefs.h"

#define GKIDE_SYS_HOME_BIN  "$" ENV_GKIDE_SYS_HOME OS_PATH_SEP_STR "bin"
#define GKIDE_SYS_HOME_ETC  "$" ENV_GKIDE_SYS_HOME OS_PATH_SEP_STR "etc"
#define GKIDE_SYS_HOME_DOC  "$" ENV_GKIDE_SYS_HOME OS_PATH_SEP_STR "doc"
#define GKIDE_SYS_HOME_PLG  "$" ENV_GKIDE_SYS_HOME OS_PATH_SEP_STR "plg"
#define GKIDE_SYS_HOME_MIS  "$" ENV_GKIDE_SYS_HOME OS_PATH_SEP_STR "mis"

#define SYS_PLG_SYNTAX_DIR  GKIDE_SYS_HOME_PLG OS_PATH_SEP_STR "syntax"
#define SYS_SYNTAX_FNS_NVL  SYS_PLG_SYNTAX_DIR OS_PATH_SEP_STR "%s.nvl"

#define GKIDE_USR_HOME_ETC  "$" ENV_GKIDE_USR_HOME OS_PATH_SEP_STR "etc"

#define SYSINIT_NVIMRC  GKIDE_SYS_HOME_ETC OS_PATH_SEP_STR "sysinit.nvimrc"
#define USRINIT_NVIMRC  GKIDE_USR_HOME_ETC OS_PATH_SEP_STR "usrinit.nvimrc"

/// file I/O and sprintf buffer size
#define IOSIZE         (1024+1)

/// maximum value for 'maxcombine'
#define MAX_MCO        (6)

/// length of buffer for small messages
#define MSG_BUF_LEN    (480)

/// cell length, worst case: utf-8 takes 6 bytes for one cell
#define MSG_BUF_CLEN   (MSG_BUF_LEN / 6)

#ifndef FILETYPE_FILE
    #define FILETYPE_FILE  "filetype.vim"
#endif

#ifndef FTPLUGIN_FILE
    #define FTPLUGIN_FILE  "ftplugin.vim"
#endif

#ifndef INDENT_FILE
    #define INDENT_FILE    "indent.vim"
#endif

#ifndef FTOFF_FILE
    #define FTOFF_FILE     "ftoff.vim"
#endif

#ifndef FTPLUGOF_FILE
    #define FTPLUGOF_FILE  "ftplugof.vim"
#endif

#ifndef INDOFF_FILE
    #define INDOFF_FILE    "indoff.vim"
#endif

#define DFLT_ERRORFILE     "errors.err"

#ifndef DFLT_HELPFILE
    #define DFLT_HELPFILE  "$VIMRUNTIME" OS_PATH_SEP_STR "doc" OS_PATH_SEP_STR "help.txt"
#endif

#ifndef EXRC_FILE
    #define EXRC_FILE      ".exrc"
#endif

#ifndef VIMRC_FILE
    #define VIMRC_FILE     ".nvimrc"
#endif

typedef enum
{
    kNone  = -1,
    kFalse = 0,
    kTrue  = 1,
} TriState;

/// nvim running status
typedef enum
{
    /// startup/exit has finished, normal status
    kRS_Normal  = 0,

    /// startup not finished: needs to update the screen
    kRS_Screens = 1,

    /// startup not finished: needs to run auto cmds
    kRS_Autocmd = 2,

    /// startup not finished: needs to load buffers
    kRS_Buffers = 3,

    /// startup not finished: needs to load plugins
    kRS_Plugins = 4,
} RunningStatus;

// Values for "starting"
#define NO_SCREEN    2   ///< no screen updating yet
#define NO_BUFFERS   1   ///< not all buffers loaded yet

// Number of Rows and Columns in the screen.
// Must be long to be able to use them as options in option.c.
//
// Note:
// Use screen_Rows and screen_Columns to access items in ScreenLines[].
// They may have different values when the screen wasn't (re)allocated yet
// after setting Rows or Columns (e.g., when starting up).
#define DFLT_COLS       100             ///< default value for 'columns'
#define DFLT_ROWS       35              ///< default value for 'lines'
EXTERN long Rows INIT(= DFLT_ROWS);     ///< nr of rows in the screen
EXTERN long Columns INIT(= DFLT_COLS);  ///< nr of columns in the screen

// The characters and attributes cached for the screen.
typedef uchar_kt schar_T;
typedef unsigned short sattr_T;

// The characters that are currently on the screen are kept in ScreenLines[].
// It is a single block of characters, the size of the screen plus one line.
// The attributes for those characters are kept in ScreenAttrs[].
//
// "LineOffset[n]" is the offset from ScreenLines[] for the start of line 'n'.
// The same value is used for ScreenLinesUC[] and ScreenAttrs[].
//
// Note:
// before the screen is initialized and when out of memory these can be NULL.

EXTERN schar_T *ScreenLines INIT(= NULL);
EXTERN sattr_T *ScreenAttrs INIT(= NULL);
EXTERN unsigned *LineOffset INIT(= NULL);
/// line wraps to next line
EXTERN uchar_kt *LineWraps INIT(= NULL);

// When using Unicode characters (in UTF-8 encoding) the character in
// ScreenLinesUC[] contains the Unicode for the character at this position, or
// NUL when the character in ScreenLines[] is to be used (ASCII char).
// The composing characters are to be drawn on top of the original character.
// ScreenLinesC[0][off] is only to be used when ScreenLinesUC[off] != 0.
// Note: These three are only allocated when enc_utf8 is set!

/// decoded UTF-8 characters
EXTERN u8char_T *ScreenLinesUC INIT(= NULL);
/// composing characters
EXTERN u8char_T *ScreenLinesC[MAX_MCO];
/// value of p_mco used when allocating ScreenLinesC[]
EXTERN int Screen_mco INIT(= 0);

/// Only used for euc-jp: Second byte of a character
/// that starts with 0x8e. These are single-width.
EXTERN schar_T  *ScreenLines2 INIT(= NULL);

EXTERN int screen_Rows INIT(= 0);     ///< actual size of ScreenLines[]
EXTERN int screen_Columns INIT(= 0);  ///< actual size of ScreenLines[]

// When vgetc() is called, it sets mod_mask to the set of modifiers that are
// held down based on the MOD_MASK_* symbols that are read first.
EXTERN int mod_mask INIT(= 0x0); ///< current key modifiers

/// Cmdline_row is the row where the command line starts,
/// just below the last window.
///
/// When the cmdline gets longer than the available space
/// the screen gets scrolled up. After a CTRL-D (show matches),
/// after hitting ':' after "hit return", and for the :global
/// command, the command line is temporarily moved. The old position
/// is restored with the next call to update_screen().
EXTERN int cmdline_row;

EXTERN int redraw_cmdline INIT(= FALSE);  ///< cmdline must be redrawn
EXTERN int clear_cmdline INIT(= FALSE);   ///< cmdline must be cleared
EXTERN int mode_displayed INIT(= FALSE);  ///< mode is being displayed
EXTERN int cmdline_star INIT(= FALSE);    ///< cmdline is crypted
EXTERN int exec_from_reg INIT(= FALSE);   ///< executing register
EXTERN int screen_cleared INIT(= FALSE);  ///< screen has been cleared

/// When '$' is included in 'cpoptions' option set:
/// When a change command is given that deletes only part of a line, a dollar
/// is put at the end of the changed text. dollar_vcol is set to the virtual
/// column of this '$'.  -1 is used to indicate no $ is being displayed.
EXTERN colnr_T dollar_vcol INIT(= -1);

// Variables for Insert mode completion.

/// Length in bytes of the text being completed
/// (this is deleted to be replaced by the match.)
EXTERN int compl_length INIT(= 0);

/// Set when character typed while looking for matches
/// and it means we should stop looking for matches.
EXTERN int compl_interrupted INIT(= FALSE);

/// Set when doing something for completion that may call edit() recursively,
/// which is not allowed. Also used to disable folding during completion
EXTERN int compl_busy INIT(= false);

// List of flags for method of completion.
EXTERN int compl_cont_status INIT(= 0);

#define CONT_ADDING    1       ///< "normal" or "adding" expansion
#define CONT_INTRPT    (2 + 4) ///< a ^X interrupted the current expansion
                               ///< it's set only iff N_ADDS is set
#define CONT_N_ADDS    4       ///< next ^X<> will add-new or expand-current
#define CONT_S_IPOS    8       ///< next ^X<> will set initial_pos?
                               ///< if so, word-wise-expansion will set SOL
#define CONT_SOL       16      ///< pattern includes start of line, just for
                               ///< word-wise expansion, not set for ^X^L
#define CONT_LOCAL     32      ///< for ctrl_x_mode 0, ^X^P/^X^N do a local
                               ///< expansion, (eg use complete=.)

// Functions for putting characters in the command
// line, while keeping ScreenLines[] updated.
EXTERN int msg_col;
EXTERN int msg_row;

/// cmdline is drawn right to left
EXTERN int cmdmsg_rl INIT(= FALSE);

/// Number of screen lines that windows
/// have scrolled because of printing messages.
EXTERN int msg_scrolled;

/// when TRUE don't set need_wait_return
/// in msg_puts_attr() when msg_scrolled is non-zero
EXTERN int msg_scrolled_ign INIT(= FALSE);

EXTERN uchar_kt *keep_msg INIT(= NULL);    ///< msg to be shown after redraw
EXTERN int keep_msg_attr INIT(= 0);      ///< highlight attr for keep_msg
EXTERN int keep_msg_more INIT(= FALSE);  ///< keep_msg was set by msgmore"()"
EXTERN int need_fileinfo INIT(= FALSE);  ///< do fileinfo"()" after redraw
EXTERN int msg_scroll INIT(= FALSE);     ///< msg_start"()" will scroll
EXTERN int msg_didout INIT(= FALSE);     ///< msg_outstr"()" was used in line
EXTERN int msg_didany INIT(= FALSE);     ///< msg_outstr"()" was used at all
EXTERN int msg_nowait INIT(= FALSE);     ///< don't wait for this msg

/// don't display errors for now, unless 'debug' is set
EXTERN int emsg_off INIT(= 0);

/// printing informative message
EXTERN int info_message INIT(= FALSE);

/// don't add messages to history
EXTERN int msg_hist_off INIT(= FALSE);

/// need to clear text before displaying a message
EXTERN int need_clr_eos INIT(= FALSE);

/// don't display errors for expression that is skipped
EXTERN int emsg_skip INIT(= 0);

/// use message of next of several emsg() calls for throw
EXTERN int emsg_severe INIT(= FALSE);

/// just had ":endif"
EXTERN int did_endif INIT(= FALSE);

/// Dictionary with v: variables
EXTERN dict_T vimvardict;

/// Dictionary with g: variables
EXTERN dict_T globvardict;

/// set by emsg() when the message is displayed or thrown
EXTERN int did_emsg;

/// did_emsg set because of a syntax error
EXTERN int did_emsg_syntax;

/// always set by emsg()
EXTERN int called_emsg;

/// exit value for ex mode
EXTERN int ex_exitval INIT(= 0);

/// there is an error message
EXTERN int emsg_on_display INIT(= FALSE);

/// vim_regcomp() called emsg()
EXTERN int rc_did_emsg INIT(= FALSE);

/// don't wait for return for now
EXTERN int no_wait_return INIT(= 0);

/// need to wait for return later
EXTERN int need_wait_return INIT(= 0);

/// wait_return() was used and nothing written since then
EXTERN int did_wait_return INIT(= FALSE);

/// call maketitle() soon
EXTERN int need_maketitle INIT(= TRUE);

/// 'q' hit at "--more--" msg
EXTERN int quit_more INIT(= FALSE);

#if defined(UNIX) || defined(MACOS_X)
    EXTERN int newline_on_exit INIT(= FALSE); ///< did msg in altern. screen
    EXTERN int intr_char INIT(= 0);           ///< extra interrupt character
#endif

EXTERN int ex_keep_indent INIT(= FALSE);  ///< getexmodeline(): keep indent
EXTERN int vgetc_busy INIT(= 0);          ///< when inside vgetc() then > 0

EXTERN int didset_vim INIT(= FALSE);        ///< did set $VIM ourselves
EXTERN int didset_vimruntime INIT(= FALSE); ///< idem for $VIMRUNTIME

// Lines left before a "more" message.
// Ex mode needs to be able to reset this after you type something.

/// lines left for listing
EXTERN int lines_left INIT(= -1);
/// don't use more prompt, truncate messages
EXTERN int msg_no_more INIT(= FALSE);

EXTERN uchar_kt *sourcing_name INIT(= NULL); ///< name of error message source
EXTERN linenr_T sourcing_lnum INIT(= 0);   ///< line number of the source file

EXTERN int ex_nesting_level INIT(= 0);     ///< nesting level
EXTERN int debug_break_level INIT(= -1);   ///< break below this level
EXTERN int debug_did_msg INIT(= false);    ///< did "debug mode" message
EXTERN int debug_tick INIT(= 0);           ///< breakpoint change count
EXTERN int debug_backtrace_level INIT(= 0);///< breakpoint backtrace level

// Values for do_profiling()
#define PROF_NONE       0    ///< profiling not started
#define PROF_YES        1    ///< profiling busy
#define PROF_PAUSED     2    ///< profiling paused

EXTERN int do_profiling INIT(= PROF_NONE); ///< PROF_ values

/// The exception currently being thrown. Used to pass an exception to
/// a different cstack. Also used for discarding an exception before it is
/// caught or made pending. Only valid when did_throw is TRUE.
EXTERN except_T *current_exception;

/// did_throw: An exception is being thrown.
/// Reset when the exception is caught or as long
/// as it is pending in a finally clause.
EXTERN int did_throw INIT(= FALSE);

/// need_rethrow: set to TRUE when a throw that cannot be handled
/// in do_cmdline() must be propagated to the cstack of the previously
/// called do_cmdline().
EXTERN int need_rethrow INIT(= FALSE);

/// check_cstack: set to TRUE when a ":finish" or ":return" that cannot be
/// handled in do_cmdline() must be propagated to the cstack of the previously
/// called do_cmdline().
EXTERN int check_cstack INIT(= FALSE);

/// Number of nested try conditionals
/// (across function calls and ":source" commands).
EXTERN int trylevel INIT(= 0);

/// When "force_abort" is TRUE, always skip commands after an error message,
/// even after the outermost ":endif", ":endwhile" or ":endfor" or for a
/// function without the "abort" flag. It is set to TRUE when "trylevel" is
/// non-zero (and ":silent!" was not used) or an exception is being thrown at
/// the time an error is detected. It is set to FALSE when "trylevel" gets
/// zero again and there was no error or interrupt or throw.
EXTERN int force_abort INIT(= FALSE);

/// "msg_list" points to a variable in the stack of do_cmdline() which keeps
/// the list of arguments of several emsg() calls, one of which is to be
/// converted to an error exception immediately after the failing command
/// returns. The message to be used for the exception value is pointed to by
/// the "throw_msg" field of the first element in the list.  It is usually the
/// same as the "msg" field of that element, but can be identical to the "msg"
/// field of a later list element, when the "emsg_severe" flag was set when the
/// emsg() call was made.
EXTERN struct msglist **msg_list INIT(= NULL);

/// suppress_errthrow: When TRUE, don't convert an error to an exception. Used
/// when displaying the interrupt message or reporting an exception that is still
/// uncaught at the top level (which has already been discarded then). Also used
/// for the error message when no exception can be thrown.
EXTERN int suppress_errthrow INIT(= FALSE);

/// The stack of all caught and not finished exceptions. The exception on the
/// top of the stack is the one got by evaluation of v:exception. The complete
/// stack of all caught and pending exceptions is embedded in the various
/// cstacks; the pending exceptions, however, are not on the caught stack.
EXTERN except_T *caught_stack INIT(= NULL);

// Garbage collection can only take place when we are sure there are no Lists
// or Dictionaries being used internally. This is flagged with:
// - may_garbage_collect when we are at the toplevel.
// - want_garbage_collect is set by the garbagecollect() function, which means
// we do garbage collection before waiting for a char at the toplevel.
// - garbage_collect_at_exit indicates garbagecollect(1) was called.
EXTERN int may_garbage_collect INIT(= FALSE);
EXTERN int want_garbage_collect INIT(= FALSE);
EXTERN int garbage_collect_at_exit INIT(= FALSE);

// Special values for current_SID.
#define SID_MODELINE    -1   ///< when using a modeline
#define SID_CMDARG      -2   ///< for "--cmd" argument
#define SID_CARG        -3   ///< for "-c" argument
#define SID_ENV         -4   ///< for sourcing environment variable
#define SID_ERROR       -5   ///< option was reset because of an error
#define SID_NONE        -6   ///< don't set scriptID

/// ID of script being sourced or was sourced to define the current function.
EXTERN scid_T current_SID INIT(= 0);

// Scope information for the code that indirectly triggered the current
// provider function call
EXTERN struct caller_scope
{
    scid_T SID;
    uint8_t *sourcing_name;
    uint8_t *autocmd_fname;
    uint8_t *autocmd_match;
    linenr_T sourcing_lnum;
    int autocmd_fname_full;
    int autocmd_bufnr;
    void *funccalp;
} provider_caller_scope;

EXTERN int provider_call_nesting INIT(= 0);

/// int value of T_CCO
EXTERN int t_colors INIT(= 256);

// When highlight_match is TRUE, highlight a match, starting at the cursor
// position. Search_match_lines is the number of lines after the match (0 for
// a match within one line), search_match_endcol the column number of the
// character just after the match in the last line.

EXTERN int highlight_match INIT(= FALSE); ///< show search match pos
EXTERN linenr_T search_match_lines;       ///< lines of of matched string
EXTERN colnr_T search_match_endcol;       ///< col nr of match end

/// don't use 'smartcase' once
EXTERN int no_smartcase INIT(= FALSE);
/// need to check file timestamps asap
EXTERN int need_check_timestamps INIT(= FALSE);
/// did check timestamps recently
EXTERN int did_check_timestamps INIT(= FALSE);
/// Don't check timestamps
EXTERN int no_check_timestamps INIT(= 0);

/// Values for index in highlight_attr[].
/// When making changes, also update hlf_names below!
typedef enum
{
    HLF_8 = 0,       ///< Meta & special keys listed with ":map", text that is
                     ///< displayed different from what it is
    HLF_EOB,         ///< after the last line in the buffer
    HLF_TERM,        ///< terminal cursor focused
    HLF_TERMNC,      ///< terminal cursor unfocused
    HLF_AT,          ///< @ characters at end of screen, characters that
                     ///< don't really exist in the text
    HLF_D,           ///< directories in CTRL-D listing
    HLF_E,           ///< error messages
    HLF_I,           ///< incremental search
    HLF_L,           ///< last search string
    HLF_M,           ///< "--More--" message
    HLF_CM,          ///< Mode (e.g., "-- INSERT --"), see @b kInsertMode
    HLF_N,           ///< line number for ":number" and ":#" commands
    HLF_CLN,         ///< current line number
    HLF_R,           ///< return to continue message and yes/no questions
    HLF_S,           ///< status lines
    HLF_SNC,         ///< status lines of not-current windows
    HLF_C,           ///< column to separate vertically split windows
    HLF_T,           ///< Titles for output from ":set all", ":autocmd" etc.
    HLF_V,           ///< Visual mode
    HLF_VNC,         ///< Visual mode, autoselecting and not clipboard owner
    HLF_W,           ///< warning messages
    HLF_WM,          ///< Wildmenu highlight
    HLF_FL,          ///< Folded line
    HLF_FC,          ///< Fold column
    HLF_ADD,         ///< Added diff line
    HLF_CHD,         ///< Changed diff line
    HLF_DED,         ///< Deleted diff line
    HLF_TXD,         ///< Text Changed in diff line
    HLF_SC,          ///< Sign column
    HLF_CONCEAL,     ///< Concealed text
    HLF_SPB,         ///< SpellBad
    HLF_SPC,         ///< SpellCap
    HLF_SPR,         ///< SpellRare
    HLF_SPL,         ///< SpellLocal
    HLF_PNI,         ///< popup menu normal item
    HLF_PSI,         ///< popup menu selected item
    HLF_PSB,         ///< popup menu scrollbar
    HLF_PST,         ///< popup menu scrollbar thumb
    HLF_TP,          ///< tabpage line
    HLF_TPS,         ///< tabpage line selected
    HLF_TPF,         ///< tabpage line filler
    HLF_CUC,         ///< 'cursurcolumn'
    HLF_CUL,         ///< 'cursurline'
    HLF_MC,          ///< 'colorcolumn'
    HLF_QFL,         ///< selected quickfix line
    HLF_0,           ///< Whitespace
    HLF_INACTIVE,    ///< NormalNC: Normal text in non-current windows
    HLF_COUNT        ///< MUST be the last one
} hlf_T;

EXTERN const char *hlf_names[] INIT(=
{
    [HLF_8] =         "SpecialKey",
    [HLF_EOB] =       "EndOfBuffer",
    [HLF_TERM] =      "TermCursor",
    [HLF_TERMNC] =    "TermCursorNC",
    [HLF_AT] =        "NonText",
    [HLF_D] =         "Directory",
    [HLF_E] =         "ErrorMsg",
    [HLF_I] =         "IncSearch",
    [HLF_L] =         "Search",
    [HLF_M] =         "MoreMsg",
    [HLF_CM] =        "ModeMsg",
    [HLF_N] =         "LineNr",
    [HLF_CLN] =       "CursorLineNr",
    [HLF_R] =         "Question",
    [HLF_S] =         "StatusLine",
    [HLF_SNC] =       "StatusLineNC",
    [HLF_C] =         "VertSplit",
    [HLF_T] =         "Title",
    [HLF_V] =         "Visual",
    [HLF_VNC] =       "VisualNOS",
    [HLF_W] =         "WarningMsg",
    [HLF_WM] =        "WildMenu",
    [HLF_FL] =        "Folded",
    [HLF_FC] =        "FoldColumn",
    [HLF_ADD] =       "DiffAdd",
    [HLF_CHD] =       "DiffChange",
    [HLF_DED] =       "DiffDelete",
    [HLF_TXD] =       "DiffText",
    [HLF_SC] =        "SignColumn",
    [HLF_CONCEAL] =   "Conceal",
    [HLF_SPB] =       "SpellBad",
    [HLF_SPC] =       "SpellCap",
    [HLF_SPR] =       "SpellRare",
    [HLF_SPL] =       "SpellLocal",
    [HLF_PNI] =       "Pmenu",
    [HLF_PSI] =       "PmenuSel",
    [HLF_PSB] =       "PmenuSbar",
    [HLF_PST] =       "PmenuThumb",
    [HLF_TP] =        "TabLine",
    [HLF_TPS] =       "TabLineSel",
    [HLF_TPF] =       "TabLineFill",
    [HLF_CUC] =       "CursorColumn",
    [HLF_CUL] =       "CursorLine",
    [HLF_MC] =        "ColorColumn",
    [HLF_QFL] =       "QuickFixLine",
    [HLF_0] =         "Whitespace",
    [HLF_INACTIVE] =  "NormalNC",
});


EXTERN int highlight_attr[HLF_COUNT];  ///< Highl. attr for each context.
EXTERN int highlight_user[9];          ///< User[1-9] attributes
EXTERN int highlight_stlnc[9];         ///< On top of user

EXTERN int cterm_normal_fg_color INIT(= 0);
EXTERN int cterm_normal_fg_bold INIT(= 0);
EXTERN int cterm_normal_bg_color INIT(= 0);

EXTERN RgbValue normal_fg INIT(= -1);
EXTERN RgbValue normal_bg INIT(= -1);
EXTERN RgbValue normal_sp INIT(= -1);

EXTERN int autocmd_busy INIT(= FALSE);      ///< Is apply_autocmds() busy?
EXTERN int autocmd_no_enter INIT(= FALSE);  ///< *Enter autocmds disabled
EXTERN int autocmd_no_leave INIT(= FALSE);  ///< *Leave autocmds disabled

EXTERN int modified_was_set;            ///< did ":set modified"
EXTERN int did_filetype INIT(= FALSE);  ///< FileType event found

/// value for did_filetype when starting to execute autocommands
EXTERN int keep_filetype INIT(= FALSE);

/// When deleting the current buffer, another one must be loaded.
/// If we know which one is preferred, au_new_curbuf is set to it.
EXTERN bufref_T au_new_curbuf INIT(= { NULL, 0 });

// When deleting a buffer/window and autocmd_busy is TRUE, do not free the
// buffer/window. but link it in the list starting with
// au_pending_free_buf/ap_pending_free_win, using b_next/w_next.
// Free the buffer/window when autocmd_busy is being set to FALSE.
EXTERN fbuf_st *au_pending_free_buf INIT(= NULL);
EXTERN win_st *au_pending_free_win INIT(= NULL);

// Mouse coordinates, set by check_termcode()
EXTERN int mouse_row;
EXTERN int mouse_col;

/// mouse below last line
EXTERN bool mouse_past_bottom INIT(= false);
/// mouse right of line
EXTERN bool mouse_past_eol INIT(= false);
/// extending Visual area with  mouse dragging
EXTERN int mouse_dragging INIT(= 0);

// Value set from 'diffopt'.
EXTERN int diff_context INIT(= 6);    ///< context for folds
EXTERN int diff_foldcolumn INIT(= 2); ///< 'foldcolumn' for diff mode
EXTERN int diff_need_scrollbind INIT(= FALSE);

/// The root of the menu hierarchy.
EXTERN vimmenu_T *root_menu INIT(= NULL);

/// While defining the system menu, sys_menu is TRUE. This avoids
/// overruling of menus that the user already defined.
EXTERN int sys_menu INIT(= FALSE);

/// While redrawing the screen this flag is set.
/// It means the screen size ('lines' and 'rows') must not be changed.
EXTERN int updating_screen INIT(= FALSE);

// All windows are linked in a list. firstwin points to the first entry,
// lastwin to the last entry (can be the same as firstwin) and curwin to the
// currently active window.
EXTERN win_st *firstwin;              ///< first window
EXTERN win_st *lastwin;               ///< last window
EXTERN win_st *prevwin INIT(= NULL);  ///< previous window

/// When using this macro "break" only breaks out of the inner loop.
/// Use "goto" to break out of the tabpage loop.
#define FOR_ALL_TAB_WINDOWS(tp, wp) \
    FOR_ALL_TABS(tp)                \
    FOR_ALL_WINDOWS_IN_TAB(wp, tp)

/// -V:FOR_ALL_WINDOWS_IN_TAB:501
#define FOR_ALL_WINDOWS_IN_TAB(wp, tp)  \
    for(win_st *wp = ((tp) == curtab)    \
        ? firstwin : (tp)->tp_firstwin; \
        wp != NULL;                     \
        wp = wp->w_next)

EXTERN win_st *curwin;    ///< currently active window
EXTERN win_st *aucmd_win; ///< window used in aucmd_prepbuf()
EXTERN int aucmd_win_used INIT(= FALSE); ///< aucmd_win is being used

// The window layout is kept in a tree of frames.
// topframe points to the top of the tree.
EXTERN frame_T  *topframe;  ///< top of the window frame tree

// Tab pages are alternative topframes. "first_tabpage" points to the first
// one in the list, "curtab" is the current one.
EXTERN tabpage_T *first_tabpage;
EXTERN tabpage_T *curtab;
EXTERN int redraw_tabline INIT(= FALSE); ///< need to redraw tabline

/// Iterates over all tabs in the tab list
#define FOR_ALL_TABS(tp) \
    for(tabpage_T *tp = first_tabpage; tp != NULL; tp = tp->tp_next)

// All buffers are linked in a list. 'firstbuf' points to the first entry,
// 'lastbuf' to the last entry and 'curbuf' to the currently active buffer.
EXTERN fbuf_st *firstbuf INIT(= NULL);  ///< first buffer
EXTERN fbuf_st *lastbuf INIT(= NULL);   ///< last buffer
EXTERN fbuf_st *curbuf INIT(= NULL);    ///< currently active buffer

// Iterates over all buffers in the buffer list.
#define FOR_ALL_BUFFERS(buf) \
    for(fbuf_st *buf = firstbuf; buf != NULL; buf = buf->b_next)
#define FOR_ALL_BUFFERS_BACKWARDS(buf)  \
    for(fbuf_st *buf = lastbuf;  buf != NULL; buf = buf->b_prev)

// Flag that is set when switching off 'swapfile'.
// It means that all blocks are to be loaded into memory.
// Shouldn't be global...
EXTERN int mf_dont_release INIT(= FALSE);  ///< don't release blocks

// List of files being edited (global argument list).
// curwin->w_alist points to this when the window is
// using the global argument list.

EXTERN alist_T global_alist;           ///< global argument list
EXTERN int max_alist_id INIT(= 0);     ///< the previous argument list id
EXTERN int arg_had_last INIT(= FALSE); ///< accessed last file in #global_alist

EXTERN int ru_col;  ///< column for ruler
EXTERN int ru_wid;  ///< 'rulerfmt' width of ruler when non-zero
EXTERN int sc_col;  ///< column for shown command

EXTERN RunningStatus runtime_status INIT(= kRS_Screens);

/// When starting or exiting some things are
/// done differently (e.g. screen updating).
EXTERN int starting INIT(= NO_SCREEN);

/// first NO_SCREEN, then NO_BUFFERS and then
/// set to 0 when starting up finished
EXTERN int exiting INIT(= FALSE);

/// TRUE when planning to exit Vim.
///
/// Might still keep on running if there is a changed buffer.
/// volatile because it is used in signal handler deathtrap"()"
EXTERN volatile int full_screen INIT(= false);

/// TRUE when doing full-screen output,
/// otherwise only writing some messages
EXTERN int restricted INIT(= FALSE);

/// TRUE when started in restricted mode (-Z)
EXTERN int secure INIT(= FALSE);

/// non-zero when only "safe" commands are
/// allowed, e.g. when sourcing .exrc or .vimrc
/// in current directory
EXTERN int textlock INIT(= 0);

/// non-zero when changing text and jumping to
/// another window or buffer is not allowed
EXTERN int curbuf_lock INIT(= 0);

/// non-zero when the current buffer can't be
/// changed. Used for FileChangedRO.
EXTERN int allbuf_lock INIT(= 0);

/// non-zero when no buffer name can be
/// changed, no buffer can be deleted and
/// current directory can't be changed.
/// Used for SwapExists et al.
EXTERN int sandbox INIT(= 0);

// Non-zero when evaluating an expression in a
// "sandbox". Several things are not allowed then.

/// set to TRUE when @b -s commandline argument used for ex
EXTERN int silent_mode INIT(= FALSE);

/// Set to true when sourcing of startup scripts (init.vim) is done.
/// Used for options that cannot be changed after startup scripts.
EXTERN bool did_source_startup_scripts INIT(= false);

/// start position of active Visual selection
EXTERN pos_T VIsual;

EXTERN int VIsual_active INIT(= FALSE);

/// whether Visual mode is active
EXTERN int VIsual_select INIT(= FALSE);

/// whether Select mode is active
EXTERN int VIsual_reselect;

/// whether to restart the selection after
/// a Select mode mapping or menu type of Visual mode
EXTERN int VIsual_mode INIT(= 'v');

/// TRUE when redoing Visual
EXTERN int redo_VIsual_busy INIT(= FALSE);

/// When pasting text with the middle mouse button in visual mode with
/// restart_edit set, remember where it started so we can set Insstart.
EXTERN pos_T where_paste_started;

/// This flag is used to make auto-indent work right on lines where only a
/// <RETURN> or <ESC> is typed. It is set when an auto-indent is done, and
/// reset when any other editing is done on the line. If an <ESC> or <RETURN>
/// is received, and did_ai is TRUE, the line is truncated.
EXTERN int did_ai INIT(= FALSE);

/// Column of first char after autoindent. 0 when no autoindent done. Used
/// when 'backspace' is 0, to avoid backspacing over autoindent.
EXTERN colnr_T ai_col INIT(= 0);

/// This is a character which will end a start-middle-end comment when typed as
/// the first character on a new line. It is taken from the last character of
/// the "end" comment leader when the COM_AUTO_END flag is given for that
/// comment end in 'comments'. It is only valid when did_ai is TRUE.
EXTERN int end_comment_pending INIT(= NUL);

/// This flag is set after a ":syncbind" to let the check_scrollbind() function
/// know that it should not attempt to perform scrollbinding due to the scroll
/// that was a result of the ":syncbind." (Otherwise, check_scrollbind() will
/// undo some of the work done by ":syncbind.")
EXTERN int did_syncbind INIT(= FALSE);

/// This flag is set when a smart indent has been performed.
/// When the next typed character is a '{' the inserted tab
/// will be deleted again.
EXTERN int did_si INIT(= FALSE);

/// This flag is set after an auto indent.
/// If the next typed character is a '}' one indent will be removed.
EXTERN int can_si INIT(= FALSE);

/// This flag is set after an "O" command.
/// If the next typed character is a '{' one indent will be removed.
EXTERN int can_si_back INIT(= FALSE);

/// w_cursor before formatting text.
EXTERN pos_T saved_cursor INIT(= INIT_POS_T(0, 0, 0));

// Stuff for insert mode.

/// This is where the latest insert/append mode started.
EXTERN pos_T Insstart;

/// This is where the latest insert/append mode started. In contrast to
/// Insstart, this won't be reset by certain keys and is needed for
/// op_insert(), to detect correctly where inserting by the user started.
EXTERN pos_T Insstart_orig;

// Stuff for 'kVReplaceMode' mode.

EXTERN int orig_line_count INIT(= 0);  ///< Line count when "gR" started
EXTERN int vr_lines_changed INIT(= 0); ///< #Lines changed by "gR" so far

// These flags are set based upon 'fileencoding'.
// Note that "enc_utf8" is also set for "unicode", because the characters are
// internally stored as UTF-8 (to avoid trouble with NUL bytes).
#define DBCS_JPN       932     ///< japan
#define DBCS_JPNU      9932    ///< euc-jp
#define DBCS_KOR       949     ///< korea
#define DBCS_KORU      9949    ///< euc-kr
#define DBCS_CHS       936     ///< chinese
#define DBCS_CHSU      9936    ///< euc-cn
#define DBCS_CHT       950     ///< taiwan
#define DBCS_CHTU      9950    ///< euc-tw
#define DBCS_2BYTE     1       ///< 2byte-
#define DBCS_DEBUG     -1      ///<

/// mbyte flags that used to depend on @b encoding.
/// These are now deprecated, as @b encoding is always "utf-8".
/// Code that use them can be refactored to remove dead code.
///
/// @todo, remove the dead code, to make things simple
#define enc_dbcs  false
#define enc_utf8  true
#define has_mbyte true

/// Encoding used when 'fencs' is set to "default"
EXTERN uchar_kt *fenc_default INIT(= NULL);

/// To speed up BYTELEN(); keep a lookup table to quickly get the length in
/// bytes of a UTF-8 character from the first byte of a UTF-8 string. Bytes
/// which are illegal when used as the first byte have a 1. The NUL byte has
/// length 1.
EXTERN char utf8len_tab[256] INIT(=
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1,
});

#if defined(USE_ICONV) && defined(DYNAMIC_ICONV)
// Pointers to functions and variables to be loaded at runtime
EXTERN size_t (*iconv)(iconv_t cd,
                       const char **inbuf,
                       size_t *inbytesleft,
                       char **outbuf,
                       size_t *outbytesleft);

EXTERN iconv_t (*iconv_open)(const char *tocode, const char *fromcode);
EXTERN int (*iconv_close)(iconv_t cd);
EXTERN int (*iconvctl)(iconv_t cd, int request, void *argument);
EXTERN int *(*iconv_errno)(void);
#endif

/// This is the current state(mode) of the command interpreter.
/// @b curmod is the main state of Vim.
///
/// There are other variables that modify the state:
/// - "Visual_mode"    When @b curmod is ::kNormalMode or ::kInsertMode.
/// - "finish_op"      When @b curmod is ::kNormalMode, after typing
///                    the operator and before typing the motion command.
EXTERN int curmod INIT(= kNormalMode);

EXTERN bool finish_op INIT(= false);    ///< true while an operator is pending
EXTERN long opcount INIT(= 0);          ///< count for pending operator

// ex mode (Q) state
EXTERN int exmode_active INIT(= 0);     ///< zero, EXMODE_NORMAL or EXMODE_VIM
EXTERN int ex_no_reprint INIT(= FALSE); ///< no need to print after z or p

EXTERN int Recording INIT(= FALSE);     ///< TRUE when recording into a reg.
EXTERN int Exec_reg INIT(= FALSE);      ///< TRUE when executing a register

EXTERN int no_mapping INIT(= false);    ///< currently no mapping allowed
EXTERN int no_zero_mapping INIT(= 0);   ///< mapping zero not allowed
EXTERN int no_u_sync INIT(= 0);         ///< Don't call u_sync()

/// Call u_sync() once when evaluating an expression.
EXTERN int u_sync_once INIT(= 0);

/// force restart_edit after ex_normal returns
EXTERN bool force_restart_edit INIT(= false);
/// call edit when next cmd finished
EXTERN int restart_edit INIT(= 0);

/// Normally FALSE, set to TRUE after hitting cursor key in insert mode.
/// Used by vgetorpeek() to decide when to call u_sync()
EXTERN int arrow_used;

/// put cursor after eol when restarting edit after CTRL-O
EXTERN int ins_at_eol INIT(= FALSE);
EXTERN uchar_kt *edit_submode INIT(= NULL);       ///< msg for CTRL-X submode
EXTERN uchar_kt *edit_submode_pre INIT(= NULL);   ///< prepended to edit_submode
EXTERN uchar_kt *edit_submode_extra INIT(= NULL); ///< appended to edit_submode

EXTERN hlf_T edit_submode_highl;      ///< highl. method for extra info
EXTERN int ctrl_x_mode INIT(= 0);     ///< Which Ctrl-X mode are we in?
EXTERN int no_abbr INIT(= TRUE);      ///< TRUE when no abbreviations loaded
EXTERN int mapped_ctrl_c INIT(= 0);   ///< Modes where CTRL-C is mapped.
EXTERN cmdmod_T cmdmod;               ///< Ex command modifiers

EXTERN int msg_silent INIT(= 0);         ///< don't print messages
EXTERN int emsg_silent INIT(= 0);        ///< don't print error messages
EXTERN bool emsg_noredir INIT(= false);  ///< don't redirect error messages
EXTERN int cmd_silent INIT(= false);     ///< don't echo the command line

// Values for swap_exists_action: what to do when swap file already exists
#define SEA_NONE        0   ///< don't use dialog
#define SEA_DIALOG      1   ///< use dialog when possible
#define SEA_QUIT        2   ///< quit editing the file
#define SEA_RECOVER     3   ///< recover the file

EXTERN int swap_exists_action INIT(= SEA_NONE);

/// For dialog when swap file already exists.
EXTERN int swap_exists_did_quit INIT(= FALSE);

#if MAXPATHL > IOSIZE
    #define OS_BUF_SIZE MAXPATHL
#else
    #define OS_BUF_SIZE IOSIZE
#endif

EXTERN char os_buf[OS_BUF_SIZE];       ///< Buffer for the os/ layer
EXTERN uchar_kt IObuff[IOSIZE];          ///< Buffer for sprintf, I/O, etc.
EXTERN uchar_kt NameBuff[MAXPATHL];      ///< Buffer for expanding file names
EXTERN uchar_kt msg_buf[MSG_BUF_LEN];    ///< Small buffer for messages

EXTERN int RedrawingDisabled INIT(= 0); ///< When non-zero, postpone redrawing
EXTERN int readonlymode INIT(= FALSE);  ///< Set to `TRUE` for "view"
EXTERN int recoverymode INIT(= FALSE);  ///< Set to `TRUE` for "-r" option

// typeahead buffer
EXTERN typebuf_T typebuf INIT(= { NULL, NULL, 0, 0, 0, 0, 0, 0, 0 });

EXTERN int ex_normal_busy INIT(= 0);    ///< recursiveness of ex_normal()
EXTERN int ex_normal_lock INIT(= 0);    ///< forbid use of ex_normal()
EXTERN int ignore_script INIT(= false); ///< ignore script input
EXTERN int stop_insert_mode;   ///< for ":stopinsert" and 'insertmode'
EXTERN int KeyTyped;           ///< TRUE if user typed current char
EXTERN int KeyStuffed;         ///< TRUE if current char from stuffbuf
EXTERN int maptick INIT(= 0);  ///< tick for each non-mapped char

EXTERN int must_redraw INIT(= 0);     ///< type of redraw necessary
EXTERN int skip_redraw INIT(= FALSE); ///< skip redraw once
EXTERN int do_redraw INIT(= FALSE);   ///< extra redraw once

EXTERN char *used_shada_file INIT(= NULL); ///< name of the ShaDa file to use
EXTERN int need_highlight_changed INIT(= true);

#define NSCRIPT 15
EXTERN FILE *scriptin[NSCRIPT];      ///< streams to read script from
EXTERN int curscript INIT(= 0);      ///< index in scriptin[]
EXTERN FILE *scriptout INIT(= NULL); ///< stream to write script to

// volatile because it is used in a signal handler.

/// set to true when interrupt signal occurred
EXTERN volatile int got_int INIT(= false);
/// set to TRUE with ! command
EXTERN int bangredo INIT(= FALSE);
/// length of previous search cmd
EXTERN int searchcmdlen;

/// Used when compiling regexp:
/// - REX_SET to allow \z\(...\),
/// - REX_USE to allow \z\1 et al
EXTERN int reg_do_extmatch INIT(= 0);

/// Used by vim_regexec(): strings for \z\1...\z\9
EXTERN reg_extmatch_T *re_extmatch_in INIT(= NULL);

/// Set by vim_regexec() to store \z\(...\) matches
EXTERN reg_extmatch_T *re_extmatch_out INIT(= NULL);

EXTERN int did_outofmem_msg INIT(= false);

/// set after out of memory msg
EXTERN int did_swapwrite_msg INIT(= false);

// set after swap write error msg
EXTERN int undo_off INIT(= false);      ///< undo switched off for now
EXTERN int global_busy INIT(= 0);       ///< set when :global is executing
EXTERN int listcmd_busy INIT(= false);  ///< set when :argdo, :windo or

/// :bufdo is executing
EXTERN int need_start_insertmode INIT(= false);

// start insert mode soon
EXTERN uchar_kt *last_cmdline INIT(= NULL);     ///< last command line (for ":)
EXTERN uchar_kt *repeat_cmdline INIT(= NULL);   ///< command line for "."
EXTERN uchar_kt *new_last_cmdline INIT(= NULL); ///< new value for last_cmdline
EXTERN uchar_kt *autocmd_fname INIT(= NULL);    ///< fname for <afile> on cmdline
EXTERN int autocmd_fname_full;                ///< autocmd_fname is full path
EXTERN int autocmd_bufnr INIT(= 0);           ///< fnum for <abuf> on cmdline
EXTERN uchar_kt *autocmd_match INIT(= NULL);    ///< name for <amatch> on cmdline
EXTERN int did_cursorhold INIT(= false);      ///< set when CursorHold t'gerd
EXTERN int last_changedtick INIT(= 0);        ///< for TextChanged event

/// for CursorMoved event
EXTERN pos_T last_cursormoved INIT(= INIT_POS_T(0, 0, 0));
EXTERN fbuf_st *last_changedtick_buf INIT(= NULL);

EXTERN int postponed_split INIT(= 0);       ///< for CTRL-W CTRL-] command
EXTERN int postponed_split_flags INIT(= 0); ///< args for win_split()
EXTERN int postponed_split_tab INIT(= 0);   ///< cmdmod.tab
EXTERN int g_do_tagpreview INIT(= 0);       ///< for tag preview commands:
                                            ///< height of preview window
EXTERN int replace_offset INIT(= 0);        ///< offset for replace_push()

/// need backslash in cmd line
EXTERN uchar_kt *escape_chars INIT(= (uchar_kt *)" \t\\\"|");

EXTERN int keep_help_flag INIT(= FALSE);    ///< doing :ta from help file

/// When a string option is NULL (which only happens in
/// out-of-memory situations), it is set to empty_option,
/// to avoid having to check for NULL everywhere.
EXTERN uchar_kt *empty_option INIT(= (uchar_kt *)"");

EXTERN int redir_off INIT(= false);       ///< no redirection for a moment
EXTERN FILE *redir_fd INIT(= NULL);       ///< message redirection file
EXTERN int redir_reg INIT(= 0);           ///< message redirection register
EXTERN int redir_vname INIT(= 0);         ///< message redirection variable
EXTERN garray_T *capture_ga INIT(= NULL); ///< captured output for execute()
EXTERN uchar_kt langmap_mapchar[256];       ///< mapping for language keys
EXTERN int save_p_ls INIT(= -1);          ///< Save 'laststatus' setting
EXTERN int save_p_wmh INIT(= -1);         ///< Save 'winminheight' setting
EXTERN char breakat_flags[256];           ///< which characters are in 'breakat'
EXTERN int wild_menu_showing INIT(= 0);

#define WM_SHOWN     1   ///< wildmenu showing
#define WM_SCROLLED  2   ///< wildmenu showing with scroll

extern char *gkide_sys_home;
extern char *gkide_usr_home;

/// When a window has a local directory, the absolute path of the global
/// current directory is stored here (in allocated memory). If the current
/// directory is not a local directory, globaldir is NULL.
EXTERN uchar_kt *globaldir INIT(= NULL);

// Characters from 'listchars' option
EXTERN int lcs_eol INIT(= '$');
EXTERN int lcs_ext INIT(= NUL);
EXTERN int lcs_prec INIT(= NUL);
EXTERN int lcs_nbsp INIT(= NUL);
EXTERN int lcs_space INIT(= NUL);
EXTERN int lcs_tab1 INIT(= NUL);
EXTERN int lcs_tab2 INIT(= NUL);
EXTERN int lcs_trail INIT(= NUL);
EXTERN int lcs_conceal INIT(= ' ');

// Characters from 'fillchars' option
EXTERN int fill_stl INIT(= ' ');
EXTERN int fill_stlnc INIT(= ' ');
EXTERN int fill_vert INIT(= ' ');
EXTERN int fill_fold INIT(= '-');
EXTERN int fill_diff INIT(= '-');

// Whether 'keymodel' contains "stopsel" and "startsel".
EXTERN int km_stopsel INIT(= FALSE);
EXTERN int km_startsel INIT(= FALSE);

EXTERN int cedit_key INIT(= -1);    ///< key value of 'cedit' option
EXTERN int cmdwin_type INIT(= 0);   ///< type of cmdline window or 0
EXTERN int cmdwin_result INIT(= 0); ///< result of cmdline window or 0

EXTERN uchar_kt no_lines_msg[] INIT(= N_("--No lines in buffer--"));

// When ":global" is used to number of substitutions
// and changed lines is accumulated until it's finished.
// Also used for ":spellrepall".
EXTERN long sub_nsubs;       ///< total number of substitutions
EXTERN linenr_T sub_nlines;  ///< total number of lines changed

/// table to store parsed 'wildmode'
EXTERN uchar_kt wim_flags[4];

/// whether titlestring and iconstring contains statusline syntax
#define STL_IN_ICON    1
#define STL_IN_TITLE   2

EXTERN int stl_syntax INIT(= 0);

/// don't use 'hlsearch' temporarily
EXTERN int no_hlsearch INIT(= FALSE);

/// Page number used for %N in 'pageheader' and 'guitablabel'.
EXTERN linenr_T printer_page_num;

/// received text from client or from feedkeys()
EXTERN int typebuf_was_filled INIT(= FALSE);

#ifdef BACKSLASH_IN_FILENAME
EXTERN char psepc INIT(= '\\'); ///< normal path separator character
EXTERN char psepcN INIT(= '/'); ///< abnormal path separator character
EXTERN char pseps[2] INIT(= { '\\', 0 }); ///< normal path separator string
#endif

/// Set to TRUE when an operator is being executed with virtual editing, MAYBE
/// when no operator is being executed, FALSE otherwise.
EXTERN int virtual_op INIT(= MAYBE);

/// Display tick, incremented for each call to update_screen()
EXTERN disptick_T display_tick INIT(= 0);

/// Line in which spell checking wasn't highlighted because it touched the
/// cursor position in Insert mode.
EXTERN linenr_T spell_redraw_lnum INIT(= 0);

/// Set when the cursor line needs to be redrawn.
EXTERN int need_cursor_line_redraw INIT(= FALSE);

#ifdef USE_MCH_ERRMSG
    /// Grow array to collect error messages in until they can be displayed.
    EXTERN garray_T error_ga INIT(= GA_EMPTY_INIT_VALUE);
#endif

// The error messages that can be shared are included here.
// Excluded are errors that are only used once and debugging messages.
EXTERN uchar_kt e_abort[] INIT(= N_("E470: Command aborted"));
EXTERN uchar_kt e_afterinit[] INIT(= N_("E905: Cannot set this option after startup"));
EXTERN uchar_kt e_api_spawn_failed[] INIT(= N_("E903: Could not spawn API job"));
EXTERN uchar_kt e_argreq[] INIT(= N_("E471: Argument required"));
EXTERN uchar_kt e_backslash[] INIT(= N_("E10: \\ should be followed by /, ? or &"));

EXTERN uchar_kt e_cmdwin[] INIT(=N_("E11: Invalid in command-line window; "
                                  "<CR> executes, CTRL-C quits"));

EXTERN uchar_kt e_curdir[] INIT(=N_("E12: Command not allowed from exrc/vimrc in current "
                                  "dir or tag search"));

EXTERN uchar_kt e_endif[] INIT(= N_("E171: Missing :endif"));
EXTERN uchar_kt e_endtry[] INIT(= N_("E600: Missing :endtry"));
EXTERN uchar_kt e_endwhile[] INIT(= N_("E170: Missing :endwhile"));
EXTERN uchar_kt e_endfor[] INIT(= N_("E170: Missing :endfor"));
EXTERN uchar_kt e_while[] INIT(= N_("E588: :endwhile without :while"));
EXTERN uchar_kt e_for[] INIT(= N_("E588: :endfor without :for"));
EXTERN uchar_kt e_exists[] INIT(= N_("E13: File exists (add ! to override)"));
EXTERN uchar_kt e_failed[] INIT(= N_("E472: Command failed"));
EXTERN uchar_kt e_internal[] INIT(= N_("E473: Internal error"));
EXTERN uchar_kt e_interr[] INIT(= N_("Interrupted"));
EXTERN uchar_kt e_invaddr[] INIT(= N_("E14: Invalid address"));
EXTERN uchar_kt e_invarg[] INIT(= N_("E474: Invalid argument"));
EXTERN uchar_kt e_invarg2[] INIT(= N_("E475: Invalid argument: %s"));
EXTERN uchar_kt e_invexpr2[] INIT(= N_("E15: Invalid expression: %s"));
EXTERN uchar_kt e_invrange[] INIT(= N_("E16: Invalid range"));
EXTERN uchar_kt e_invcmd[] INIT(= N_("E476: Invalid command"));
EXTERN uchar_kt e_isadir2[] INIT(= N_("E17: \"%s\" is a directory"));
EXTERN uchar_kt e_invjob[] INIT(= N_("E900: Invalid job id"));
EXTERN uchar_kt e_jobtblfull[] INIT(= N_("E901: Job table is full"));
EXTERN uchar_kt e_jobspawn[] INIT(= N_("E903: Process failed to start: %s: \"%s\""));
EXTERN uchar_kt e_jobnotpty[] INIT(= N_("E904: Job is not connected to a pty"));
EXTERN uchar_kt e_libcall[] INIT(= N_("E364: Library call failed for \"%s()\""));
EXTERN uchar_kt e_mkdir[] INIT(= N_("E739: Cannot create directory %s: %s"));
EXTERN uchar_kt e_markinval[] INIT(= N_("E19: Mark has invalid line number"));
EXTERN uchar_kt e_marknotset[] INIT(= N_("E20: Mark not set"));
EXTERN uchar_kt e_modifiable[] INIT(= N_("E21: Cannot make changes, 'modifiable' is off"));
EXTERN uchar_kt e_nesting[] INIT(= N_("E22: Scripts nested too deep"));
EXTERN uchar_kt e_noalt[] INIT(= N_("E23: No alternate file"));
EXTERN uchar_kt e_noabbr[] INIT(= N_("E24: No such abbreviation"));
EXTERN uchar_kt e_nobang[] INIT(= N_("E477: No ! allowed"));
EXTERN uchar_kt e_nogvim[] INIT(= N_("E25: Nvim does not have a built-in GUI"));
EXTERN uchar_kt e_nogroup[] INIT(= N_("E28: No such highlight group name: %s"));
EXTERN uchar_kt e_noinstext[] INIT(= N_("E29: No inserted text yet"));
EXTERN uchar_kt e_nolastcmd[] INIT(= N_("E30: No previous command line"));
EXTERN uchar_kt e_nomap[] INIT(= N_("E31: No such mapping"));
EXTERN uchar_kt e_nomatch[] INIT(= N_("E479: No match"));
EXTERN uchar_kt e_nomatch2[] INIT(= N_("E480: No match: %s"));
EXTERN uchar_kt e_noname[] INIT(= N_("E32: No file name"));
EXTERN uchar_kt e_nopresub[] INIT(= N_("E33: No previous substitute regular expression"));
EXTERN uchar_kt e_noprev[] INIT(= N_("E34: No previous command"));
EXTERN uchar_kt e_noprevre[] INIT(= N_("E35: No previous regular expression"));
EXTERN uchar_kt e_norange[] INIT(= N_("E481: No range allowed"));
EXTERN uchar_kt e_noroom[] INIT(= N_("E36: Not enough room"));
EXTERN uchar_kt e_notmp[] INIT(= N_("E483: Can't get temp file name"));
EXTERN uchar_kt e_notopen[] INIT(= N_("E484: Can't open file %s"));
EXTERN uchar_kt e_notread[] INIT(= N_("E485: Can't read file %s"));
EXTERN uchar_kt e_nowrtmsg[] INIT(= N_("E37: No write since last change (add ! to override)"));
EXTERN uchar_kt e_nowrtmsg_nobang[] INIT(= N_("E37: No write since last change"));
EXTERN uchar_kt e_null[] INIT(= N_("E38: Null argument"));
EXTERN uchar_kt e_number_exp[] INIT(= N_("E39: Number expected"));
EXTERN uchar_kt e_openerrf[] INIT(= N_("E40: Can't open errorfile %s"));
EXTERN uchar_kt e_outofmem[] INIT(= N_("E41: Out of memory!"));
EXTERN uchar_kt e_patnotf[] INIT(= N_("Pattern not found"));
EXTERN uchar_kt e_patnotf2[] INIT(= N_("E486: Pattern not found: %s"));
EXTERN uchar_kt e_positive[] INIT(= N_("E487: Argument must be positive"));
EXTERN uchar_kt e_prev_dir[] INIT(= N_("E459: Cannot go back to previous directory"));
EXTERN uchar_kt e_quickfix[] INIT(= N_("E42: No Errors"));
EXTERN uchar_kt e_loclist[] INIT(= N_("E776: No location list"));
EXTERN uchar_kt e_re_damg[] INIT(= N_("E43: Damaged match string"));
EXTERN uchar_kt e_re_corr[] INIT(= N_("E44: Corrupted regexp program"));
EXTERN uchar_kt e_readonly[] INIT(= N_("E45: 'readonly' option is set (add ! to override)"));
EXTERN uchar_kt e_readerrf[] INIT(= N_("E47: Error while reading errorfile"));
EXTERN uchar_kt e_sandbox[] INIT(= N_("E48: Not allowed in sandbox"));
EXTERN uchar_kt e_secure[] INIT(= N_("E523: Not allowed here"));
EXTERN uchar_kt e_screenmode[] INIT(= N_("E359: Screen mode setting not supported"));
EXTERN uchar_kt e_scroll[] INIT(= N_("E49: Invalid scroll size"));
EXTERN uchar_kt e_shellempty[] INIT(= N_("E91: 'shell' option is empty"));
EXTERN uchar_kt e_signdata[] INIT(= N_("E255: Couldn't read in sign data!"));
EXTERN uchar_kt e_swapclose[] INIT(= N_("E72: Close error on swap file"));
EXTERN uchar_kt e_tagstack[] INIT(= N_("E73: tag stack empty"));
EXTERN uchar_kt e_toocompl[] INIT(= N_("E74: Command too complex"));
EXTERN uchar_kt e_longname[] INIT(= N_("E75: Name too long"));
EXTERN uchar_kt e_toomsbra[] INIT(= N_("E76: Too many ["));
EXTERN uchar_kt e_toomany[] INIT(= N_("E77: Too many file names"));
EXTERN uchar_kt e_trailing[] INIT(= N_("E488: Trailing characters"));
EXTERN uchar_kt e_umark[] INIT(= N_("E78: Unknown mark"));
EXTERN uchar_kt e_wildexpand[] INIT(= N_("E79: Cannot expand wildcards"));
EXTERN uchar_kt e_winheight[] INIT(= N_("E591: 'winheight' cannot be smaller than 'winminheight'"));
EXTERN uchar_kt e_winwidth[] INIT(= N_("E592: 'winwidth' cannot be smaller than 'winminwidth'"));
EXTERN uchar_kt e_write[] INIT(= N_("E80: Error while writing"));
EXTERN uchar_kt e_zerocount[] INIT(= N_("Zero count"));
EXTERN uchar_kt e_usingsid[] INIT(= N_("E81: Using <SID> not in a script context"));
EXTERN uchar_kt e_intern2[] INIT(= N_("E685: Internal error: %s"));
EXTERN uchar_kt e_maxmempat[] INIT(= N_("E363: pattern uses more memory than 'maxmempattern'"));
EXTERN uchar_kt e_emptybuf[] INIT(= N_("E749: empty buffer"));
EXTERN uchar_kt e_nobufnr[] INIT(= N_("E86: Buffer %" PRId64 " does not exist"));
EXTERN uchar_kt e_invalpat[] INIT(= N_("E682: Invalid search pattern or delimiter"));
EXTERN uchar_kt e_bufloaded[] INIT(= N_("E139: File is loaded in another buffer"));
EXTERN uchar_kt e_notset[] INIT(= N_("E764: Option '%s' is not set"));
EXTERN uchar_kt e_invalidreg[] INIT(= N_("E850: Invalid register name"));
EXTERN uchar_kt e_dirnotf[] INIT(= N_("E919: Directory not found in '%s': \"%s\""));
EXTERN uchar_kt e_unsupportedoption[] INIT(= N_("E519: Option not supported"));
EXTERN uchar_kt e_fnametoolong[] INIT(= N_("E856: Filename too long"));
EXTERN uchar_kt e_float_as_string[] INIT(= N_("E806: using Float as a String"));

EXTERN char top_bot_msg[] INIT(= N_("search hit TOP, continuing at BOTTOM"));
EXTERN char bot_top_msg[] INIT(= N_("search hit BOTTOM, continuing at TOP"));

/// For undo we need to know the lowest time possible.
EXTERN time_t starttime;

///  where to write startup timing
EXTERN FILE *time_fd INIT(= NULL);

/// Some compilers warn for not using a return value,
/// but in some situations we can't do anything useful
/// with the value. Assign to this variable to avoid the warning.
EXTERN int ignored;
EXTERN char *ignoredp;

/// If a msgpack-rpc channel should be started over stdin/stdout
EXTERN bool embedded_mode INIT(= false);

/// next free id for a job or rpc channel
EXTERN uint64_t next_chan_id INIT(= 1);

/// Used to track the status of external functions.
/// Currently only used for iconv().
typedef enum
{
    kUnknown,
    kWorking,
    kBroken
} WorkingStatus;

/// The scope of a working-directory command like :cd.
///
/// Scopes are enumerated from lowest to highest. When adding a scope make sure
/// to update all functions using scopes as well, such as the implementation of
/// getcwd(). When using scopes as limits (e.g. in loops) don't use the scopes
/// directly, use MIN_CD_SCOPE and MAX_CD_SCOPE instead.
typedef enum
{
    kCdScopeInvalid = -1,
    kCdScopeWindow,  ///< Affects one window.
    kCdScopeTab,     ///< Affects one tab page.
    kCdScopeGlobal,  ///< Affects the entire Nvim instance.
} CdScope;

#define MIN_CD_SCOPE  kCdScopeWindow
#define MAX_CD_SCOPE  kCdScopeGlobal

#endif // NVIM_GLOBALS_H
