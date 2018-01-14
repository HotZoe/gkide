/// @file nvim/ui_bridge.c
///
/// UI wrapper that sends requests to the UI thread.
/// Used by the built-in TUI and libnvim-based UIs.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#include "nvim/log.h"
#include "nvim/main.h"
#include "nvim/vim.h"
#include "nvim/ui.h"
#include "nvim/memory.h"
#include "nvim/ui_bridge.h"
#include "nvim/ugrid.h"
#include "nvim/api/private/helpers.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "ui_bridge.c.generated.h"
#endif

#define UI_PTR(b) (((ui_bridge_st *)b)->ui)

#if NVIM_LOG_LEVEL_MIN <= DEBUG_LOG_LEVEL
static size_t uilog_seen = 0;
static argv_callback_ft uilog_event = NULL;

#define UI_CALL(ui, name, argc, ...)                                           \
    do                                                                         \
    {                                                                          \
        if(uilog_event == ui_bridge_##name##_event)                            \
        {                                                                      \
            uilog_seen++;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            if(uilog_seen > 0)                                                 \
            {                                                                  \
                DEBUG_LOG("UI bridge: ...%zu times", uilog_seen);              \
            }                                                                  \
            DEBUG_LOG("UI bridge: " STR(name));                                \
            uilog_seen = 0;                                                    \
            uilog_event = ui_bridge_##name##_event;                            \
        }                                                                      \
                                                                               \
        ((ui_bridge_st *)ui)->scheduler(event_create(ui_bridge_##name##_event, \
                                                     argc,                     \
                                                     __VA_ARGS__),             \
                                        UI_PTR(ui));                           \
    }while(0)
#else
/// Schedule a function call on the UI bridge thread.
#define UI_CALL(ui, name, argc, ...)                                       \
    ((ui_bridge_st *)ui)->scheduler(event_create(ui_bridge_##name##_event, \
                                                 argc,                     \
                                                 __VA_ARGS__),             \
                                    UI_PTR(ui))
#endif

#define INT2PTR(i)   ((void *)(intptr_t)i)
#define PTR2INT(p)   ((Integer)(intptr_t)p)

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "ui_events_bridge.generated.h"
#endif

ui_st *ui_bridge_attach(ui_st *ui,
                        ui_main_ft ui_main,
                        event_scheduler_ft scheduler)
{
    ui_bridge_st *rv = xcalloc(1, sizeof(ui_bridge_st));

    rv->ui = ui;
    rv->bridge.rgb = ui->rgb;
    rv->bridge.stop = ui_bridge_stop;
    rv->bridge.resize = ui_bridge_resize;
    rv->bridge.clear = ui_bridge_clear;
    rv->bridge.eol_clear = ui_bridge_eol_clear;
    rv->bridge.cursor_goto = ui_bridge_cursor_goto;
    rv->bridge.mode_info_set = ui_bridge_mode_info_set;
    rv->bridge.update_menu = ui_bridge_update_menu;
    rv->bridge.busy_start = ui_bridge_busy_start;
    rv->bridge.busy_stop = ui_bridge_busy_stop;
    rv->bridge.mouse_on = ui_bridge_mouse_on;
    rv->bridge.mouse_off = ui_bridge_mouse_off;
    rv->bridge.mode_change = ui_bridge_mode_change;
    rv->bridge.set_scroll_region = ui_bridge_set_scroll_region;
    rv->bridge.scroll = ui_bridge_scroll;
    rv->bridge.highlight_set = ui_bridge_highlight_set;
    rv->bridge.put = ui_bridge_put;
    rv->bridge.bell = ui_bridge_bell;
    rv->bridge.visual_bell = ui_bridge_visual_bell;
    rv->bridge.update_fg = ui_bridge_update_fg;
    rv->bridge.update_bg = ui_bridge_update_bg;
    rv->bridge.update_sp = ui_bridge_update_sp;
    rv->bridge.flush = ui_bridge_flush;
    rv->bridge.suspend = ui_bridge_suspend;
    rv->bridge.set_title = ui_bridge_set_title;
    rv->bridge.set_icon = ui_bridge_set_icon;
    rv->scheduler = scheduler;

    for(UIWidget i = 0; (int)i < UI_WIDGETS; i++)
    {
        rv->bridge.ui_ext[i] = ui->ui_ext[i];
    }

    rv->ui_main = ui_main;
    uv_mutex_init(&rv->mutex);
    uv_cond_init(&rv->cond);
    uv_mutex_lock(&rv->mutex);
    rv->ready = false;

    if(uv_thread_create(&rv->ui_thread, ui_thread_run, rv))
    {
        abort();
    }

    while(!rv->ready)
    {
        uv_cond_wait(&rv->cond, &rv->mutex);
    }

    uv_mutex_unlock(&rv->mutex);
    ui_attach_impl(&rv->bridge);

    return &rv->bridge;
}

void ui_bridge_stopped(ui_bridge_st *bridge)
{
    uv_mutex_lock(&bridge->mutex);
    bridge->stopped = true;
    uv_mutex_unlock(&bridge->mutex);
}

/// The UI thread entry point function
static void ui_thread_run(void *data)
{
    ui_bridge_st *bridge = data;
    bridge->ui_main(bridge, bridge->ui);
}

static void ui_bridge_stop(ui_st *b)
{
    ui_bridge_st *bridge = (ui_bridge_st *)b;
    bool stopped = bridge->stopped = false;
    UI_CALL(b, stop, 1, b);

    for(;;)
    {
        uv_mutex_lock(&bridge->mutex);
        stopped = bridge->stopped;
        uv_mutex_unlock(&bridge->mutex);

        if(stopped)
        {
            break;
        }

        loop_poll_events(&main_loop, 10);
    }

    uv_thread_join(&bridge->ui_thread);
    uv_mutex_destroy(&bridge->mutex);
    uv_cond_destroy(&bridge->cond);
    ui_detach_impl(b);
    xfree(b);
}
static void ui_bridge_stop_event(void **argv)
{
    ui_st *ui = UI_PTR(argv[0]);
    ui->stop(ui);
}

static void ui_bridge_highlight_set(ui_st *b, uihl_attr_st attrs)
{
    uihl_attr_st *a = xmalloc(sizeof(uihl_attr_st));
    *a = attrs;
    UI_CALL(b, highlight_set, 2, b, a);
}
static void ui_bridge_highlight_set_event(void **argv)
{
    ui_st *ui = UI_PTR(argv[0]);
    ui->highlight_set(ui, *((uihl_attr_st *)argv[1]));
    xfree(argv[1]);
}

static void ui_bridge_suspend(ui_st *b)
{
    ui_bridge_st *data = (ui_bridge_st *)b;
    uv_mutex_lock(&data->mutex);
    UI_CALL(b, suspend, 1, b);
    data->ready = false;

    // suspend the main thread until
    // CONTINUE is called by the UI thread
    while(!data->ready)
    {
        uv_cond_wait(&data->cond, &data->mutex);
    }

    uv_mutex_unlock(&data->mutex);
}
static void ui_bridge_suspend_event(void **argv)
{
    ui_st *ui = UI_PTR(argv[0]);
    ui->suspend(ui);
}
