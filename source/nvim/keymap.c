/// @file nvim/keymap.c

#include <assert.h>
#include <inttypes.h>
#include <limits.h>

#include "nvim/nvim.h"
#include "nvim/ascii.h"
#include "nvim/keymap.h"
#include "nvim/charset.h"
#include "nvim/memory.h"
#include "nvim/edit.h"
#include "nvim/eval.h"
#include "nvim/message.h"
#include "nvim/strings.h"
#include "nvim/mouse.h"
#include "nvim/utils.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "keymap.c.generated.h"
#endif

/// Some useful tables.
static struct modmasktable_s
{
    short mod_mask;  ///< Bit-mask for particular key modifier
    short mod_flag;  ///< Bit(s) for particular key modifier
    uchar_kt name;   ///< Single letter name of modifier
} mod_mask_table[] =
{
    { MOD_MASK_ALT,          MOD_MASK_ALT,     (uchar_kt)'M' },
    { MOD_MASK_META,         MOD_MASK_META,    (uchar_kt)'T' },
    { MOD_MASK_CTRL,         MOD_MASK_CTRL,    (uchar_kt)'C' },
    { MOD_MASK_SHIFT,        MOD_MASK_SHIFT,   (uchar_kt)'S' },
    { MOD_MASK_MULTI_CLICK,  MOD_MASK_2CLICK,  (uchar_kt)'2' },
    { MOD_MASK_MULTI_CLICK,  MOD_MASK_3CLICK,  (uchar_kt)'3' },
    { MOD_MASK_MULTI_CLICK,  MOD_MASK_4CLICK,  (uchar_kt)'4' },
    { MOD_MASK_CMD,          MOD_MASK_CMD,     (uchar_kt)'D' },
    { MOD_MASK_ALT,          MOD_MASK_ALT,     (uchar_kt)'A' },
    { 0, 0, NUL } // 'A' must be the last one; NUL for the end
};

/// Shifted key terminal codes and their unshifted equivalent.
/// Don't add mouse codes here, they are handled separately!
#define MOD_KEYS_ENTRY_SIZE    5

static uchar_kt modifier_keys_table[] =
{
    // mod mask     with modifier               without modifier
    MOD_MASK_SHIFT, '&', '9',                   '@', '1',    // begin
    MOD_MASK_SHIFT, '&', '0',                   '@', '2',    // cancel
    MOD_MASK_SHIFT, '*', '1',                   '@', '4',    // command
    MOD_MASK_SHIFT, '*', '2',                   '@', '5',    // copy
    MOD_MASK_SHIFT, '*', '3',                   '@', '6',    // create
    MOD_MASK_SHIFT, '*', '4',                   'k', 'D',    // delete char
    MOD_MASK_SHIFT, '*', '5',                   'k', 'L',    // delete line
    MOD_MASK_SHIFT, '*', '7',                   '@', '7',    // end
    MOD_MASK_CTRL , KS_EXTRA, (int)KE_C_END,    '@', '7',    // end
    MOD_MASK_SHIFT, '*', '9',                   '@', '9',    // exit
    MOD_MASK_SHIFT, '*', '0',                   '@', '0',    // find
    MOD_MASK_SHIFT, '#', '1',                   '%', '1',    // help
    MOD_MASK_SHIFT, '#', '2',                   'k', 'h',    // home
    MOD_MASK_CTRL , KS_EXTRA, (int)KE_C_HOME,   'k', 'h',    // home
    MOD_MASK_SHIFT, '#', '3',                   'k', 'I',    // insert
    MOD_MASK_SHIFT, '#', '4',                   'k', 'l',    // left arrow
    MOD_MASK_CTRL , KS_EXTRA, (int)KE_C_LEFT,   'k', 'l',    // left arrow
    MOD_MASK_SHIFT, '%', 'a',                   '%', '3',    // message
    MOD_MASK_SHIFT, '%', 'b',                   '%', '4',    // move
    MOD_MASK_SHIFT, '%', 'c',                   '%', '5',    // next
    MOD_MASK_SHIFT, '%', 'd',                   '%', '7',    // options
    MOD_MASK_SHIFT, '%', 'e',                   '%', '8',    // previous
    MOD_MASK_SHIFT, '%', 'f',                   '%', '9',    // print
    MOD_MASK_SHIFT, '%', 'g',                   '%', '0',    // redo
    MOD_MASK_SHIFT, '%', 'h',                   '&', '3',    // replace
    MOD_MASK_SHIFT, '%', 'i',                   'k', 'r',    // right arr.
    MOD_MASK_CTRL , KS_EXTRA, (int)KE_C_RIGHT,  'k', 'r',    // right arr.
    MOD_MASK_SHIFT, '%', 'j',                   '&', '5',    // resume
    MOD_MASK_SHIFT, '!', '1',                   '&', '6',    // save
    MOD_MASK_SHIFT, '!', '2',                   '&', '7',    // suspend
    MOD_MASK_SHIFT, '!', '3',                   '&', '8',    // undo
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_UP,     'k', 'u',    // up arrow
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_DOWN,   'k', 'd',    // down arrow

    /* vt100 F1 */
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_XF1,  KS_EXTRA, (int)KE_XF1,
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_XF2,  KS_EXTRA, (int)KE_XF2,
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_XF3,  KS_EXTRA, (int)KE_XF3,
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_XF4,  KS_EXTRA, (int)KE_XF4,

    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F1,   'k', '1',    // F1
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F2,   'k', '2',    // F2
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F3,   'k', '3',    // F3
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F4,   'k', '4',    // F4
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F5,   'k', '5',    // F5
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F6,   'k', '6',    // F6
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F7,   'k', '7',    // F7
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F8,   'k', '8',    // F8
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F9,   'k', '9',    // F9
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F10,  'k', ';',    // F10

    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F11,  'F', '1',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F12,  'F', '2',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F13,  'F', '3',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F14,  'F', '4',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F15,  'F', '5',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F16,  'F', '6',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F17,  'F', '7',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F18,  'F', '8',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F19,  'F', '9',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F20,  'F', 'A',

    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F21,  'F', 'B',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F22,  'F', 'C',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F23,  'F', 'D',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F24,  'F', 'E',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F25,  'F', 'F',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F26,  'F', 'G',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F27,  'F', 'H',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F28,  'F', 'I',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F29,  'F', 'J',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F30,  'F', 'K',

    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F31,  'F', 'L',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F32,  'F', 'M',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F33,  'F', 'N',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F34,  'F', 'O',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F35,  'F', 'P',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F36,  'F', 'Q',
    MOD_MASK_SHIFT, KS_EXTRA, (int)KE_S_F37,  'F', 'R',

    // TAB pseudo code
    MOD_MASK_SHIFT, 'k', 'B', KS_EXTRA, (int)KE_TAB,

    // terminal NUL
    NUL
};

