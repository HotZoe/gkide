/// @file nvim/tui/tui.c
///
/// Terminal UI functions.
/// Invoked (by ui_bridge.c) on the TUI thread.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#include <uv.h>
#include <unibilium.h>

#if defined(HAVE_TERMIOS_H)
    #include <termios.h>
#endif

#include "nvim/lib/kvec.h"

#include "nvim/ascii.h"
#include "nvim/nvim.h"
#include "nvim/log.h"
#include "nvim/ui.h"
#include "nvim/map.h"
#include "nvim/main.h"
#include "nvim/memory.h"
#include "nvim/api/nvim.h"
#include "nvim/api/private/helpers.h"
#include "nvim/event/loop.h"
#include "nvim/event/signal.h"
#include "nvim/os/input.h"
#include "nvim/os/os.h"
#include "nvim/strings.h"
#include "nvim/ui_bridge.h"
#include "nvim/ugrid.h"
#include "nvim/tui/input.h"
#include "nvim/tui/tui.h"
#include "nvim/cursor_shape.h"
#include "nvim/syntax.h"
#include "nvim/macros.h"

// Space reserved in the output buffer to restore the cursor to normal when
// flushing. No existing terminal will require 32 bytes to do that.
#define CNORM_COMMAND_MAX_SIZE    32
#define OUTBUF_SIZE               0xffff

#define TOO_MANY_EVENTS           1000000
#define STARTS_WITH(str, prefix)  (!memcmp(str, prefix, sizeof(prefix) - 1))

#define TMUX_WRAP(seq) \
    (is_tmux ? "\x1bPtmux;\x1b" seq "\x1b\\" : seq)

typedef enum term_type_e
{
    kTermUnknown,
    kTermGnome,
    kTermiTerm,
    kTermKonsole,
    kTermRxvt,
    kTermDTTerm,
    kTermXTerm,
    kTermTeraTerm,
} term_type_et;

typedef struct rect_s
{
    int top;
    int bot;
    int left;
    int right;
} rect_st;

typedef struct tui_data_s
{
    ui_bridge_st *bridge;
    main_loop_st *loop;
    bool stop;
    unibi_var_t params[9];
    char buf[OUTBUF_SIZE];
    size_t bufpos, bufsize;
    terminal_input_st input;
    uv_loop_t write_loop;
    unibi_term *ut;

    union
    {
        uv_tty_t tty;
        uv_pipe_t pipe;
    } output_handle;

    bool out_isatty;
    signal_watcher_st winch_handle, cont_handle;
    bool cont_received;
    ugrid_st grid;
    kvec_t(rect_st) invalid_regions;
    int out_fd;
    bool scroll_region_is_full_screen;
    bool can_change_scroll_region;
    bool can_set_lr_margin;
    bool can_set_left_right_margin;
    bool mouse_enabled;
    bool busy;
    cursor_info_st cursor_shapes[kCsrShpIdxAllIndexCount];
    uihl_attr_st print_attrs;
    mode_shape_et showing_mode;
    term_type_et term;

    struct
    {
        int enable_mouse, disable_mouse;
        int enable_bracketed_paste, disable_bracketed_paste;
        int enable_lr_margin, disable_lr_margin;
        int set_rgb_foreground, set_rgb_background;
        int set_cursor_color;
        int enable_focus_reporting, disable_focus_reporting;
        int resize_screen;
        int reset_scroll_region;
    } unibi_ext;
} tuidata_st;

static bool volatile got_winch = false;
static bool cursor_style_enabled = false;
static bool is_tmux = false;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "tui/tui.c.generated.h"
#endif

ui_st *tui_start(void)
{
    ui_st *ui = xcalloc(1, sizeof(ui_st));

    ui->stop = tui_stop;
    ui->rgb = p_tgc;
    ui->resize = tui_resize;
    ui->clear = tui_clear;
    ui->eol_clear = tui_eol_clear;
    ui->cursor_goto = tui_cursor_goto;
    ui->mode_info_set = tui_mode_info_set;
    ui->update_menu = tui_update_menu;
    ui->busy_start = tui_busy_start;
    ui->busy_stop = tui_busy_stop;
    ui->mouse_on = tui_mouse_on;
    ui->mouse_off = tui_mouse_off;
    ui->mode_change = tui_mode_change;
    ui->set_scroll_region = tui_set_scroll_region;
    ui->scroll = tui_scroll;
    ui->highlight_set = tui_highlight_set;
    ui->put = tui_put;
    ui->bell = tui_bell;
    ui->visual_bell = tui_visual_bell;
    ui->update_fg = tui_update_fg;
    ui->update_bg = tui_update_bg;
    ui->update_sp = tui_update_sp;
    ui->flush = tui_flush;
    ui->suspend = tui_suspend;
    ui->set_title = tui_set_title;
    ui->set_icon = tui_set_icon;
    ui->event = tui_event;

    memset(ui->ui_ext, 0, sizeof(ui->ui_ext));

    return ui_bridge_attach(ui, tui_main, tui_scheduler);
}

