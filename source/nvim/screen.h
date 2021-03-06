/// @file nvim/screen.h

#ifndef NVIM_SCREEN_H
#define NVIM_SCREEN_H

#include <stdbool.h>

#include "nvim/types.h"
#include "nvim/buffer_defs.h"
#include "nvim/pos.h"

// flags for update_screen()
// The higher the value, the higher the priority

#define VALID         10  ///< buffer not changed, or changes marked with b_mod_*
#define INVERTED      20  ///< redisplay inverted part that changed
#define INVERTED_ALL  25  ///< redisplay whole inverted part
#define REDRAW_TOP    30  ///< display first w_upd_rows screen lines
#define SOME_VALID    35  ///< like NOT_VALID but may scroll
#define NOT_VALID     40  ///< buffer needs complete redraw
#define CLEAR         50  ///< screen messed up, clear it

/// Status line click definition
typedef struct stl_clickdef_s
{
    enum
    {
        kStlClickDisabled = 0,  ///< Clicks to this area are ignored.
        kStlClickTabSwitch,     ///< Switch to the given tab.
        kStlClickTabClose,      ///< Close given tab.
        kStlClickFuncRun,       ///< Run user function.
    } type;                     ///< Type of the click.
    int tabnr;                  ///< Tab page number.
    char *func;                 ///< Function to run.
} stl_clickdef_st;

/// Used for tabline clicks
typedef struct stl_clickrec_s
{
    stl_clickdef_st def;  ///< Click definition.
    const char *start;    ///< Location where region starts.
} stl_clickrec_st;

/// Array defining what should be done when tabline is clicked
extern stl_clickdef_st *tab_page_click_defs;

/// Size of the tab_page_click_defs array
extern long tab_page_click_defs_size;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "screen.h.generated.h"
#endif

#endif // NVIM_SCREEN_H