static struct key_name_entry_s
{
    int key;        ///< Special key code or ascii value
    uchar_kt *name; ///< Name of key
} key_names_table[] =
{
    { ' ',      (uchar_kt *)"Space"     },
    { TAB,      (uchar_kt *)"Tab"       },
    { K_TAB,    (uchar_kt *)"Tab"       },
    { NL,       (uchar_kt *)"NL"        },
    { NL,       (uchar_kt *)"NewLine"   },  // Alternative name
    { NL,       (uchar_kt *)"LineFeed"  },  // Alternative name
    { NL,       (uchar_kt *)"LF"        },  // Alternative name
    { CAR,      (uchar_kt *)"CR"        },
    { CAR,      (uchar_kt *)"Return"    },  // Alternative name
    { CAR,      (uchar_kt *)"Enter"     },  // Alternative name
    { K_BS,     (uchar_kt *)"BS"        },
    { K_BS,     (uchar_kt *)"BackSpace" },  // Alternative name
    { ESC,      (uchar_kt *)"Esc"       },
    { CSI,      (uchar_kt *)"CSI"       },
    { K_CSI,    (uchar_kt *)"xCSI"      },
    { '|',      (uchar_kt *)"Bar"       },
    { '\\',     (uchar_kt *)"Bslash"    },
    { K_DEL,    (uchar_kt *)"Del"       },
    { K_DEL,    (uchar_kt *)"Delete"    },  // Alternative name
    { K_KDEL,   (uchar_kt *)"kDel"      },
    { K_UP,     (uchar_kt *)"Up"        },
    { K_DOWN,   (uchar_kt *)"Down"      },
    { K_LEFT,   (uchar_kt *)"Left"      },
    { K_RIGHT,  (uchar_kt *)"Right"     },
    { K_XUP,    (uchar_kt *)"xUp"       },
    { K_XDOWN,  (uchar_kt *)"xDown"     },
    { K_XLEFT,  (uchar_kt *)"xLeft"     },
    { K_XRIGHT, (uchar_kt *)"xRight"    },

    { K_F1,  (uchar_kt *)"F1"   },
    { K_F2,  (uchar_kt *)"F2"   },
    { K_F3,  (uchar_kt *)"F3"   },
    { K_F4,  (uchar_kt *)"F4"   },
    { K_F5,  (uchar_kt *)"F5"   },
    { K_F6,  (uchar_kt *)"F6"   },
    { K_F7,  (uchar_kt *)"F7"   },
    { K_F8,  (uchar_kt *)"F8"   },
    { K_F9,  (uchar_kt *)"F9"   },
    { K_F10, (uchar_kt *)"F10"  },

    { K_F11, (uchar_kt *)"F11"  },
    { K_F12, (uchar_kt *)"F12"  },
    { K_F13, (uchar_kt *)"F13"  },
    { K_F14, (uchar_kt *)"F14"  },
    { K_F15, (uchar_kt *)"F15"  },
    { K_F16, (uchar_kt *)"F16"  },
    { K_F17, (uchar_kt *)"F17"  },
    { K_F18, (uchar_kt *)"F18"  },
    { K_F19, (uchar_kt *)"F19"  },
    { K_F20, (uchar_kt *)"F20"  },

    { K_F21, (uchar_kt *)"F21"  },
    { K_F22, (uchar_kt *)"F22"  },
    { K_F23, (uchar_kt *)"F23"  },
    { K_F24, (uchar_kt *)"F24"  },
    { K_F25, (uchar_kt *)"F25"  },
    { K_F26, (uchar_kt *)"F26"  },
    { K_F27, (uchar_kt *)"F27"  },
    { K_F28, (uchar_kt *)"F28"  },
    { K_F29, (uchar_kt *)"F29"  },
    { K_F30, (uchar_kt *)"F30"  },

    { K_F31, (uchar_kt *)"F31"  },
    { K_F32, (uchar_kt *)"F32"  },
    { K_F33, (uchar_kt *)"F33"  },
    { K_F34, (uchar_kt *)"F34"  },
    { K_F35, (uchar_kt *)"F35"  },
    { K_F36, (uchar_kt *)"F36"  },
    { K_F37, (uchar_kt *)"F37"  },

    { K_XF1, (uchar_kt *)"xF1"  },
    { K_XF2, (uchar_kt *)"xF2"  },
    { K_XF3, (uchar_kt *)"xF3"  },
    { K_XF4, (uchar_kt *)"xF4"  },

    { K_HELP,      (uchar_kt *)"Help"       },
    { K_UNDO,      (uchar_kt *)"Undo"       },
    { K_INS,       (uchar_kt *)"Insert"     },
    { K_INS,       (uchar_kt *)"Ins"        },  // Alternative name
    { K_KINS,      (uchar_kt *)"kInsert"    },
    { K_HOME,      (uchar_kt *)"Home"       },
    { K_KHOME,     (uchar_kt *)"kHome"      },
    { K_XHOME,     (uchar_kt *)"xHome"      },
    { K_ZHOME,     (uchar_kt *)"zHome"      },
    { K_END,       (uchar_kt *)"End"        },
    { K_KEND,      (uchar_kt *)"kEnd"       },
    { K_XEND,      (uchar_kt *)"xEnd"       },
    { K_ZEND,      (uchar_kt *)"zEnd"       },
    { K_PAGEUP,    (uchar_kt *)"PageUp"     },
    { K_PAGEDOWN,  (uchar_kt *)"PageDown"   },
    { K_KPAGEUP,   (uchar_kt *)"kPageUp"    },
    { K_KPAGEDOWN, (uchar_kt *)"kPageDown"  },

    { K_KPLUS,     (uchar_kt *)"kPlus"      },
    { K_KMINUS,    (uchar_kt *)"kMinus"     },
    { K_KDIVIDE,   (uchar_kt *)"kDivide"    },
    { K_KMULTIPLY, (uchar_kt *)"kMultiply"  },
    { K_KENTER,    (uchar_kt *)"kEnter"     },
    { K_KPOINT,    (uchar_kt *)"kPoint"     },

    { K_K0, (uchar_kt *)"k0"    },
    { K_K1, (uchar_kt *)"k1"    },
    { K_K2, (uchar_kt *)"k2"    },
    { K_K3, (uchar_kt *)"k3"    },
    { K_K4, (uchar_kt *)"k4"    },
    { K_K5, (uchar_kt *)"k5"    },
    { K_K6, (uchar_kt *)"k6"    },
    { K_K7, (uchar_kt *)"k7"    },
    { K_K8, (uchar_kt *)"k8"    },
    { K_K9, (uchar_kt *)"k9"    },

    { '<',  (uchar_kt *)"lt"    },

    { K_MOUSE,          (uchar_kt *)"Mouse"             },
    { K_LEFTMOUSE,      (uchar_kt *)"LeftMouse"         },
    { K_LEFTMOUSE_NM,   (uchar_kt *)"LeftMouseNM"       },
    { K_LEFTDRAG,       (uchar_kt *)"LeftDrag"          },
    { K_LEFTRELEASE,    (uchar_kt *)"LeftRelease"       },
    { K_LEFTRELEASE_NM, (uchar_kt *)"LeftReleaseNM"     },
    { K_MIDDLEMOUSE,    (uchar_kt *)"MiddleMouse"       },
    { K_MIDDLEDRAG,     (uchar_kt *)"MiddleDrag"        },
    { K_MIDDLERELEASE,  (uchar_kt *)"MiddleRelease"     },
    { K_RIGHTMOUSE,     (uchar_kt *)"RightMouse"        },
    { K_RIGHTDRAG,      (uchar_kt *)"RightDrag"         },
    { K_RIGHTRELEASE,   (uchar_kt *)"RightRelease"      },
    { K_MOUSEDOWN,      (uchar_kt *)"ScrollWheelUp"     },
    { K_MOUSEUP,        (uchar_kt *)"ScrollWheelDown"   },
    { K_MOUSELEFT,      (uchar_kt *)"ScrollWheelRight"  },
    { K_MOUSERIGHT,     (uchar_kt *)"ScrollWheelLeft"   },
    { K_MOUSEDOWN,      (uchar_kt *)"MouseDown"         },
    { K_MOUSEUP,        (uchar_kt *)"MouseUp"           },
    { K_X1MOUSE,        (uchar_kt *)"X1Mouse"           },
    { K_X1DRAG,         (uchar_kt *)"X1Drag"            },
    { K_X1RELEASE,      (uchar_kt *)"X1Release"         },
    { K_X2MOUSE,        (uchar_kt *)"X2Mouse"           },
    { K_X2DRAG,         (uchar_kt *)"X2Drag"            },
    { K_X2RELEASE,      (uchar_kt *)"X2Release"         },
    { K_DROP,           (uchar_kt *)"Drop"              },
    { K_ZERO,           (uchar_kt *)"Nul"               },
    { K_SNR,            (uchar_kt *)"SNR"               },
    { K_PLUG,           (uchar_kt *)"Plug"              },
    { K_PASTE,          (uchar_kt *)"Paste"             },
    { K_FOCUSGAINED,    (uchar_kt *)"FocusGained"       },
    { K_FOCUSLOST,      (uchar_kt *)"FocusLost"         },
    { 0,                NULL                            }
};

