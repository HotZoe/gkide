/// @file nvim/api/ui.c

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "nvim/nvim.h"
#include "nvim/ui.h"
#include "nvim/memory.h"
#include "nvim/map.h"
#include "nvim/msgpack/channel.h"
#include "nvim/api/ui.h"
#include "nvim/api/private/defs.h"
#include "nvim/api/private/helpers.h"
#include "nvim/popupmnu.h"
#include "nvim/cursor_shape.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "api/ui.c.generated.h"
    #include "ui_events_remote.generated.h"
#endif

typedef struct ui_data_s
{
    uint64_t channel_id;
    Array buffer;
} ui_data_st;

static PMap(uint64_t) *connected_uis = NULL;

void remote_ui_init(void)
FUNC_API_NOEXPORT
{
    connected_uis = pmap_new(uint64_t)();
}

void remote_ui_disconnect(uint64_t channel_id)
FUNC_API_NOEXPORT
{
    ui_st *ui = pmap_get(uint64_t)(connected_uis, channel_id);

    if(!ui)
    {
        return;
    }

    ui_data_st *data = ui->data;
    // destroy pending screen updates
    api_free_array(data->buffer);
    pmap_del(uint64_t)(connected_uis, channel_id);
    xfree(ui->data);
    ui_detach_impl(ui);
    xfree(ui);
}

void nvim_ui_attach(uint64_t channel_id,
                    Integer width,
                    Integer height,
                    Dictionary options,
                    error_st *err)
FUNC_API_SINCE(1)
FUNC_API_REMOTE_ONLY
{
    if(pmap_has(uint64_t)(connected_uis, channel_id))
    {
        api_set_error(err, kErrorTypeException,
                      "UI already attached for channel");
        return;
    }

    if(width <= 0 || height <= 0)
    {
        api_set_error(err, kErrorTypeValidation,
                      "Expected width > 0 and height > 0");
        return;
    }

    ui_st *ui = xcalloc(1, sizeof(ui_st));
    ui->width = (int)width;
    ui->height = (int)height;
    ui->rgb = true;
    ui->resize = remote_ui_resize;
    ui->clear = remote_ui_clear;
    ui->eol_clear = remote_ui_eol_clear;
    ui->cursor_goto = remote_ui_cursor_goto;
    ui->mode_info_set = remote_ui_mode_info_set;
    ui->update_menu = remote_ui_update_menu;
    ui->busy_start = remote_ui_busy_start;
    ui->busy_stop = remote_ui_busy_stop;
    ui->mouse_on = remote_ui_mouse_on;
    ui->mouse_off = remote_ui_mouse_off;
    ui->mode_change = remote_ui_mode_change;
    ui->set_scroll_region = remote_ui_set_scroll_region;
    ui->scroll = remote_ui_scroll;
    ui->highlight_set = remote_ui_highlight_set;
    ui->put = remote_ui_put;
    ui->bell = remote_ui_bell;
    ui->visual_bell = remote_ui_visual_bell;
    ui->update_fg = remote_ui_update_fg;
    ui->update_bg = remote_ui_update_bg;
    ui->update_sp = remote_ui_update_sp;
    ui->flush = remote_ui_flush;
    ui->suspend = remote_ui_suspend;
    ui->set_title = remote_ui_set_title;
    ui->set_icon = remote_ui_set_icon;
    ui->event = remote_ui_event;

    memset(ui->ui_ext, 0, sizeof(ui->ui_ext));

    for(size_t i = 0; i < options.size; i++)
    {
        ui_set_option(ui, options.items[i].key,
                      options.items[i].value, err);

        if(ERROR_SET(err))
        {
            xfree(ui);
            return;
        }
    }

    ui_data_st *data = xmalloc(sizeof(ui_data_st));
    data->channel_id = channel_id;
    data->buffer = (Array)ARRAY_DICT_INIT;
    ui->data = data;

    pmap_put(uint64_t)(connected_uis, channel_id, ui);
    ui_attach_impl(ui);
}

void nvim_ui_detach(uint64_t channel_id, error_st *err)
FUNC_API_SINCE(1)
FUNC_API_REMOTE_ONLY
{
    if(!pmap_has(uint64_t)(connected_uis, channel_id))
    {
        api_set_error(err, kErrorTypeException,
                      "UI is not attached for channel");
        return;
    }

    remote_ui_disconnect(channel_id);
}


void nvim_ui_try_resize(uint64_t channel_id,
                        Integer width,
                        Integer height,
                        error_st *err)
FUNC_API_SINCE(1)
FUNC_API_REMOTE_ONLY
{
    if(!pmap_has(uint64_t)(connected_uis, channel_id))
    {
        api_set_error(err, kErrorTypeException,
                      "UI is not attached for channel");
        return;
    }

    if(width <= 0 || height <= 0)
    {
        api_set_error(err, kErrorTypeValidation,
                      "Expected width > 0 and height > 0");
        return;
    }

    ui_st *ui = pmap_get(uint64_t)(connected_uis, channel_id);
    ui->width = (int)width;
    ui->height = (int)height;
    ui_refresh();
}

void nvim_ui_set_option(uint64_t channel_id,
                        String name,
                        Object value,
                        error_st *error)