static void terminfo_start(ui_st *ui)
{
    tuidata_st *data = ui->data;

    data->scroll_region_is_full_screen = true;
    data->bufpos = 0;
    data->bufsize = sizeof(data->buf) - CNORM_COMMAND_MAX_SIZE;
    data->showing_mode = kCsrShpIdxNormal;
    data->unibi_ext.enable_mouse = -1;
    data->unibi_ext.disable_mouse = -1;
    data->unibi_ext.set_cursor_color = -1;
    data->unibi_ext.enable_bracketed_paste = -1;
    data->unibi_ext.disable_bracketed_paste = -1;
    data->unibi_ext.enable_lr_margin = -1;
    data->unibi_ext.disable_lr_margin = -1;
    data->unibi_ext.enable_focus_reporting = -1;
    data->unibi_ext.disable_focus_reporting = -1;
    data->unibi_ext.resize_screen = -1;
    data->unibi_ext.reset_scroll_region = -1;
    data->out_fd = 1;
    data->out_isatty = os_isatty(data->out_fd);
    data->ut = unibi_from_env(); // setup unibilium

    if(!data->ut)
    {
        // For some reason could not read terminfo file, use a dummy entry
        // that will be populated with common values by fix_terminfo below
        data->ut = unibi_dummy();
    }

    fix_terminfo(data);

    data->can_change_scroll_region =
        !!unibi_get_str(data->ut, unibi_change_scroll_region);

    data->can_set_lr_margin =
        !!unibi_get_str(data->ut, unibi_set_lr_margin);

    data->can_set_left_right_margin =
        !!unibi_get_str(data->ut, unibi_set_left_margin_parm)
        && !!unibi_get_str(data->ut, unibi_set_right_margin_parm);

    // Set 't_Co' from the result of unibilium & fix_terminfo.
    t_colors = unibi_get_num(data->ut, unibi_max_colors);

    // Enter alternate screen and clear
    // NOTE: Do this *before* changing terminal settings.
    unibi_out(ui, unibi_enter_ca_mode);
    unibi_out(ui, unibi_clear_screen);

    // Enable bracketed paste
    unibi_out(ui, data->unibi_ext.enable_bracketed_paste);

    // Enable focus reporting
    unibi_out(ui, data->unibi_ext.enable_focus_reporting);
    uv_loop_init(&data->write_loop);

    if(data->out_isatty)
    {
        uv_tty_init(&data->write_loop,
                    &data->output_handle.tty,
                    data->out_fd, 0);

        uv_tty_set_mode(&data->output_handle.tty, UV_TTY_MODE_RAW);
    }
    else
    {
        uv_pipe_init(&data->write_loop, &data->output_handle.pipe, 0);
        uv_pipe_open(&data->output_handle.pipe, data->out_fd);
    }
}

static void terminfo_stop(ui_st *ui)
{
    tuidata_st *data = ui->data;

    // Destroy output stuff
    tui_mode_change(ui, (String)STRING_INIT, kCsrShpIdxNormal);
    tui_mouse_off(ui);
    unibi_out(ui, unibi_exit_attribute_mode);

    // cursor should be set to normal before exiting alternate screen
    unibi_out(ui, unibi_cursor_normal);
    unibi_out(ui, unibi_exit_ca_mode);

    // Disable bracketed paste
    unibi_out(ui, data->unibi_ext.disable_bracketed_paste);

    // Disable focus reporting
    unibi_out(ui, data->unibi_ext.disable_focus_reporting);
    flush_buf(ui, true);

    uv_tty_reset_mode();
    uv_close((uv_handle_t *)&data->output_handle, NULL);
    uv_run(&data->write_loop, UV_RUN_DEFAULT);

    if(uv_loop_close(&data->write_loop))
    {
        abort();
    }

    unibi_destroy(data->ut);
}

static void tui_terminal_start(ui_st *ui)
{
    tuidata_st *data = ui->data;

    data->print_attrs = EMPTY_ATTRS;
    ugrid_init(&data->grid);
    terminfo_start(ui);
    update_size(ui);
    signal_watcher_start(&data->winch_handle, sigwinch_cb, SIGWINCH);
    term_input_start(&data->input);
}

static void tui_terminal_stop(ui_st *ui)
{
    tuidata_st *data = ui->data;

    term_input_stop(&data->input);
    signal_watcher_stop(&data->winch_handle);
    terminfo_stop(ui);
    ugrid_free(&data->grid);
}

static void tui_stop(ui_st *ui)
{
    tui_terminal_stop(ui);
    tuidata_st *data = ui->data;
    data->stop = true;
}

/// Main function of the TUI thread
static void tui_main(ui_bridge_st *bridge, ui_st *ui)
{
    main_loop_st tui_loop;
    loop_init(&tui_loop, NULL);

    tuidata_st *data = xcalloc(1, sizeof(tuidata_st));

    ui->data = data;
    data->bridge = bridge;
    data->loop = &tui_loop;

    kv_init(data->invalid_regions);

    signal_watcher_init(data->loop, &data->winch_handle, ui);
    signal_watcher_init(data->loop, &data->cont_handle, data);

#ifdef UNIX
    signal_watcher_start(&data->cont_handle, sigcont_cb, SIGCONT);
#endif

#if TERMKEY_VERSION_MAJOR > 0 || TERMKEY_VERSION_MINOR > 18
    data->input.tk_ti_hook_fn = tui_tk_ti_getstr;
#endif

    term_input_init(&data->input, &tui_loop);
    tui_terminal_start(ui);
    data->stop = false;

    // allow the main thread to continue, we are
    // ready to start handling UI callbacks
    CONTINUE(bridge);

    while(!data->stop)
    {
        loop_poll_events(&tui_loop, -1);
    }

    ui_bridge_stopped(bridge);
    term_input_destroy(&data->input);

    signal_watcher_stop(&data->cont_handle);
    signal_watcher_close(&data->cont_handle, NULL);
    signal_watcher_close(&data->winch_handle, NULL);

    loop_close(&tui_loop, false);

    kv_destroy(data->invalid_regions);

    xfree(data);
    xfree(ui);
}

static void tui_scheduler(event_msg_st event, void *d)
{
    ui_st *ui = d;
    tuidata_st *data = ui->data;
    loop_schedule(data->loop, event);
}

#ifdef UNIX
static void sigcont_cb(signal_watcher_st *FUNC_ARGS_UNUSED_MATCH(watcher),
                       int FUNC_ARGS_UNUSED_MATCH(signum),
                       void *data)
{
    ((tuidata_st *)data)->cont_received = true;
}
#endif

static void sigwinch_cb(signal_watcher_st *FUNC_ARGS_UNUSED_MATCH(watcher),
                        int FUNC_ARGS_UNUSED_MATCH(signum),
                        void *data)
{
    got_winch = true;
    ui_st *ui = data;
    update_size(ui);
    ui_schedule_refresh();
}