static struct mousetable_s
{
    int pseudo_code; ///< Code for pseudo mouse event
    int button;      ///< Which mouse button is it ?
    int is_click;    ///< Is it a mouse button click event ?
    int is_drag;     ///< Is it a mouse drag event ?
} mouse_table[] =
{
    { (int)KE_LEFTMOUSE,      MOUSE_LEFT,     TRUE,   FALSE },
    { (int)KE_LEFTDRAG,       MOUSE_LEFT,     FALSE,  TRUE  },
    { (int)KE_LEFTRELEASE,    MOUSE_LEFT,     FALSE,  FALSE },
    { (int)KE_MIDDLEMOUSE,    MOUSE_MIDDLE,   TRUE,   FALSE },
    { (int)KE_MIDDLEDRAG,     MOUSE_MIDDLE,   FALSE,  TRUE  },
    { (int)KE_MIDDLERELEASE,  MOUSE_MIDDLE,   FALSE,  FALSE },
    { (int)KE_RIGHTMOUSE,     MOUSE_RIGHT,    TRUE,   FALSE },
    { (int)KE_RIGHTDRAG,      MOUSE_RIGHT,    FALSE,  TRUE  },
    { (int)KE_RIGHTRELEASE,   MOUSE_RIGHT,    FALSE,  FALSE },
    { (int)KE_X1MOUSE,        MOUSE_X1,       TRUE,   FALSE },
    { (int)KE_X1DRAG,         MOUSE_X1,       FALSE,  TRUE  },
    { (int)KE_X1RELEASE,      MOUSE_X1,       FALSE,  FALSE },
    { (int)KE_X2MOUSE,        MOUSE_X2,       TRUE,   FALSE },
    { (int)KE_X2DRAG,         MOUSE_X2,       FALSE,  TRUE  },
    { (int)KE_X2RELEASE,      MOUSE_X2,       FALSE,  FALSE },
    // DRAG without CLICK
    { (int)KE_IGNORE,         MOUSE_RELEASE,  FALSE,  TRUE  },
    // RELEASE without CLICK
    { (int)KE_IGNORE,         MOUSE_RELEASE,  FALSE,  FALSE },
    { 0,                      0,              0,      0     },
};

