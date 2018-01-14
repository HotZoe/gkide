/// @file nvim/ui.h

#ifndef NVIM_UI_H
#define NVIM_UI_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "api/private/defs.h"
#include "nvim/buffer_defs.h"

typedef enum
{
    kUICmdline = 0,
    kUIPopupmenu,
    kUITabline,
    kUIWildmenu,
} UIWidget;

#define UI_WIDGETS   (kUIWildmenu + 1)

typedef struct uihl_attr_s
{
    bool bold;
    bool underline;
    bool undercurl;
    bool italic;
    bool reverse;
    int foreground;
    int background;
    int special;
} uihl_attr_st;

typedef struct ui_s ui_st;
struct ui_s
{
    bool rgb;
    /// Externalized widgets
    bool ui_ext[UI_WIDGETS];
    int width;
    int height;
    void *data;

    #ifdef INCLUDE_GENERATED_DECLARATIONS
    // automatic generated function pointer definations
    #include "ui_events.generated.h"
    #endif

    void (*stop)(ui_st *ui);
    void (*event)(ui_st *ui, char *name, Array args, bool *args_consumed);
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "ui.h.generated.h"
    #include "ui_events_call.h.generated.h"
#endif

#endif // NVIM_UI_H