static bool attrs_differ(uihl_attr_st a1, uihl_attr_st a2)
{
    return a1.foreground != a2.foreground
           || a1.background != a2.background
           || a1.bold != a2.bold
           || a1.italic != a2.italic
           || a1.undercurl != a2.undercurl
           || a1.underline != a2.underline
           || a1.reverse != a2.reverse;
}

static void update_attrs(ui_st *ui, uihl_attr_st attrs)
{
    tuidata_st *data = ui->data;

    if(!attrs_differ(attrs, data->print_attrs))
    {
        return;
    }

    data->print_attrs = attrs;
    unibi_out(ui, unibi_exit_attribute_mode);

    ugrid_st *grid = &data->grid;
    int fg = attrs.foreground != -1 ? attrs.foreground : grid->fg;
    int bg = attrs.background != -1 ? attrs.background : grid->bg;

    if(ui->rgb)
    {
        if(fg != -1)
        {
            data->params[0].i = (fg >> 16) & 0xff; // red
            data->params[1].i = (fg >> 8) & 0xff;  // green
            data->params[2].i = fg & 0xff;         // blue
            unibi_out(ui, data->unibi_ext.set_rgb_foreground);
        }

        if(bg != -1)
        {
            data->params[0].i = (bg >> 16) & 0xff; // red
            data->params[1].i = (bg >> 8) & 0xff;  // green
            data->params[2].i = bg & 0xff;         // blue
            unibi_out(ui, data->unibi_ext.set_rgb_background);
        }
    }
    else
    {
        if(fg != -1)
        {
            data->params[0].i = fg;
            unibi_out(ui, unibi_set_a_foreground);
        }

        if(bg != -1)
        {
            data->params[0].i = bg;
            unibi_out(ui, unibi_set_a_background);
        }
    }

    if(attrs.bold)
    {
        unibi_out(ui, unibi_enter_bold_mode);
    }

    if(attrs.italic)
    {
        unibi_out(ui, unibi_enter_italics_mode);
    }

    if(attrs.underline || attrs.undercurl)
    {
        unibi_out(ui, unibi_enter_underline_mode);
    }

    if(attrs.reverse)
    {
        unibi_out(ui, unibi_enter_reverse_mode);
    }
}

static void print_cell(ui_st *ui, ucell_st *ptr)
{
    update_attrs(ui, ptr->attrs);
    out(ui, ptr->data, strlen(ptr->data));
}

static void clear_region(ui_st *ui, int top, int bot, int left, int right)
{
    tuidata_st *data = ui->data;
    ugrid_st *grid = &data->grid;
    bool cleared = false;

    if(grid->bg == -1 && right == ui->width -1)
    {
        // Background is set to the default color and the right edge
        // matches the screen end, try to use terminal codes for
        // clearing the requested area.
        uihl_attr_st clear_attrs = EMPTY_ATTRS;

        clear_attrs.foreground = grid->fg;
        clear_attrs.background = grid->bg;
        update_attrs(ui, clear_attrs);

        if(left == 0)
        {
            if(bot == ui->height - 1)
            {
                if(top == 0)
                {
                    unibi_out(ui, unibi_clear_screen);
                }
                else
                {
                    unibi_goto(ui, top, 0);
                    unibi_out(ui, unibi_clr_eos);
                }

                cleared = true;
            }
        }

        if(!cleared)
        {
            // iterate through each line and clear with clr_eol
            for(int row = top; row <= bot; ++row)
            {
                unibi_goto(ui, row, left);
                unibi_out(ui, unibi_clr_eol);
            }

            cleared = true;
        }
    }

    if(!cleared)
    {
        // could not clear using faster terminal
        // codes, refresh the whole region
        int currow = -1;

        UGRID_FOREACH_CELL(grid, top, bot, left, right, {
            if(currow != row)
            {
                unibi_goto(ui, row, col);
                currow = row;
            }
            print_cell(ui, cell);
        });
    }

    // restore cursor
    unibi_goto(ui, grid->row, grid->col);
}

static bool can_use_scroll(ui_st *ui)
{
    tuidata_st *data = ui->data;
    ugrid_st *grid = &data->grid;

    return data->scroll_region_is_full_screen
           || (data->can_change_scroll_region
               && ((grid->left == 0 && grid->right == ui->width - 1)
                   || data->can_set_lr_margin
                   || data->can_set_left_right_margin));
}

static void set_scroll_region(ui_st *ui)
{
    tuidata_st *data = ui->data;
    ugrid_st *grid = &data->grid;

    data->params[0].i = grid->top;
    data->params[1].i = grid->bot;
    unibi_out(ui, unibi_change_scroll_region);

    if(grid->left != 0 || grid->right != ui->width - 1)
    {
        unibi_out(ui, data->unibi_ext.enable_lr_margin);

        if(data->can_set_lr_margin)
        {
            data->params[0].i = grid->left;
            data->params[1].i = grid->right;
            unibi_out(ui, unibi_set_lr_margin);
        }
        else
        {
            data->params[0].i = grid->left;
            unibi_out(ui, unibi_set_left_margin_parm);
            data->params[0].i = grid->right;
            unibi_out(ui, unibi_set_right_margin_parm);
        }
    }

    unibi_goto(ui, grid->row, grid->col);
}

static void reset_scroll_region(ui_st *ui)
{
    tuidata_st *data = ui->data;
    ugrid_st *grid = &data->grid;

    if(0 <= data->unibi_ext.reset_scroll_region)
    {
        unibi_out(ui, data->unibi_ext.reset_scroll_region);
    }
    else
    {
        data->params[0].i = 0;
        data->params[1].i = ui->height - 1;
        unibi_out(ui, unibi_change_scroll_region);
    }

    if(grid->left != 0 || grid->right != ui->width - 1)
    {
        if(data->can_set_lr_margin)
        {
            data->params[0].i = 0;
            data->params[1].i = ui->width - 1;
            unibi_out(ui, unibi_set_lr_margin);
        }
        else
        {
            data->params[0].i = 0;
            unibi_out(ui, unibi_set_left_margin_parm);
            data->params[0].i = ui->width - 1;
            unibi_out(ui, unibi_set_right_margin_parm);
        }

        unibi_out(ui, data->unibi_ext.disable_lr_margin);
    }

    unibi_goto(ui, grid->row, grid->col);
}