/// Return the modifier mask bit (MOD_MASK_*) which corresponds to the given
/// modifier name ('S' for Shift, 'C' for Ctrl etc).
int name_to_mod_mask(int c)
{
    int i;
    c = TOUPPER_ASC(c);

    for(i = 0; mod_mask_table[i].mod_mask != 0; i++)
    {
        if(c == mod_mask_table[i].name)
        {
            return mod_mask_table[i].mod_flag;
        }
    }

    return 0;
}

/// Check if if there is a special key code for "key"
/// that includes the modifiers specified.
int simplify_key(int key, int *modifiers)
{
    int i;
    int key0;
    int key1;

    if(*modifiers & (MOD_MASK_SHIFT | MOD_MASK_CTRL | MOD_MASK_ALT))
    {
        // TAB is a special case
        if(key == TAB && (*modifiers & MOD_MASK_SHIFT))
        {
            *modifiers &= ~MOD_MASK_SHIFT;
            return K_S_TAB;
        }

        key0 = KEY2TERMCAP0(key);
        key1 = KEY2TERMCAP1(key);

        for(i = 0; modifier_keys_table[i] != NUL; i += MOD_KEYS_ENTRY_SIZE)
        {
            if(key0 == modifier_keys_table[i + 3]
               && key1 == modifier_keys_table[i + 4]
               && (*modifiers & modifier_keys_table[i]))
            {
                *modifiers &= ~modifier_keys_table[i];

                return TERMCAP2KEY(modifier_keys_table[i + 1],
                                   modifier_keys_table[i + 2]);
            }
        }
    }

    return key;
}

