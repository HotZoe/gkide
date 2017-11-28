/// @file nvim/menu.h

#ifndef NVIM_MENU_H
#define NVIM_MENU_H

#include <stdbool.h>

#include "nvim/types.h"
#include "nvim/ex_cmds_defs.h"

// Indices into vimmenu_T->strings[] and vimmenu_T->noremap[] for each mode
#define MENU_INDEX_INVALID     -1
#define MENU_INDEX_NORMAL       0
#define MENU_INDEX_VISUAL       1
#define MENU_INDEX_SELECT       2
#define MENU_INDEX_OP_PENDING   3
#define MENU_INDEX_INSERT       4
#define MENU_INDEX_CMDLINE      5
#define MENU_INDEX_TIP          6  // note MENU_INDEX_TIP is not a 'real' mode
#define MENU_MODES              7

// Menu modes
#define MENU_NORMAL_MODE        (1 << MENU_INDEX_NORMAL)
#define MENU_VISUAL_MODE        (1 << MENU_INDEX_VISUAL)
#define MENU_SELECT_MODE        (1 << MENU_INDEX_SELECT)
#define MENU_OP_PENDING_MODE    (1 << MENU_INDEX_OP_PENDING)
#define MENU_INSERT_MODE        (1 << MENU_INDEX_INSERT)
#define MENU_CMDLINE_MODE       (1 << MENU_INDEX_CMDLINE)
#define MENU_TIP_MODE           (1 << MENU_INDEX_TIP)
#define MENU_ALL_MODES          ((1 << MENU_INDEX_TIP) - 1)

// Start a menu name with this to not include it on the main menu bar
#define MNU_HIDDEN_CHAR         ']'

typedef struct VimMenu vimmenu_T;

struct VimMenu
{
    int modes;      ///< Which modes is this menu visible for?
    int enabled;    ///< for which modes the menu is enabled
    char_u *name;   ///< Name of menu, possibly translated
    char_u *dname;  ///< Displayed Name ("name" without '&')

    /// @b name untranslated, NULL when @b name was not translated
    char_u *en_name;
    /// @b dname untranslated, NULL when @b dname was not translated
    char_u *en_dname;

    int mnemonic;                ///< mnemonic key (after '&')
    char_u *actext;              ///< accelerator text (after TAB)
    long priority;               ///< Menu order priority
    char_u *strings[MENU_MODES]; ///< Mapped string for each mode
    int noremap[MENU_MODES];     ///< A REMAP_ flag for each mode
    bool silent[MENU_MODES];     ///< A silent flag for each mode
    vimmenu_T *children;         ///< Children of sub-menu
    vimmenu_T *parent;           ///< Parent of menu
    vimmenu_T *next;             ///< Next item in menu
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "menu.h.generated.h"
#endif

#endif // NVIM_MENU_H
