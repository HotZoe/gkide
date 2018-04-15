/// @file nvim/nvim.h

#ifndef NVIM_NVIM_H
#define NVIM_NVIM_H

#include "generated/config/config.h"
#include "generated/config/confignvim.h"

#include "nvim/pos.h"
#include "nvim/types.h"

// Some defines from the old feature.h
#define SESSION_FILE      "Session.vim"
#define SYS_OPTWIN_FILE   "$VIMRUNTIME/optwin.vim"
#define RUNTIME_DIRNAME   "runtime"

#define MAX_MSG_HIST_LEN  200

// Check if configure correctly managed to find sizeof(int).
// If this failed, it becomes zero.
// This is likely a problem of not being able to run the test program.
// Other items from configure may also be wrong then!
#if (HOST_SIZEOF_INT == 0)
    #error "Configure did not run properly."
#endif

// bring lots of system header files
#include "nvim/os/os_defs.h"

/// length of a buffer to store a
/// number in ASCII (64 bits binary + NUL)
#define NUMBUFLEN     65

#define MAX_TYPENR    65535  ///<
#define ROOT_UID      0

#include "nvim/keymap.h"
#include "nvim/macros.h"
#include "nvim/gettext.h"

/// special attribute addition: Put message in history
#define MSG_HIST   0x1000

/// nvim working mode defination
///
/// most usual mode: insert, normal, cmdline, visual, etc.
///
/// The lower bits up to 0x20 are used to distinguish
/// normal/visual/op_pending and cmdline/insert+replace mode.
/// This is used for mapping. If none of these bits are set,
/// no mapping is done.
///
/// The upper bits are used to distinguish between other states.
///
/// @see ::curmod
typedef enum nvim_working_mode_e
{
    kNormalMode    = 0x01, ///< mode: Normal mode, command expected
    kVisualMode    = 0x02, ///< mode: Visual mode, use get_real_state()
    kOpPendMode    = 0x04, ///< mode: Normal mode, but operator is pending, to
                           ///<       get the current mode use get_real_state()
    kCmdLineMode   = 0x08, ///< mode: Command line mode
    kInsertMode    = 0x10, ///< mode: Insert mode

    kModFlgLangMap = 0x20, ///< flag: Language mapping flag can be combined
                           ///<       with @b kInsertMode and @b kCmdLineMode
    kModFlgReplace = 0x40, ///< flag: Replace mode flag
    kModFlgVReplace= 0x80, ///< flag: Virtual-replace mode flag

    /// Derive mode: Replace mode
    kReplaceMode    = kModFlgReplace + kInsertMode,
    /// Derive mode: Virtual-replace mode
    kVReplaceMode   = kModFlgReplace + kModFlgVReplace + kInsertMode,
    /// Derive mode: Line-replace mode
    kLReplaceMode   = kModFlgReplace + kModFlgLangMap,

    /// Normal mode, busy with a command
    kNormalBusyMode      = 0x100 + kNormalMode,
    /// waiting for return or command
    kNormalWaitMode      = 0x200 + kNormalMode,
    /// Asking if you want --more--
    kAskMoreMode         = 0x300,
    /// window size has changed
    kSetWinSizeMode      = 0x400,
    /* not used for now  = 0x500 */
    /// executing an external command
    kExecExtCmdMode      = 0x600,
    /// show matching paren
    kInsertShowMatchMode = 0x700 + kInsertMode,
    /// ":confirm" prompt
    kConfirmMode         = 0x800,
    /// Select mode, only for mappings
    kMapSelectMode       = 0x1000,
    /// Terminal focus mode
    kTermFocusMode       = 0x2000,
    /// live preview incomplete command mode
    kPreviewCmdMode      = 0x4000,

    /// all mode bits used for mapping
    kModFlgAllMapFlg = (0x3f | kMapSelectMode | kTermFocusMode),
} nvim_working_mode_et;

/// Directions
typedef enum
{
    BACKWARD_FILE = (-3),
    BACKWARD = (-1),
    kDirectionNotSet = 0,
    FORWARD = 1,
    FORWARD_FILE = 3,

} Direction;

#undef OK
#undef FAIL
#undef NOTDONE

/// return values for functions: false
#define FAIL       0
/// return values for functions: true
#define OK         1
/// return values for functions: not OK or FAIL but skipped
#define NOTDONE    2