static void tui_resize(ui_st *ui, Integer width, Integer height)
{
    tuidata_st *data = ui->data;
    ugrid_resize(&data->grid, (int)width, (int)height);

    if(!got_winch) // Try to resize the terminal window.
    {
        data->params[0].i = (int)height;
        data->params[1].i = (int)width;
        unibi_out(ui, data->unibi_ext.resize_screen);

        // DECSLPP does not reset the scroll region.
        if(data->scroll_region_is_full_screen)
        {
            reset_scroll_region(ui);
        }
    }
    else
    {
        // Already handled the SIGWINCH
        // signal, avoid double-resize.
        got_winch = false;
    }
}

static void tui_clear(ui_st *ui)
{
    tuidata_st *data = ui->data;
    ugrid_st *grid = &data->grid;

    ugrid_clear(grid);
    clear_region(ui, grid->top, grid->bot, grid->left, grid->right);
}

static void tui_eol_clear(ui_st *ui)
{
    tuidata_st *data = ui->data;
    ugrid_st *grid = &data->grid;

    ugrid_eol_clear(grid);
    clear_region(ui, grid->row, grid->row, grid->col, grid->right);
}

static void tui_cursor_goto(ui_st *ui, Integer row, Integer col)
{
    tuidata_st *data = ui->data;

    ugrid_goto(&data->grid, (int)row, (int)col);
    unibi_goto(ui, (int)row, (int)col);
}

cursor_shape_et tui_cursor_decode_shape(const char *shape_str)
{
    cursor_shape_et shape = 0;

    if(xstrequal(shape_str, "block"))
    {
        shape = kCsrShpBlock;
    }
    else if(xstrequal(shape_str, "vertical"))
    {
        shape = kCsrShpVertical;
    }
    else if(xstrequal(shape_str, "horizontal"))
    {
        shape = kCsrShpHorizontal;
    }
    else
    {
        EMSG2(_(e_invarg2), shape_str);
    }

    return shape;
}

static cursor_info_st decode_cursor_entry(Dictionary args)
{
    cursor_info_st r;

    for(size_t i = 0; i < args.size; i++)
    {
        char *key = args.items[i].key.data;
        Object value = args.items[i].value;

        if(xstrequal(key, "cursor_shape"))
        {
            r.shape =
                tui_cursor_decode_shape(args.items[i].value.data.string.data);
        }
        else if(xstrequal(key, "blinkon"))
        {
            r.blinkon = (int)value.data.integer;
        }
        else if(xstrequal(key, "blinkoff"))
        {
            r.blinkoff = (int)value.data.integer;
        }
        else if(xstrequal(key, "hl_id"))
        {
            r.id = (int)value.data.integer;
        }
    }

    return r;
}

static void tui_mode_info_set(ui_st *ui, bool guicursor_enabled, Array args)
{
    cursor_style_enabled = guicursor_enabled;

    if(!guicursor_enabled)
    {
        return; // Do not send cursor style control codes.
    }

    tuidata_st *data = ui->data;
    assert(args.size);

    // cursor style entries as defined by `shape_table`.
    for(size_t i = 0; i < args.size; i++)
    {
        assert(args.items[i].type == kObjectTypeDictionary);
        cursor_info_st r = decode_cursor_entry(args.items[i].data.dictionary);
        data->cursor_shapes[i] = r;
    }

    tui_set_mode(ui, data->showing_mode);
}

static void tui_update_menu(ui_st *FUNC_ARGS_UNUSED_MATCH(ui))
{
    // Do nothing
    // menus are for GUI only
}

static void tui_busy_start(ui_st *ui)
{
    ((tuidata_st *)ui->data)->busy = true;
}

static void tui_busy_stop(ui_st *ui)
{
    ((tuidata_st *)ui->data)->busy = false;
}

static void tui_mouse_on(ui_st *ui)
{
    tuidata_st *data = ui->data;

    if(!data->mouse_enabled)
    {
        unibi_out(ui, data->unibi_ext.enable_mouse);
        data->mouse_enabled = true;
    }
}

static void tui_mouse_off(ui_st *ui)
{
    tuidata_st *data = ui->data;

    if(data->mouse_enabled)
    {
        unibi_out(ui, data->unibi_ext.disable_mouse);
        data->mouse_enabled = false;
    }
}

static void tui_set_mode(ui_st *ui, mode_shape_et mode)
{
    if(!cursor_style_enabled)
    {
        return;
    }

    tuidata_st *data = ui->data;
    cursor_info_st c = data->cursor_shapes[mode];
    int shape = c.shape;
    unibi_var_t vars[26 + 26] = { { 0 } };

    // Support changing cursor shape on some popular terminals.
    const char *vte_version = os_getenv("VTE_VERSION");

    if(c.id != 0 && ui->rgb)
    {
        int attr = syn_id2attr(c.id);

        if(attr > 0)
        {
            attrinfo_st *aep = syn_cterm_attr2entry(attr);
            data->params[0].i = aep->rgb_bg_color;
            unibi_out(ui, data->unibi_ext.set_cursor_color);
        }
    }

    if(data->term == kTermKonsole)
    {
        // Konsole uses a proprietary escape code to set
        // the cursor shape and does not support DECSCUSR.
        switch(shape)
        {
            case kCsrShpBlock:
                shape = 0;
                break;

            case kCsrShpVertical:
                shape = 1;
                break;

            case kCsrShpHorizontal:
                shape = 2;
                break;

            default:
                ALERT_LOG("Unknown shape value %d", shape);
                break;
        }

        data->params[0].i = shape;
        data->params[1].i = (c.blinkon != 0);

        unibi_format(vars,
                     vars + 26,
                     TMUX_WRAP("\x1b]50;"
                               "cursor_shape_et=%p1%d;"
                               "BlinkingCursorEnabled=%p2%d\x07"),
                     data->params,
                     out,
                     ui,
                     NULL,
                     NULL);
    }
    else if(!vte_version || atoi(vte_version) >= 3900)
    {
        // Assume that the terminal supports DECSCUSR unless it is an
        // old VTE based terminal. This should not get wrapped for tmux,
        // which will handle it via its Ss/Se terminfo extension - usually
        // according to its terminal-overrides.
        switch(shape)
        {
            case kCsrShpBlock:
                shape = 1;
                break;

            case kCsrShpHorizontal:
                shape = 3;
                break;

            case kCsrShpVertical:
                shape = 5;
                break;

            default:
                ALERT_LOG("Unknown shape value %d", shape);
                break;
        }

        data->params[0].i = shape + (int)(c.blinkon == 0);

        unibi_format(vars,
                     vars + 26,
                     "\x1b[%p1%d q",
                     data->params,
                     out,
                     ui,
                     NULL,
                     NULL);
    }
}