/// Change <xHome> to <Home>, <xUp> to <Up>, etc.
int handle_x_keys(int key)
{
    switch(key)
    {
        case K_XUP:
            return K_UP;

        case K_XDOWN:
            return K_DOWN;

        case K_XLEFT:
            return K_LEFT;

        case K_XRIGHT:
            return K_RIGHT;

        case K_XHOME:
            return K_HOME;

        case K_ZHOME:
            return K_HOME;

        case K_XEND:
            return K_END;

        case K_ZEND:
            return K_END;

        case K_XF1:
            return K_F1;

        case K_XF2:
            return K_F2;

        case K_XF3:
            return K_F3;

        case K_XF4:
            return K_F4;

        case K_S_XF1:
            return K_S_F1;

        case K_S_XF2:
            return K_S_F2;

        case K_S_XF3:
            return K_S_F3;

        case K_S_XF4:
            return K_S_F4;
    }

    return key;
}

/// Return a string which contains the name of the given key when the given
/// modifiers are down.
uchar_kt *get_special_key_name(int c, int modifiers)
{
    static uchar_kt string[MAX_KEY_NAME_LEN + 1];
    int i, idx;
    int table_idx;
    uchar_kt  *s;
    string[0] = '<';
    idx = 1;

    // Key that stands for a normal character.
    if(IS_SPECIAL(c) && KEY2TERMCAP0(c) == KS_KEY)
    {
        c = KEY2TERMCAP1(c);
    }

    // Translate shifted special keys into unshifted
    // keys and set modifier. Same for CTRL and ALT modifiers.
    if(IS_SPECIAL(c))
    {
        for(i = 0; modifier_keys_table[i] != 0; i += MOD_KEYS_ENTRY_SIZE)
        {
            if(KEY2TERMCAP0(c) == (int)modifier_keys_table[i + 1]
               && (int)KEY2TERMCAP1(c) == (int)modifier_keys_table[i + 2])
            {
                modifiers |= modifier_keys_table[i];

                c = TERMCAP2KEY(modifier_keys_table[i + 3],
                                modifier_keys_table[i + 4]);
                break;
            }
        }
    }

    // try to find the key in the special key table
    table_idx = find_special_key_in_table(c);

    // When not a known special key, and not a printable
    // character, try to extract modifiers.
    if(c > 0 && (*mb_char2len)(c) == 1)
    {
        if(table_idx < 0
           && (!is_print_char(c) || (c & 0x7f) == ' ')
           && (c & 0x80))
        {
            c &= 0x7f;
            modifiers |= MOD_MASK_ALT;

            // try again, to find the un-alted key in the special key table
            table_idx = find_special_key_in_table(c);
        }

        if(table_idx < 0 && !is_print_char(c) && c < ' ')
        {
            c += '@';
            modifiers |= MOD_MASK_CTRL;
        }
    }

    // translate the modifier into a string
    for(i = 0; mod_mask_table[i].name != 'A'; i++)
    {
        if((modifiers & mod_mask_table[i].mod_mask)
           == mod_mask_table[i].mod_flag)
        {
            string[idx++] = mod_mask_table[i].name;
            string[idx++] = (uchar_kt)'-';
        }
    }

    if(table_idx < 0) // unknown special key, may output t_xx
    {
        if(IS_SPECIAL(c))
        {
            string[idx++] = 't';
            string[idx++] = '_';
            string[idx++] = (uchar_kt)KEY2TERMCAP0(c);
            string[idx++] = KEY2TERMCAP1(c);
        }
        else
        {
            // Not a special key, only modifiers, output directly
            if((*mb_char2len)(c) > 1)
            {
                idx += (*mb_char2bytes)(c, string + idx);
            }
            else if(is_print_char(c))
            {
                string[idx++] = (uchar_kt)c;
            }
            else
            {
                s = transchar(c);

                while(*s)
                {
                    string[idx++] = *s++;
                }
            }
        }
    }
    else // use name of special key
    {
        ustrcpy(string + idx, key_names_table[table_idx].name);
        idx = (int)ustrlen(string);
    }

    string[idx++] = '>';
    string[idx] = NUL;

    return string;
}

