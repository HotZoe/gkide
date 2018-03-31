#include "nvim/vim.h"
#include "nvim/os/signal.h"

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
