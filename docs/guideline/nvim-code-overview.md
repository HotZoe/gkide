Here are some guideline information for getting around the source code. This
can get you started, but do not make it less complex than it is.

## Jumping around between files

Most code can be found in a file with an obvious name (incomplete list):

* [buffer.c](../../source/nvim/buffer.c)
  : manipulating buffers (loaded files)
* [diff.c](../../source/nvim/diff.c)
  : diff mode (vimdiff)
* [eval.c](../../source/nvim/eval.c)
  : expression evaluation
* [fileio.c](../../source/nvim/fileio.c)
  : reading and writing files
* [fold.c](../../source/nvim/fold.c)
  : folding
* [getchar.c](../../source/nvim/getchar.c)
  : getting characters and key mapping
* [mark.c](../../source/nvim/mark.c)
  : marks
* [mbyte.c](../../source/nvim/mbyte.c)
  : multi-byte character handling
* [memfile.c](../../source/nvim/memfile.c)
  : storing lines for buffers in a swapfile
* [memline.c](../../source/nvim/memline.c)
  : storing lines for buffers in memory
* [menu.c](../../source/nvim/menu.c)
  : menus
* [message.c](../../source/nvim/message.c)
  : about messages
* [ops.c](../../source/nvim/ops.c)
  : handling operators (`d`, `y`, `p`)
* [option.c](../../source/nvim/option.c)
  : options
* [quickfix.c](../../source/nvim/quickfix.c)
  : quickfix commands (`:make`, `:cn`)
* [regexp.c](../../source/nvim/regexp.c)
  : pattern matching
* [screen.c](../../source/nvim/screen.c)
  : updating the windows
* [search.c](../../source/nvim/search.c)
  : pattern searching
* [spell.c](../../source/nvim/spell.c)
  : spell checking
* [syntax.c](../../source/nvim/syntax.c)
  : syntax and other highlighting
* [tag.c](../../source/nvim/tag.c)
  : tags
* [terminal.c](../../source/nvim/terminal.c)
  : integrated terminal emulator
* [undo.c](../../source/nvim/undo.c)
  : undo and redo
* [window.c](../../source/nvim/window.c)
  : handling split windows

## Important variables

The current mode is stored in `curmod`. The values it can have are `NORMAL`,
`INSERT`, `CMDLINE`, and a few others.

The current window is `curwin`. The current buffer is `curbuf`. These point
to structures with the cursor position in the window, option values, the file
name, etc.

All the global variables are declared in
[globals.h](../../source/nvim/globals.h).


## The main loop

This is conveniently called `main_loop()`. It updates a few things and then
calls `normal_cmd()` to process a command. This returns when the command is
finished.

The basic idea is that Vim waits for the user to type a character and processes
it until another character is needed. Thus there are several places where Vim
waits for a character to be typed. The `vgetc()` function is used for this.
It also handles mapping.

Updating the screen is mostly postponed until a command or a sequence of
commands has finished. The work is done by `update_screen()`, which calls
`win_update()` for every window, which calls `win_line()` for every line.
See the start of [screen.c](../../source/nvim/screen.c) for more
explanations.

## Command-line mode

When typing a `:`, `normal_cmd()` will call `getcmdline()` to obtain a line
with an Ex command. `getcmdline()` contains a loop that will handle each
typed character. It returns when hitting `<CR>` or `<Esc>` or some other
character that ends the command line mode.

## Ex commands

Ex commands are handled by the function `do_cmdline()`. It does the generic
parsing of the `:` command line and calls `do_one_cmd()` for each separate
command. It also takes care of while loops.

`do_one_cmd()` parses the range and generic arguments and puts them in the
exarg_t and passes it to the function that handles the command.

The `:` commands are listed in [ex_cmds_defs.h](../../source/nvim/ex_cmds_defs.h).
The third entry of each item is the name of the function that handles the
command. The last entry are the flags that are used for the command.

## Normal mode commands

The Normal mode commands are handled by the `normal_cmd()` function. It also
handles the optional count and an extra character for some commands. These
are passed in a `cmdarg_t` to the function that handles the command.

There is a table `nv_cmds` in [normal.c](../../source/nvim/normal.c) which
lists the first character of every command. The second entry of each item
is the name of the function that handles the command.

## Insert mode commands

When doing an `i` or `a` command, `normal_cmd()` will call the `edit()`
function. It contains a loop that waits for the next character and handles
it. It returns when leaving Insert mode.

## Options

There is a list with all option names in
[option.c](../../source/nvim/option.c), called `options[]`.