/// values for xp_context when doing command line completion
enum
{
    EXPAND_UNSUCCESSFUL = -2,
    EXPAND_OK = -1,
    EXPAND_NOTHING = 0,
    EXPAND_COMMANDS,
    EXPAND_FILES,
    EXPAND_DIRECTORIES,
    EXPAND_SETTINGS,
    EXPAND_BOOL_SETTINGS,
    EXPAND_TAGS,
    EXPAND_OLD_SETTING,
    EXPAND_HELP,
    EXPAND_BUFFERS,
    EXPAND_EVENTS,
    EXPAND_MENUS,
    EXPAND_SYNTAX,
    EXPAND_HIGHLIGHT,
    EXPAND_AUGROUP,
    EXPAND_USER_VARS,
    EXPAND_MAPPINGS,
    EXPAND_TAGS_LISTFILES,
    EXPAND_FUNCTIONS,
    EXPAND_USER_FUNC,
    EXPAND_EXPRESSION,
    EXPAND_MENUNAMES,
    EXPAND_USER_COMMANDS,
    EXPAND_USER_CMD_FLAGS,
    EXPAND_USER_NARGS,
    EXPAND_USER_COMPLETE,
    EXPAND_ENV_VARS,
    EXPAND_LANGUAGE,
    EXPAND_COLORS,
    EXPAND_COMPILER,
    EXPAND_USER_DEFINED,
    EXPAND_USER_LIST,
    EXPAND_SHELLCMD,
    EXPAND_CSCOPE,
    EXPAND_SIGN,
    EXPAND_PROFILE,
    EXPAND_BEHAVE,
    EXPAND_FILETYPE,
    EXPAND_FILES_IN_PATH,
    EXPAND_OWNSYNTAX,
    EXPAND_LOCALES,
    EXPAND_HISTORY,
    EXPAND_USER,
    EXPAND_SYNTIME,
    EXPAND_USER_ADDR_TYPE,
    EXPAND_PACKADD,
};

// Minimal size for block 0 of a swap file.
// NOTE: This depends on size of blk_zero_st! It's not done with a sizeof(),
// because struct blk_zero_st is defined in memline.c (Sorry).
// The maximal block size is arbitrary.
#define MIN_SWAP_PAGE_SIZE    1048
#define MAX_SWAP_PAGE_SIZE    50000

// note: this is an int, not a long!
#if !defined(TRUE) && !defined(FALSE)
    #define TRUE   1
    #define FALSE  0
#endif

/// sometimes used for a variant on TRUE
#define MAYBE   2

#define STATUS_HEIGHT   1   ///< height of a status line under a window
#define QF_WINHEIGHT    10  ///< default height for quickfix window

// Buffer sizes
#ifndef CMDBUFFSIZE
    #define CMDBUFFSIZE   256    ///< size of the command processing buffer
#endif

#define LSIZE             512    ///< max. size of a line in the tags file
#define DIALOG_MSG_SIZE   1000   ///< buffer size for dialog_msg()

/// buffer size for get_foldtext()
#define FOLD_TEXT_LEN     51

/// Maximum length of key sequence to be mapped.
/// Must be able to hold an Amiga resize report.
#define MAXMAPLEN         50

/// Size in bytes of the hash used in the undo file.
#define UNDO_HASH_SIZE    32

#include "nvim/message.h"

/// Prefer using emsgf(), because perror() may send the output
/// to the wrong destination and mess up the screen.
#define PERROR(msg)    (void)emsgf("%s: %s", msg, strerror(errno))

#define SHOWCMD_COLS   10  ///< columns needed by shown command
#define STL_MAX_ITEM   80  ///< max nr of %<flag> in statusline

/// Compare file names
///
/// On some systems case in a file name does not matter, on others it does.
///
/// @note
/// Does not account for maximum name lengths and things like "../dir",
/// thus it is not 100% accurate. OS may also use different algorythm for
/// case-insensitive comparison.
///
/// @param[in]  x  First file name to compare.
/// @param[in]  y  Second file name to compare.
///
/// @return 0 for equal file names, non-zero otherwise.
#define fnamecmp(x, y) \
    path_fnamecmp((const char *)(x), (const char *)(y))

#define fnamencmp(x, y, n) \
    path_fnamencmp((const char *)(x), (const char *)(y), (size_t)(n))

// Enums need a typecast to be used as array index (for Ultrix).
#define hl_attr(n)    highlight_attr[(int)(n)]
#define term_str(n)   term_strings[(int)(n)]

/// Maximum number of bytes in a multi-byte character. It can be one 32-bit
/// character of up to 6 bytes, or one 16-bit character of up to three bytes
/// plus six following composing characters of three bytes each.
#define MB_MAXBYTES    21

#include "nvim/globals.h"        // global variables and messages
#include "nvim/buffer_defs.h"    // buffer and windows
#include "nvim/ex_cmds_defs.h"   // Ex command defines

#define SET_NO_HLSEARCH(flag)  \
    no_hlsearch = (flag);      \
    set_vim_var_nr(VV_HLSEARCH, !no_hlsearch && p_hls)

// Used for flags in do_in_path()
#define DIP_ALL       0x01  ///< all matches, not just the first one
#define DIP_DIR       0x02  ///< find directories instead of files
#define DIP_ERR       0x04  ///< give an error message when none found
#define DIP_START     0x08  ///< also use "start" directory in 'packpath'
#define DIP_OPT       0x10  ///< also use "opt" directory in 'packpath'
#define DIP_NORTP     0x20  ///< do not use 'runtimepath'
#define DIP_NOAFTER   0x40  ///< skip "after" directories
#define DIP_AFTER     0x80  ///< only use "after" directories

/// Lowest number used for window ID.
/// Cannot have this many windows per tab.
#define LOWEST_WIN_ID   1000

#endif // NVIM_NVIM_H