/// Try translating a <> name
///
/// @param[in,out]  srcp
/// Source from which <> are translated.
/// Is advanced to after the <> name if there is a match.
///
/// @param[in]  src_len
/// Length of the srcp.
///
/// @param[out]  dst
/// Location where translation result will be kept.
/// Must have at least six bytes.
///
/// @param[in]  keycode
/// Prefer key code, e.g. K_DEL in place of DEL.
///
/// @param[in]  in_string
/// Inside a double quoted string
///
/// @return Number of characters added to dst, zero for no match.
unsigned int trans_special(const uchar_kt **srcp,
                           const size_t src_len,
                           uchar_kt *const dst,
                           const bool keycode,
                           const bool in_string)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    int modifiers = 0;
    int key;
    unsigned int dlen = 0;

    key = find_special_key(srcp, src_len, &modifiers,
                           keycode, false, in_string);

    if(key == 0)
    {
        return 0;
    }

    // Put the appropriate modifier in a string
    if(modifiers != 0)
    {
        dst[dlen++] = K_SPECIAL;
        dst[dlen++] = KS_MODIFIER;
        dst[dlen++] = (uchar_kt)modifiers;
    }

    if(IS_SPECIAL(key))
    {
        dst[dlen++] = K_SPECIAL;
        dst[dlen++] = (uchar_kt)KEY2TERMCAP0(key);
        dst[dlen++] = KEY2TERMCAP1(key);
    }
    else if(!keycode)
    {
        dlen += (unsigned int)(*mb_char2bytes)(key, dst + dlen);
    }
    else if(keycode)
    {
        uchar_kt *after = add_char2buf(key, dst + dlen);
        assert(after >= dst && (uintmax_t)(after - dst) <= UINT_MAX);
        dlen = (unsigned int)(after - dst);
    }
    else
    {
        dst[dlen++] = (uchar_kt)key;
    }

    return dlen;
}

/// Try translating a <> name
///
/// @param[in,out] srcp
/// Translated <> name. Is advanced to after the <> name.
///
/// @param[in] src_len
/// srcp length.
///
/// @param[out] modp
/// Location where information about modifiers is saved.
///
/// @param[in] keycode
/// Prefer key code, e.g. K_DEL in place of DEL.
///
/// @param[in] keep_x_key
/// Don’t translate xHome to Home key.
///
/// @param[in] in_string
/// In string, double quote is escaped
///
/// @return Key and modifiers or 0 if there is no match.
int find_special_key(const uchar_kt **srcp,
                     const size_t src_len,
                     int *const modp,
                     const bool keycode,
                     const bool keep_x_key,
                     const bool in_string)
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_NONNULL_ALL
{
    const uchar_kt *last_dash;
    const uchar_kt *end_of_name;
    const uchar_kt *src;
    const uchar_kt *bp;
    const uchar_kt *const end = *srcp + src_len - 1;
    int modifiers;
    int bit;
    int key;
    unsigned long n;
    int l;

    if(src_len == 0)
    {
        return 0;
    }

    src = *srcp;

    if(src[0] != '<')
    {
        return 0;
    }

    // Find end of modifier list
    last_dash = src;

    for(bp = src + 1; bp <= end && (*bp == '-' || is_id_char(*bp)); bp++)
    {
        if(*bp == '-')
        {
            last_dash = bp;

            if(bp + 1 <= end)
            {
                l = mb_ptr2len_len(bp + 1, (int)(end - bp) + 1);

                // Anything accepted, like <C-?>.
                // <C-"> or <M-"> are not special in strings as " is
                // the string delimiter. With a backslash it works: <M-\">
                if(end - bp > l
                   && !(in_string && bp[1] == '"')
                   && bp[2] == '>')
                {
                    bp += l;
                }
                else if(end - bp > 2
                        && in_string
                        && bp[1] == '\\'
                        && bp[2] == '"'
                        && bp[3] == '>')
                {
                    bp += 2;
                }
            }
        }

        if(end - bp > 3 && bp[0] == 't' && bp[1] == '_')
        {
            bp += 3; // skip t_xx, xx may be '-' or '>'
        }
        else if(end - bp > 4 && ustrnicmp(bp, "char-", 5) == 0)
        {
            str_to_num(bp + 5, NULL, &l, kStrToNumAll, NULL, NULL, 0);
            bp += l + 5;
            break;
        }
    }

    if(bp <= end && *bp == '>') // found matching '>'
    {
        end_of_name = bp + 1;

        // Which modifiers are given?
        modifiers = 0x0;

        for(bp = src + 1; bp < last_dash; bp++)
        {
            if(*bp != '-')
            {
                bit = name_to_mod_mask(*bp);

                if(bit == 0x0)
                {
                    break; // Illegal modifier name
                }

                modifiers |= bit;
            }
        }

        // Legal modifier name.
        if(bp >= last_dash)
        {
            if(ustrnicmp(last_dash + 1, "char-", 5) == 0
               && ascii_isdigit(last_dash[6]))
            {
                // <Char-123> or <Char-033> or <Char-0x33>
                str_to_num(last_dash + 6, NULL, NULL, kStrToNumAll, NULL, &n, 0);
                key = (int)n;
            }
            else
            {
                int off = 1;

                // Modifier with single letter, or special key name.
                if(in_string && last_dash[1] == '\\' && last_dash[2] == '"')
                {
                    off = 2;
                }

                l = mb_ptr2len(last_dash + 1);

                if(modifiers != 0 && last_dash[l + 1] == '>')
                {
                    key = mb_ptr2char(last_dash + off);
                }
                else
                {
                    key = get_special_key_code(last_dash + off);

                    if(!keep_x_key)
                    {
                        key = handle_x_keys(key);
                    }
                }
            }

            // get_special_key_code() may return NUL
            // for invalid special key name.
            if(key != NUL)
            {
                // Only use a modifier when there is no
                // special key code that includes the modifier.
                key = simplify_key(key, &modifiers);

                if(!keycode)
                {
                    // don't want keycode, use single byte code
                    if(key == K_BS)
                    {
                        key = BS;
                    }
                    else if(key == K_DEL || key == K_KDEL)
                    {
                        key = DEL;
                    }
                }

                // Normal Key with modifier:
                // Try to make a single byte code
                // (except for Alt/Meta modifiers).
                if(!IS_SPECIAL(key))
                {
                    key = extract_modifiers(key, &modifiers);
                }

                *modp = modifiers;
                *srcp = end_of_name;
                return key;
            }
        }
    }

    return 0;
}