/// @param mode editor mode
static void tui_mode_change(ui_st *ui,
                            String FUNC_ARGS_UNUSED_MATCH(mode),
                            Integer mode_idx)
{
    tuidata_st *data = ui->data;

    tui_set_mode(ui, (mode_shape_et)mode_idx);
    data->showing_mode = (mode_shape_et)mode_idx;
}

static void tui_set_scroll_region(ui_st *ui,
                                  Integer top,
                                  Integer bot,
                                  Integer left,
                                  Integer right)
{
    tuidata_st *data = ui->data;

    ugrid_set_scroll_region(&data->grid,
                            (int)top,
                            (int)bot,
                            (int)left,
                            (int)right);

    data->scroll_region_is_full_screen = left == 0
                                         && right == ui->width - 1
                                         && top == 0
                                         && bot == ui->height - 1;
}

static void tui_scroll(ui_st *ui, Integer count)
{
    tuidata_st *data = ui->data;
    ugrid_st *grid = &data->grid;
    int clear_top, clear_bot;

    ugrid_scroll(grid, (int)count, &clear_top, &clear_bot);

    if(can_use_scroll(ui))
    {
        bool scroll_clears_to_current_colour =
            unibi_get_bool(data->ut, unibi_back_color_erase);

        // Change terminal scroll region and move cursor to the top
        if(!data->scroll_region_is_full_screen)
        {
            set_scroll_region(ui);
        }

        unibi_goto(ui, grid->top, grid->left);

        // also set default color attributes
        // or some terminals can become funny
        if(scroll_clears_to_current_colour)
        {
            uihl_attr_st clear_attrs = EMPTY_ATTRS;
            clear_attrs.foreground = grid->fg;
            clear_attrs.background = grid->bg;
            update_attrs(ui, clear_attrs);
        }

        if(count > 0)
        {
            if(count == 1)
            {
                unibi_out(ui, unibi_delete_line);
            }
            else
            {
                data->params[0].i = (int)count;
                unibi_out(ui, unibi_parm_delete_line);
            }
        }
        else
        {
            if(count == -1)
            {
                unibi_out(ui, unibi_insert_line);
            }
            else
            {
                data->params[0].i = -(int)count;
                unibi_out(ui, unibi_parm_insert_line);
            }
        }

        // Restore terminal scroll region and cursor
        if(!data->scroll_region_is_full_screen)
        {
            reset_scroll_region(ui);
        }

        unibi_goto(ui, grid->row, grid->col);

        if(!scroll_clears_to_current_colour)
        {
            // This is required because scrolling will leave
            // wrong background in the cleared area on non-bge terminals.
            clear_region(ui, clear_top, clear_bot, grid->left, grid->right);
        }
    }
    else
    {
        // Mark the entire scroll region as invalid for redrawing later
        invalidate(ui, grid->top, grid->bot, grid->left, grid->right);
    }
}

static void tui_highlight_set(ui_st *ui, uihl_attr_st attrs)
{
    ((tuidata_st *)ui->data)->grid.attrs = attrs;
}

static void tui_put(ui_st *ui, String text)
{
    tuidata_st *data = ui->data;
    print_cell(ui, ugrid_put(&data->grid, (uint8_t *)text.data, text.size));
}

static void tui_bell(ui_st *ui)
{
    unibi_out(ui, unibi_bell);
}

static void tui_visual_bell(ui_st *ui)
{
    unibi_out(ui, unibi_flash_screen);
}

static void tui_update_fg(ui_st *ui, Integer fg)
{
    ((tuidata_st *)ui->data)->grid.fg = (int)fg;
}

static void tui_update_bg(ui_st *ui, Integer bg)
{
    ((tuidata_st *)ui->data)->grid.bg = (int)bg;
}

static void tui_update_sp(ui_st *FUNC_ARGS_UNUSED_MATCH(ui),
                          Integer FUNC_ARGS_UNUSED_MATCH(sp))
{
    // Do nothing; 'special' color is for GUI only
}

static void tui_flush(ui_st *ui)
{
    tuidata_st *data = ui->data;
    ugrid_st *grid = &data->grid;
    size_t nrevents = loop_size(data->loop);

    if(nrevents > TOO_MANY_EVENTS)
    {
        // TUI event-queue flooded, purging
        STATE_LOG("TUI event-queue flooded (thread_events=%zu)", nrevents);

        // Back-pressure: UI events may accumulate much
        // faster than the terminal device can serve them.
        // Even if SIGINT/CTRL-C is received, user must still
        // wait for the TUI event-queue to drain, and if there
        // are ~millions of events in the queue, it could take hours.
        // Clearing the queue allows the UI to recover.
        loop_purge(data->loop);
        tui_busy_stop(ui); // avoid hidden cursor
    }

    while(kv_size(data->invalid_regions))
    {
        rect_st r = kv_pop(data->invalid_regions);
        int currow = -1;

        UGRID_FOREACH_CELL(grid, r.top, r.bot, r.left, r.right, {
            if(currow != row)
            {
                unibi_goto(ui, row, col);
                currow = row;
            }
            print_cell(ui, cell);
        });
    }

    unibi_goto(ui, grid->row, grid->col);
    flush_buf(ui, true);
}