FUNC_API_SINCE(1)
FUNC_API_REMOTE_ONLY
{
    if(!pmap_has(uint64_t)(connected_uis, channel_id))
    {
        api_set_error(error, kErrorTypeException,
                      "UI is not attached for channel");
        return;
    }

    ui_st *ui = pmap_get(uint64_t)(connected_uis, channel_id);

    ui_set_option(ui, name, value, error);

    if(!ERROR_SET(error))
    {
        ui_refresh();
    }
}

static void ui_set_option(ui_st *ui,
                          String name,
                          Object value,
                          error_st *error)
{
#define UI_EXT_OPTION(o, e)                                                   \
    do                                                                        \
    {                                                                         \
        if(xstrequal(name.data, #o))                                           \
        {                                                                     \
            if(value.type != kObjectTypeBoolean)                              \
            {                                                                 \
                api_set_error(error,                                          \
                              kErrorTypeValidation, #o " must be a Boolean"); \
                return;                                                       \
            }                                                                 \
            ui->ui_ext[(e)] = value.data.boolean;                             \
            return;                                                           \
        }                                                                     \
    } while (0)

    if(xstrequal(name.data, "rgb"))
    {
        if(value.type != kObjectTypeBoolean)
        {
            api_set_error(error, kErrorTypeValidation, "rgb must be a Boolean");
            return;
        }

        ui->rgb = value.data.boolean;
        return;
    }

    UI_EXT_OPTION(ext_cmdline, kUICmdline);
    UI_EXT_OPTION(ext_popupmenu, kUIPopupmenu);
    UI_EXT_OPTION(ext_tabline, kUITabline);
    UI_EXT_OPTION(ext_wildmenu, kUIWildmenu);

    if(xstrequal(name.data, "popupmenu_external"))
    {
        // LEGACY: Deprecated option, use `ui_ext` instead.
        if(value.type != kObjectTypeBoolean)
        {
            api_set_error(error, kErrorTypeValidation,
                          "popupmenu_external must be a Boolean");
            return;
        }

        ui->ui_ext[kUIPopupmenu] = value.data.boolean;
        return;
    }

    api_set_error(error, kErrorTypeValidation, "No such ui option");
#undef UI_EXT_OPTION
}

static void push_call(ui_st *ui, char *name, Array args)
{
    Array call = ARRAY_DICT_INIT;
    ui_data_st *data = ui->data;

    // To optimize data transfer(especially for "put"), we bundle adjacent
    // calls to same method together, so only add a new call entry if the last
    // method call is different from "name"
    if(kv_size(data->buffer))
    {
        call = kv_A(data->buffer, kv_size(data->buffer) - 1).data.array;
    }

    if(!kv_size(call) || strcmp(kv_A(call, 0).data.string.data, name))
    {
        call = (Array)ARRAY_DICT_INIT;
        ADD(data->buffer, ARRAY_OBJ(call));
        ADD(call, STRING_OBJ(cstr_to_string(name)));
    }

    ADD(call, ARRAY_OBJ(args));
    kv_A(data->buffer, kv_size(data->buffer) - 1).data.array = call;
}

static void remote_ui_highlight_set(ui_st *ui, uihl_attr_st attrs)
{
    Array args = ARRAY_DICT_INIT;
    Dictionary hl = ARRAY_DICT_INIT;

    if(attrs.bold)
    {
        PUT(hl, "bold", BOOLEAN_OBJ(true));
    }

    if(attrs.underline)
    {
        PUT(hl, "underline", BOOLEAN_OBJ(true));
    }

    if(attrs.undercurl)
    {
        PUT(hl, "undercurl", BOOLEAN_OBJ(true));
    }

    if(attrs.italic)
    {
        PUT(hl, "italic", BOOLEAN_OBJ(true));
    }

    if(attrs.reverse)
    {
        PUT(hl, "reverse", BOOLEAN_OBJ(true));
    }

    if(attrs.foreground != -1)
    {
        PUT(hl, "foreground", INTEGER_OBJ(attrs.foreground));
    }

    if(attrs.background != -1)
    {
        PUT(hl, "background", INTEGER_OBJ(attrs.background));
    }

    if(attrs.special != -1)
    {
        PUT(hl, "special", INTEGER_OBJ(attrs.special));
    }

    ADD(args, DICTIONARY_OBJ(hl));
    push_call(ui, "highlight_set", args);
}

static void remote_ui_flush(ui_st *ui)
{
    ui_data_st *data = ui->data;

    if(data->buffer.size > 0)
    {
        channel_send_event(data->channel_id, "redraw", data->buffer);
        data->buffer = (Array)ARRAY_DICT_INIT;
    }
}

static void remote_ui_event(ui_st *ui,
                            char *name,
                            Array args,
                            bool *args_consumed)
{
    Array my_args = ARRAY_DICT_INIT;

    // Objects are currently single-reference
    // make a copy, but only if necessary
    if(*args_consumed)
    {
        for(size_t i = 0; i < args.size; i++)
        {
            ADD(my_args, copy_object(args.items[i]));
        }
    }
    else
    {
        my_args = args;
        *args_consumed = true;
    }

    push_call(ui, name, my_args);
}