/// Try to include modifiers (except alt/meta) in the key.
/// Changes "Shift-a" to 'A', "Ctrl-@" to <Nul>, etc.
static int extract_modifiers(int key, int *modp)
{
    int modifiers = *modp;

    if(!(modifiers & MOD_MASK_CMD)) // Command-key is special
    {
        if((modifiers & MOD_MASK_SHIFT) && ASCII_ISALPHA(key))
        {
            key = TOUPPER_ASC(key);
            modifiers &= ~MOD_MASK_SHIFT;
        }
    }

    if((modifiers & MOD_MASK_CTRL)
       && ((key >= '?' && key <= '_') || ASCII_ISALPHA(key)))
    {
        key = Ctrl_chr(key);
        modifiers &= ~MOD_MASK_CTRL;

        if(key == 0) // <C-@> is <Nul>
        {
            key = K_ZERO;
        }
    }

    *modp = modifiers;
    return key;
}

/// Try to find key "c" in the special key table.
/// Return the index when found, -1 when not found.
int find_special_key_in_table(int c)
{
    int i;

    for(i = 0; key_names_table[i].name != NULL; i++)
    {
        if(c == key_names_table[i].key)
        {
            break;
        }
    }

    if(key_names_table[i].name == NULL)
    {
        i = -1;
    }

    return i;
}

/// Find the special key with the given name (the given string does not have
/// to end with NUL, the name is assumed to end before the first non-idchar).
/// If the name starts with "t_" the next two characters are interpreted as a
/// termcap name.
/// Return the key code, or 0 if not found.
int get_special_key_code(const uchar_kt *name)
{
    uchar_kt  *table_name;
    int i, j;

    for(i = 0; key_names_table[i].name != NULL; i++)
    {
        table_name = key_names_table[i].name;

        for(j = 0; is_id_char(name[j]) && table_name[j] != NUL; j++)
        {
            if(TOLOWER_ASC(table_name[j]) != TOLOWER_ASC(name[j]))
            {
                break;
            }
        }

        if(!is_id_char(name[j]) && table_name[j] == NUL)
        {
            return key_names_table[i].key;
        }
    }

    return 0;
}

/// Look up the given mouse code to return the relevant information in
/// the other arguments. Return which button is down or was released.
int get_mouse_button(int code, bool *is_click, bool *is_drag)
{
    int i;

    for(i = 0; mouse_table[i].pseudo_code; i++)
    {
        if(code == mouse_table[i].pseudo_code)
        {
            *is_click = mouse_table[i].is_click;
            *is_drag = mouse_table[i].is_drag;
            return mouse_table[i].button;
        }
    }

    return 0; // Shouldn't get here
}

/// Replace any terminal code strings with the equivalent internal representation
///
/// This is used for the "from" and "to" part of a mapping, and the "to" part of
/// a menu command. Any strings like "<C-UP>" are also replaced, unless
/// 'cpoptions' contains '<'. K_SPECIAL by itself is replaced by K_SPECIAL
/// KS_SPECIAL KE_FILLER.
///
/// @param[in]  from
/// What characters to replace.
///
/// @param[in]  from_len
/// Length of the "from" argument.
///
/// @param[out]  bufp
/// Location where results were saved in case of success
/// (allocated). Will be set to NULL in case of failure.
///
/// @param[in]  do_lt
/// If true, also translate <lt>.
///
/// @param[in]  from_part
/// If true, trailing <C-v> is included, otherwise it is
/// removed (to make ":map xx ^V" map xx to nothing).
/// When cpo_flags contains #FLAG_CPO_BSLASH, a backslash can be used
/// in place of <C-v>. All other <C-v> characters are removed.
///
/// @param[in]  special
/// If true, always accept <key> notation.
///
/// @param[in]  cpo_flags
/// Relevant flags derived from p_cpo, see #CPO_TO_CPO_FLAGS.
///
/// @return
/// Pointer to an allocated memory in case of success, "from" in
/// case of failure. In case of success returned pointer is also
/// saved to "bufp".
uchar_kt *replace_termcodes(const uchar_kt *from,
                          const size_t from_len,
                          uchar_kt **bufp,
                          const bool from_part,
                          const bool do_lt,
                          const bool special,
                          int cpo_flags)