#ifdef UNIX
static void suspend_event(void **argv)
{
    ui_st *ui = argv[0];
    tuidata_st *data = ui->data;
    bool enable_mouse = data->mouse_enabled;
    tui_terminal_stop(ui);
    data->cont_received = false;
    stream_set_blocking(input_global_fd(), true); // normalize stream (#2598)
    kill(0, SIGTSTP);

    while(!data->cont_received)
    {
        // poll the event loop until SIGCONT is received
        loop_poll_events(data->loop, -1);
    }

    tui_terminal_start(ui);

    if(enable_mouse)
    {
        tui_mouse_on(ui);
    }

    // libuv expects this
    stream_set_blocking(input_global_fd(), false);

    // resume the main thread
    CONTINUE(data->bridge);
}
#endif

static void tui_suspend(ui_st *ui)
{
#ifdef UNIX
    tuidata_st *data = ui->data;
    // kill(0, SIGTSTP) won't stop the UI thread, so we must poll for SIGCONT
    // before continuing. This is done in another callback to avoid
    // loop_poll_events recursion
    multiqueue_put_event(data->loop->fast_events,
                         event_create(suspend_event, 1, ui));
#endif
}

static void tui_set_title(ui_st *ui, String title)
{
    tuidata_st *data = ui->data;

    if(!(title.data && unibi_get_str(data->ut, unibi_to_status_line)
         && unibi_get_str(data->ut, unibi_from_status_line)))
    {
        return;
    }

    unibi_out(ui, unibi_to_status_line);
    out(ui, title.data, title.size);
    unibi_out(ui, unibi_from_status_line);
}

static void tui_set_icon(ui_st *FUNC_ARGS_UNUSED_MATCH(ui),
                         String FUNC_ARGS_UNUSED_MATCH(icon))
{
}

/// NB: if we start to use this, the ui_bridge must
/// be updated to make a copy for the tui thread
static void tui_event(ui_st *FUNC_ARGS_UNUSED_MATCH(ui),
                      char *FUNC_ARGS_UNUSED_MATCH(name),
                      Array FUNC_ARGS_UNUSED_MATCH(args),
                      bool *FUNC_ARGS_UNUSED_MATCH(args_consumed))
{
}

static void invalidate(ui_st *ui, int top, int bot, int left, int right)
{
    tuidata_st *data = ui->data;
    rect_st *intersects = NULL;

    // Increase dimensions before comparing to ensure
    // adjacent regions are treated as intersecting
    --top;
    ++bot;
    --left;
    ++right;

    for(size_t i = 0; i < kv_size(data->invalid_regions); i++)
    {
        rect_st *r = &kv_A(data->invalid_regions, i);

        if(!(top > r->bot
             || bot < r->top
             || left > r->right
             || right < r->left))
        {
            intersects = r;
            break;
        }
    }

    ++top;
    --bot;
    ++left;
    --right;

    if(intersects)
    {
        // If top/bot/left/right intersects with a
        // invalid rect, we replace it by the union
        intersects->top = MIN(top, intersects->top);
        intersects->bot = MAX(bot, intersects->bot);
        intersects->left = MIN(left, intersects->left);
        intersects->right = MAX(right, intersects->right);
    }
    else
    {
        // Else just add a new entry;
        kv_push(data->invalid_regions, ((rect_st) {
            top, bot, left, right
        }));
    }
}

static void update_size(ui_st *ui)
{
    tuidata_st *data = ui->data;
    int width = 0, height = 0;

    // 1 - look for non-default 'columns' and 'lines' options during startup
    if(starting != 0 && (Columns != DFLT_COLS || Rows != DFLT_ROWS))
    {
        assert(Columns >= INT_MIN && Columns <= INT_MAX);
        assert(Rows >= INT_MIN && Rows <= INT_MAX);

        width = (int)Columns;
        height = (int)Rows;

        goto end;
    }

    // 2 - try from a system call(ioctl/TIOCGWINSZ on unix)
    if(data->out_isatty
       && !uv_tty_get_winsize(&data->output_handle.tty, &width, &height))
    {
        goto end;
    }

    // 3 - use $LINES/$COLUMNS if available
    const char *val;
    int advance;

    if((val = os_getenv("LINES"))
       && sscanf(val, "%d%n", &height, &advance) != EOF && advance
       && (val = os_getenv("COLUMNS"))
       && sscanf(val, "%d%n", &width, &advance) != EOF && advance)
    {
        goto end;
    }

    // 4 - read from terminfo if available
    height = unibi_get_num(data->ut, unibi_lines);
    width = unibi_get_num(data->ut, unibi_columns);

end:

    if(width <= 0 || height <= 0)
    {
        // use the defaults
        width = DFLT_COLS;
        height = DFLT_ROWS;
    }

    data->bridge->bridge.width = ui->width = width;
    data->bridge->bridge.height = ui->height = height;
}

static void unibi_goto(ui_st *ui, int row, int col)
{
    tuidata_st *data = ui->data;

    data->params[0].i = row;
    data->params[1].i = col;
    unibi_out(ui, unibi_cursor_address);
}

static void unibi_out(ui_st *ui, int unibi_index)
{
    tuidata_st *data = ui->data;
    const char *str = NULL;

    if(unibi_index >= 0)
    {
        if(unibi_index < unibi_string_begin_)
        {
            str = unibi_get_ext_str(data->ut, (unsigned)unibi_index);
        }
        else
        {
            str = unibi_get_str(data->ut, (unsigned)unibi_index);
        }
    }

    if(str)
    {
        unibi_var_t vars[26 + 26] = {{0}};
        unibi_format(vars, vars + 26, str, data->params, out, ui, NULL, NULL);
    }
}

static void out(void *ctx, const char *str, size_t len)
{
    ui_st *ui = ctx;
    tuidata_st *data = ui->data;
    size_t available = data->bufsize - data->bufpos;

    if(len > available)
    {
        flush_buf(ui, false);
    }

    memcpy(data->buf + data->bufpos, str, len);
    data->bufpos += len;
}

