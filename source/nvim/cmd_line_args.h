/// @file nvim/cmd_line_args.h

#ifndef NVIM_CMD_LINE_ARGS_H
#define NVIM_CMD_LINE_ARGS_H

#include "nvim/types.h"

/// Maximum number of commands from +, -c or --cmd arguments
#define MAX_CMDS_NUM    10

/// startup edit type
typedef enum edit_type_e
{
    kEditTypeNone  = 0, ///< no edit type yet
    kEditTypeFile  = 1, ///< file name argument[s] given, use argument list
    kEditTypeStdin = 2, ///< read file from stdin
    kEditTypeTag   = 3, ///< tag name argument given, use tagname
    kEditTypeQkfx  = 4, ///< start in quickfix mode
} edit_type_et;

/// windows layout, used by @b main_args_st::window_layout
typedef enum win_layout_e
{
    kWinLayoutDefault    = 0,
    kWinLayoutHorizontal = 1, ///< @b -o horizontally split windows
    kWinLayoutVertical   = 2, ///< @b -O vertically split windows
    kWinLayoutTabpage    = 3, ///< @b -p windows on tab pages
} win_layout_et;

/// Struct for various parameters passed between main() and other functions.
typedef struct main_args_s
{
    int argc;
    char **argv;

    char *use_nvimrc;                   ///< nvimrc from -u argument

    int cmd_num;                        ///< number of commands from + or -c
    char *cmd_args[MAX_CMDS_NUM];       ///< commands from + or -c arg
    int pre_cmd_num;                    ///< number of commands from --cmd
    char *pre_cmd_args[MAX_CMDS_NUM];   ///< commands from --cmd argument
    uchar_kt cmds_tofree[MAX_CMDS_NUM]; ///< commands that need free()

    int edit_type;          ///< type of editing to do, @b edit_type_et
    uchar_kt *tagname;      ///< tag from -t argument
    uchar_kt *err_file;     ///< 'errorfile' from -q argument
    uchar_kt *server_addr;  ///< NULL: not start, NUL: default, others: use it

    int want_full_screen;   ///< full screen on startup
    bool input_isatty;      ///< stdin is a terminal
    bool output_isatty;     ///< stdout is a terminal
    bool err_isatty;        ///< stderr is a terminal
    bool headless;          ///< Do not try to start an user interface
                            ///< or read/write to stdio(unless embedding)
    int no_swap_file;       ///< @b -n argument used
    int debug_break_level;
    int window_count;       ///< number of windows to use
    int window_layout;      ///< @b win_layout_et

#if !defined(UNIX)
    int literal;            ///< don't expand file names
#endif

    int diff_mode;          ///< start with @b diff set
} main_args_st;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "cmd_line_args.h.generated.h"
#endif

#endif // NVIM_CMD_LINE_ARGS_H