FUNC_ATTR_NONNULL_ALL
{
    ssize_t i;
    size_t slen;
    uchar_kt key;
    size_t dlen = 0;
    const uchar_kt *src;
    const uchar_kt *const end = from + from_len - 1;
    int do_backslash; // backslash is a special character
    int do_special; // recognize <> key codes
    uchar_kt *result; // buffer for resulting string

    do_backslash = !(cpo_flags&FLAG_CPO_BSLASH);
    do_special = !(cpo_flags&FLAG_CPO_SPECI) || special;

    // Allocate space for the translation. Worst case a single character is
    // replaced by 6 bytes (shifted special key), plus a NUL at the end.
    result = xmalloc(from_len * 6 + 1);

    src = from;

    // Check for #n at start only: function key n
    if(from_part && from_len > 1 && src[0] == '#'
       && ascii_isdigit(src[1])) // function key
    {
        result[dlen++] = K_SPECIAL;
        result[dlen++] = 'k';

        if(src[1] == '0')
        {
            result[dlen++] = ';'; // #0 is F10 is "k;"
        }
        else
        {
            result[dlen++] = src[1]; // #3 is F3 is "k3"
        }

        src += 2;
    }

    // Copy each byte from *from to result[dlen]
    while(src <= end)
    {
        // If 'cpoptions' does not contain '<',
        // check for special key codes, like "<C-S-LeftMouse>"
        if(do_special
           && (do_lt || ((end - src) >= 3 && ustrncmp(src, "<lt>", 4) != 0)))
        {
            // Replace <SID> by K_SNR <script-nr> _.
            // (room: 5 * 6 = 30 bytes; needed: 3 + <nr> + 1 <= 14)
            if(end - src >= 4 && ustrnicmp(src, "<SID>", 5) == 0)
            {
                if(current_SID <= 0)
                {
                    EMSG(_(e_usingsid));
                }
                else
                {
                    src += 5;
                    result[dlen++] = K_SPECIAL;
                    result[dlen++] = (int)KS_EXTRA;
                    result[dlen++] = (int)KE_SNR;

                    sprintf((char *)result + dlen,
                            "%" PRId64, (int64_t)current_SID);

                    dlen += ustrlen(result + dlen);
                    result[dlen++] = '_';
                    continue;
                }
            }

            slen = trans_special(&src, (size_t)(end - src) + 1,
                                 result + dlen, true, true);

            if(slen)
            {
                dlen += slen;
                continue;
            }
        }

        if(do_special)
        {
            uchar_kt  *p, *s, len;

            // Replace <Leader> by the value of "mapleader".
            // Replace <LocalLeader> by the value of "maplocalleader".
            // If "mapleader" or "maplocalleader" isn't set use a backslash.
            if(end - src >= 7 && ustrnicmp(src, "<Leader>", 8) == 0)
            {
                len = 8;
                p = get_var_value("g:mapleader");
            }
            else if(end - src >= 12 && ustrnicmp(src, "<LocalLeader>", 13) == 0)
            {
                len = 13;
                p = get_var_value("g:maplocalleader");
            }
            else
            {
                len = 0;
                p = NULL;
            }

            if(len != 0)
            {
                // Allow up to 8 * 6 characters for "mapleader".
                if(p == NULL || *p == NUL || ustrlen(p) > 8 * 6)
                {
                    s = (uchar_kt *)"\\";
                }
                else
                {
                    s = p;
                }

                while(*s != NUL)
                {
                    result[dlen++] = *s++;
                }

                src += len;
                continue;
            }
        }

        // Remove CTRL-V and ignore the next character.
        // For "from" side the CTRL-V at the end is included, for the "to"
        // part it is removed.
        // If 'cpoptions' does not contain 'B', also accept a backslash.
        key = *src;

        if(key == Ctrl_V || (do_backslash && key == '\\'))
        {
            src++; // skip CTRL-V or backslash

            if(src > end)
            {
                if(from_part)
                {
                    result[dlen++] = key;
                }

                break;
            }
        }

        // skip multibyte char correctly
        for(i = (*mb_ptr2len_len)(src, (int)(end - src) + 1); i > 0; i--)
        {
            // If the character is K_SPECIAL, replace it with K_SPECIAL
            // KS_SPECIAL KE_FILLER.
            // If compiled with the GUI replace CSI with K_CSI.
            if(*src == K_SPECIAL)
            {
                result[dlen++] = K_SPECIAL;
                result[dlen++] = KS_SPECIAL;
                result[dlen++] = KE_FILLER;
            }
            else
            {
                result[dlen++] = *src;
            }

            ++src;
        }
    }

    result[dlen] = NUL;

    *bufp = xrealloc(result, dlen + 1);

    return *bufp;
}