static void unibi_set_if_empty(unibi_term *ut,
                               enum unibi_string str,
                               const char *val)
{
    if(!unibi_get_str(ut, str))
    {
        unibi_set_str(ut, str, val);
    }
}

static term_type_et detect_term(const char *term, const char *colorterm)
{
    if(STARTS_WITH(term, "rxvt"))
    {
        return kTermRxvt;
    }

    if(os_getenv("KONSOLE_PROFILE_NAME") || os_getenv("KONSOLE_DBUS_SESSION"))
    {
        return kTermKonsole;
    }

    const char *termprg = os_getenv("TERM_PROGRAM");

    if(termprg && strstr(termprg, "iTerm.app"))
    {
        return kTermiTerm;
    }

    if(colorterm && strstr(colorterm, "gnome-terminal"))
    {
        return kTermGnome;
    }

    if(STARTS_WITH(term, "xterm"))
    {
        return kTermXTerm;
    }

    if(STARTS_WITH(term, "dtterm"))
    {
        return kTermDTTerm;
    }

    if(STARTS_WITH(term, "teraterm"))
    {
        return kTermTeraTerm;
    }

    return kTermUnknown;
}

static void fix_terminfo(tuidata_st *data)
{
    unibi_term *ut = data->ut;
    is_tmux = os_getenv("TMUX") != NULL;

    const char *term = os_getenv("TERM");
    const char *colorterm = os_getenv("COLORTERM");

    if(!term)
    {
        goto end;
    }

    data->term = detect_term(term, colorterm);

    if(data->term == kTermRxvt)
    {
        unibi_set_if_empty(ut, unibi_exit_attribute_mode, "\x1b[m\x1b(B");
        unibi_set_if_empty(ut, unibi_flash_screen, "\x1b[?5h$<20/>\x1b[?5l");
        unibi_set_if_empty(ut, unibi_enter_italics_mode, "\x1b[3m");
        unibi_set_if_empty(ut, unibi_to_status_line, "\x1b]2");
    }
    else if(data->term == kTermXTerm)
    {
        unibi_set_if_empty(ut, unibi_to_status_line, "\x1b]0;");
    }
    else if(STARTS_WITH(term, "screen") || STARTS_WITH(term, "tmux"))
    {
        unibi_set_if_empty(ut, unibi_to_status_line, "\x1b_");
        unibi_set_if_empty(ut, unibi_from_status_line, "\x1b\\");
    }

    if(data->term == kTermXTerm || data->term == kTermRxvt)
    {
        const char *normal = unibi_get_str(ut, unibi_cursor_normal);

        if(!normal)
        {
            unibi_set_str(ut, unibi_cursor_normal, "\x1b[?25h");
        }
        else if(STARTS_WITH(normal, "\x1b[?12l"))
        {
            // terminfo typically includes DECRST 12 as part of setting
            // up the normal cursor, which interferes with the user's
            // control via NVIM_TUI_ENABLE_CURSOR_SHAPE.  When DECRST 12
            // is present, skip over it, but honor the rest of the TI setting.
            unibi_set_str(ut, unibi_cursor_normal, normal + strlen("\x1b[?12l"));
        }

        unibi_set_if_empty(ut, unibi_cursor_invisible, "\x1b[?25l");
        unibi_set_if_empty(ut, unibi_flash_screen, "\x1b[?5h$<100/>\x1b[?5l");
        unibi_set_if_empty(ut, unibi_exit_attribute_mode, "\x1b(B\x1b[m");
        unibi_set_if_empty(ut, unibi_set_tb_margin, "\x1b[%i%p1%d;%p2%dr");
        unibi_set_if_empty(ut, unibi_set_lr_margin, "\x1b[%i%p1%d;%p2%ds");
        unibi_set_if_empty(ut, unibi_set_left_margin_parm, "\x1b[%i%p1%ds");
        unibi_set_if_empty(ut, unibi_set_right_margin_parm, "\x1b[%i;%p2%ds");
        unibi_set_if_empty(ut, unibi_change_scroll_region, "\x1b[%i%p1%d;%p2%dr");
        unibi_set_if_empty(ut, unibi_clear_screen, "\x1b[H\x1b[2J");
        unibi_set_if_empty(ut, unibi_from_status_line, "\x07");
        unibi_set_bool(ut, unibi_back_color_erase, true);
    }

    data->unibi_ext.enable_lr_margin =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[?69h");

    data->unibi_ext.disable_lr_margin =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[?69l");

    data->unibi_ext.enable_bracketed_paste =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[?2004h");

    data->unibi_ext.disable_bracketed_paste =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[?2004l");

    data->unibi_ext.enable_focus_reporting =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[?1004h");

    data->unibi_ext.disable_focus_reporting =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[?1004l");

#define XTERM_SETAF \
    "\x1b[%?%p1%{8}%<%t3%p1%d%e%p1%{16}%<%t9%p1%{8}%-%d%e38;5;%p1%d%;m"

#define XTERM_SETAB \
    "\x1b[%?%p1%{8}%<%t4%p1%d%e%p1%{16}%<%t10%p1%{8}%-%d%e48;5;%p1%d%;m"

    if((colorterm && strstr(colorterm, "256"))
       || STARTS_WITH(term, "linux")
       || strstr(term, "256")
       || strstr(term, "xterm"))
    {
        // Linux 4.8+ supports 256-color SGR, but terminfo has
        // 8-color setaf/setab. Assume TERM=~xterm|linux or
        // COLORTERM=~256 supports 256 colors.
        unibi_set_num(ut, unibi_max_colors, 256);
        unibi_set_str(ut, unibi_set_a_foreground, XTERM_SETAF);
        unibi_set_str(ut, unibi_set_a_background, XTERM_SETAB);
    }

    // Only define this capability for
    // terminal types that we know understand it.
    if(data->term == kTermDTTerm      // originated this extension
       || data->term == kTermXTerm    // per xterm ctlseqs doc
       || data->term == kTermKonsole  // per commentary in VT102Emulation.cpp
       || data->term == kTermTeraTerm // per "Supported Control Functions" doc
       || data->term == kTermRxvt)    // per command.C
    {
        data->unibi_ext.resize_screen =
            (int)unibi_add_ext_str(ut, NULL, "\x1b[8;%p1%d;%p2%dt");
    }

    if(data->term == kTermXTerm || data->term == kTermRxvt)
    {
        data->unibi_ext.reset_scroll_region =
            (int)unibi_add_ext_str(ut, NULL, "\x1b[r");
    }

end:

    // Fill some empty slots with common terminal strings
    if(data->term == kTermiTerm)
    {
        data->unibi_ext.set_cursor_color =
            (int)unibi_add_ext_str(ut, NULL, TMUX_WRAP("\033]Pl%p1%06x\033\\"));
    }
    else
    {
        data->unibi_ext.set_cursor_color =
            (int)unibi_add_ext_str(ut, NULL, "\033]12;#%p1%06x\007");
    }

    data->unibi_ext.enable_mouse =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[?1002h\x1b[?1006h");

    data->unibi_ext.disable_mouse =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[?1002l\x1b[?1006l");

    data->unibi_ext.set_rgb_foreground =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[38;2;%p1%d;%p2%d;%p3%dm");

    data->unibi_ext.set_rgb_background =
        (int)unibi_add_ext_str(ut, NULL, "\x1b[48;2;%p1%d;%p2%d;%p3%dm");

    unibi_set_if_empty(ut, unibi_cursor_address, "\x1b[%i%p1%d;%p2%dH");
    unibi_set_if_empty(ut, unibi_exit_attribute_mode, "\x1b[0;10m");
    unibi_set_if_empty(ut, unibi_set_a_foreground, XTERM_SETAF);
    unibi_set_if_empty(ut, unibi_set_a_background, XTERM_SETAB);
    unibi_set_if_empty(ut, unibi_enter_bold_mode, "\x1b[1m");
    unibi_set_if_empty(ut, unibi_enter_underline_mode, "\x1b[4m");
    unibi_set_if_empty(ut, unibi_enter_reverse_mode, "\x1b[7m");
    unibi_set_if_empty(ut, unibi_bell, "\x07");

    unibi_set_if_empty(data->ut, unibi_enter_ca_mode, "\x1b[?1049h");
    unibi_set_if_empty(data->ut, unibi_exit_ca_mode, "\x1b[?1049l");

    unibi_set_if_empty(ut, unibi_delete_line, "\x1b[M");
    unibi_set_if_empty(ut, unibi_parm_delete_line, "\x1b[%p1%dM");
    unibi_set_if_empty(ut, unibi_insert_line, "\x1b[L");
    unibi_set_if_empty(ut, unibi_parm_insert_line, "\x1b[%p1%dL");
    unibi_set_if_empty(ut, unibi_clear_screen, "\x1b[H\x1b[J");
    unibi_set_if_empty(ut, unibi_clr_eol, "\x1b[K");
    unibi_set_if_empty(ut, unibi_clr_eos, "\x1b[J");
}

static void flush_buf(ui_st *ui, bool toggle_cursor)
{
    uv_write_t req;
    uv_buf_t buf;
    tuidata_st *data = ui->data;

    if(toggle_cursor && !data->busy)
    {
        // not busy and the cursor is invisible(see below).
        // Append a "cursor normal" command to the end of the buffer.
        data->bufsize += CNORM_COMMAND_MAX_SIZE;
        unibi_out(ui, unibi_cursor_normal);
        data->bufsize -= CNORM_COMMAND_MAX_SIZE;
    }

    buf.base = data->buf;
    buf.len = data->bufpos;

    uv_write(&req,
             STRUCT_CAST(uv_stream_t, &data->output_handle),
             &buf,
             1,
             NULL);

    uv_run(&data->write_loop, UV_RUN_DEFAULT);
    data->bufpos = 0;

    if(toggle_cursor && !data->busy)
    {
        // not busy and cursor is visible(see above),
        // append a "cursor invisible" command to the
        // beginning of the buffer for the next flush
        unibi_out(ui, unibi_cursor_invisible);
    }
}

#if TERMKEY_VERSION_MAJOR > 0 || TERMKEY_VERSION_MINOR > 18
/// Try to get "kbs" code from stty because "the terminfo kbs
/// entry is extremely unreliable." (Vim, Bash, and tmux also do this.)
///
/// @see tmux/tty-keys.c fe4e9470bb504357d073320f5d305b22663ee3fd
/// @see https://bugzilla.redhat.com/show_bug.cgi?id=142659
static const char *tui_get_stty_erase(void)
{
    static char stty_erase[2] = { 0 };

#if defined(ECHOE) && defined(ICANON) && defined(HAVE_TERMIOS_H)
    struct termios t;

    if(tcgetattr(input_global_fd(), &t) != -1)
    {
        stty_erase[0] = (char)t.c_cc[VERASE];
        stty_erase[1] = '\0';

        STATE_LOG("stty/termios:erase=%s", stty_erase);
    }
#endif

    return stty_erase;
}

/// libtermkey hook to override terminfo entries.
/// @see terminal_input_st.tk_ti_hook_fn
static const char *tui_tk_ti_getstr(const char *name,
                                    const char *value,
                                    void *FUNC_ARGS_UNUSED_MATCH(data))
{
    static const char *stty_erase = NULL;

    if(stty_erase == NULL)
    {
        stty_erase = tui_get_stty_erase();
    }

    if(xstrequal(name, "key_backspace"))
    {
        STATE_LOG("libtermkey:kbs=%s", value);

        if(stty_erase[0] != 0)
        {
            return stty_erase;
        }
    }
    else if(xstrequal(name, "key_dc"))
    {
        STATE_LOG("libtermkey:kdch1=%s", value);

        // Vim: "If <BS> and <DEL> are now the same, redefine <DEL>."
        if(value != NULL && xstrequal(stty_erase, value))
        {
            return stty_erase[0] == DEL ? CTRL_H_STR : DEL_STR;
        }
    }

    return value;
}
#endif
