/// @file nvim/cursor_shape.c

#include <assert.h>
#include <stdint.h>
#include "nvim/nvim.h"
#include "nvim/ascii.h"
#include "nvim/cursor_shape.h"
#include "nvim/ex_getln.h"
#include "nvim/charset.h"
#include "nvim/strings.h"
#include "nvim/syntax.h"
#include "nvim/api/private/helpers.h"
#include "nvim/ui.h"

/// Handling of cursor and mouse pointer shapes in various modes.
cursor_info_st shape_table[kCsrShpIdxAllIndexCount] =
{
    // Values are set by 'guicursor' and 'mouseshape'.
    // Adjust the SHAPE_IDX_ defines when changing this!
    { "normal",           0, 0, 0, 700L, 400L, 250L, 0, 0, "n",  SHAPE_CURSOR + SHAPE_MOUSE },
    { "visual",           0, 0, 0, 700L, 400L, 250L, 0, 0, "v",  SHAPE_CURSOR + SHAPE_MOUSE },
    { "insert",           0, 0, 0, 700L, 400L, 250L, 0, 0, "i",  SHAPE_CURSOR + SHAPE_MOUSE },
    { "replace",          0, 0, 0, 700L, 400L, 250L, 0, 0, "r",  SHAPE_CURSOR + SHAPE_MOUSE },
    { "cmdline_normal",   0, 0, 0, 700L, 400L, 250L, 0, 0, "c",  SHAPE_CURSOR + SHAPE_MOUSE },
    { "cmdline_insert",   0, 0, 0, 700L, 400L, 250L, 0, 0, "ci", SHAPE_CURSOR + SHAPE_MOUSE },
    { "cmdline_replace",  0, 0, 0, 700L, 400L, 250L, 0, 0, "cr", SHAPE_CURSOR + SHAPE_MOUSE },
    { "operator",         0, 0, 0, 700L, 400L, 250L, 0, 0, "o",  SHAPE_CURSOR + SHAPE_MOUSE },
    { "visual_select",    0, 0, 0, 700L, 400L, 250L, 0, 0, "ve", SHAPE_CURSOR + SHAPE_MOUSE },
    { "cmdline_hover",    0, 0, 0,   0L,   0L,   0L, 0, 0, "e",  SHAPE_MOUSE },
    { "statusline_hover", 0, 0, 0,   0L,   0L,   0L, 0, 0, "s",  SHAPE_MOUSE },
    { "statusline_drag",  0, 0, 0,   0L,   0L,   0L, 0, 0, "sd", SHAPE_MOUSE },
    { "vsep_hover",       0, 0, 0,   0L,   0L,   0L, 0, 0, "vs", SHAPE_MOUSE },
    { "vsep_drag",        0, 0, 0,   0L,   0L,   0L, 0, 0, "vd", SHAPE_MOUSE },
    { "more",             0, 0, 0,   0L,   0L,   0L, 0, 0, "m",  SHAPE_MOUSE },
    { "more_lastline",    0, 0, 0,   0L,   0L,   0L, 0, 0, "ml", SHAPE_MOUSE },
    { "showmatch",        0, 0, 0, 100L, 100L, 100L, 0, 0, "sm", SHAPE_CURSOR },
};

/// Converts cursor_shapes into an Array of Dictionaries
/// @return Array of the form {[ "cursor_shape": ... ], ...}
Array mode_style_array(void)
{
    Array all = ARRAY_DICT_INIT;

    for(int i = 0; i < kCsrShpIdxAllIndexCount; i++)
    {
        Dictionary dic = ARRAY_DICT_INIT;
        cursor_info_st *cur = &shape_table[i];

        if(cur->used_for & SHAPE_MOUSE)
        {
            PUT(dic, "mouse_shape", INTEGER_OBJ(cur->mshape));
        }

        if(cur->used_for & SHAPE_CURSOR)
        {
            String shape_str;

            switch(cur->shape)
            {
                case kCsrShpBlock:
                    shape_str = cstr_to_string("block");
                    break;

                case kCsrShpVertical:
                    shape_str = cstr_to_string("vertical");
                    break;

                case kCsrShpHorizontal:
                    shape_str = cstr_to_string("horizontal");
                    break;

                default:
                    shape_str = cstr_to_string("unknown");
            }

            PUT(dic, "cursor_shape",    STRING_OBJ(shape_str));
            PUT(dic, "cell_percentage", INTEGER_OBJ(cur->percentage));
            PUT(dic, "blinkwait",       INTEGER_OBJ(cur->blinkwait));
            PUT(dic, "blinkon",         INTEGER_OBJ(cur->blinkon));
            PUT(dic, "blinkoff",        INTEGER_OBJ(cur->blinkoff));
            PUT(dic, "hl_id",           INTEGER_OBJ(cur->id));
            PUT(dic, "id_lm",           INTEGER_OBJ(cur->id_lm));
        }

        PUT(dic, "name",       STRING_OBJ(cstr_to_string(cur->full_name)));
        PUT(dic, "short_name", STRING_OBJ(cstr_to_string(cur->name)));

        ADD(all, DICTIONARY_OBJ(dic));
    }

    return all;
}

/// Parse the 'guicursor' option
///
/// @param what SHAPE_CURSOR or SHAPE_MOUSE ('mouseshape')
///
/// @returns error message for an illegal option, NULL otherwise.
uchar_kt *parse_shape_opt(int what)
{
    uchar_kt *modep;
    uchar_kt *colonp;
    uchar_kt *commap;
    uchar_kt *slashp;
    uchar_kt *p;
    uchar_kt *endp;
    int idx = 0; // init for GCC
    int all_idx;
    int len;
    int i;
    int found_ve = false; // found "ve" flag
    int round;

    // First round: check for errors; second round: do it for real.
    for(round = 1; round <= 2; round++)
    {
        // Repeat for all comma separated parts.
        modep = p_guicursor;

        if(*p_guicursor == NUL)
        {
            modep = (uchar_kt *)"a:block-blinkon0";
        }

        while(*modep != NUL)
        {
            colonp = ustrchr(modep, ':');

            if(colonp == NULL)
            {
                return (uchar_kt *)N_("E545: Missing colon");
            }

            if(colonp == modep)
            {
                return (uchar_kt *)N_("E546: Illegal mode");
            }

            commap = ustrchr(modep, ',');

            // Repeat for all modes before the colon.
            // For the 'a' mode, we loop to handle all the modes.
            all_idx = -1;
            assert(modep < colonp);

            while(modep < colonp || all_idx >= 0)
            {
                if(all_idx < 0)
                {
                    // Find the mode
                    if(modep[1] == '-' || modep[1] == ':')
                    {
                        len = 1;
                    }
                    else
                    {
                        len = 2;
                    }

                    if(len == 1 && TOLOWER_ASC(modep[0]) == 'a')
                    {
                        all_idx = kCsrShpIdxAllIndexCount - 1;
                    }
                    else
                    {
                        for(idx = 0; idx < kCsrShpIdxAllIndexCount; ++idx)
                        {
                            if(ustrnicmp(modep, shape_table[idx].name, len) == 0)
                            {
                                break;
                            }
                        }

                        if(idx == kCsrShpIdxAllIndexCount
                           || (shape_table[idx].used_for & what) == 0)
                        {
                            return (uchar_kt *)N_("E546: Illegal mode");
                        }

                        if(len == 2 && modep[0] == 'v' && modep[1] == 'e')
                        {
                            found_ve = true;
                        }
                    }

                    modep += len + 1;
                }

                if(all_idx >= 0)
                {
                    idx = all_idx--;
                }
                else if(round == 2)
                {
                    {
                        // Set the defaults, for the missing parts
                        shape_table[idx].shape = kCsrShpBlock;
                        shape_table[idx].blinkwait = 0L;
                        shape_table[idx].blinkon = 0L;
                        shape_table[idx].blinkoff = 0L;
                    }
                }

                // Parse the part after the colon
                for(p = colonp + 1; *p && *p != ',';)
                {
                    //if(what != SHAPE_MOUSE)
                    {
                        // First handle the ones with a number argument.
                        i = *p;
                        len = 0;

                        if(ustrnicmp(p, "ver", 3) == 0)
                        {
                            len = 3;
                        }
                        else if(ustrnicmp(p, "hor", 3) == 0)
                        {
                            len = 3;
                        }
                        else if(ustrnicmp(p, "blinkwait", 9) == 0)
                        {
                            len = 9;
                        }
                        else if(ustrnicmp(p, "blinkon", 7) == 0)
                        {
                            len = 7;
                        }
                        else if(ustrnicmp(p, "blinkoff", 8) == 0)
                        {
                            len = 8;
                        }

                        if(len != 0)
                        {
                            p += len;

                            if(!ascii_isdigit(*p))
                            {
                                return (uchar_kt *)N_("E548: digit expected");
                            }

                            int n = getdigits_int(&p);

                            // "ver" or "hor"
                            if(len == 3)
                            {
                                if(n == 0)
                                {
                                    return (uchar_kt *)N_("E549: Illegal percentage");
                                }

                                if(round == 2)
                                {
                                    if(TOLOWER_ASC(i) == 'v')
                                    {
                                        shape_table[idx].shape = kCsrShpVertical;
                                    }
                                    else
                                    {
                                        shape_table[idx].shape = kCsrShpHorizontal;
                                    }

                                    shape_table[idx].percentage = n;
                                }
                            }
                            else if(round == 2)
                            {
                                if(len == 9)
                                {
                                    shape_table[idx].blinkwait = n;
                                }
                                else if(len == 7)
                                {
                                    shape_table[idx].blinkon = n;
                                }
                                else
                                {
                                    shape_table[idx].blinkoff = n;
                                }
                            }
                        }
                        else if(ustrnicmp(p, "block", 5) == 0)
                        {
                            if(round == 2)
                            {
                                shape_table[idx].shape = kCsrShpBlock;
                            }

                            p += 5;
                        }
                        else
                        {
                            // must be a highlight group name then
                            endp = ustrchr(p, '-');

                            if(commap == NULL)
                            {
                                // last part
                                if(endp == NULL)
                                {
                                    endp = p + ustrlen(p); // find end of part
                                }
                            }
                            else if(endp > commap || endp == NULL)
                            {
                                endp = commap;
                            }

                            slashp = ustrchr(p, '/');

                            if(slashp != NULL && slashp < endp)
                            {
                                // "group/langmap_group"
                                i = syn_check_group(p, (int)(slashp - p));
                                p = slashp + 1;
                            }

                            if(round == 2)
                            {
                                shape_table[idx].id =
                                    syn_check_group(p, (int)(endp - p));

                                shape_table[idx].id_lm = shape_table[idx].id;

                                if(slashp != NULL && slashp < endp)
                                {
                                    shape_table[idx].id = i;
                                }
                            }

                            p = endp;
                        }
                    }

                    if(*p == '-')
                    {
                        ++p;
                    }
                }
            }

            modep = p;

            if(*modep == ',')
            {
                ++modep;
            }
        }
    }

    // If the 's' flag is not given, use the 'v' cursor for 's'
    if(!found_ve)
    {
        shape_table[kCsrShpIdxVisualExclus].shape = shape_table[kCsrShpIdxVisual].shape;
        shape_table[kCsrShpIdxVisualExclus].percentage = shape_table[kCsrShpIdxVisual].percentage;
        shape_table[kCsrShpIdxVisualExclus].blinkwait = shape_table[kCsrShpIdxVisual].blinkwait;
        shape_table[kCsrShpIdxVisualExclus].blinkon = shape_table[kCsrShpIdxVisual].blinkon;
        shape_table[kCsrShpIdxVisualExclus].blinkoff = shape_table[kCsrShpIdxVisual].blinkoff;
        shape_table[kCsrShpIdxVisualExclus].id = shape_table[kCsrShpIdxVisual].id;
        shape_table[kCsrShpIdxVisualExclus].id_lm = shape_table[kCsrShpIdxVisual].id_lm;
    }

    ui_mode_info_set();
    return NULL;
}


/// Map cursor mode from string to integer
///
/// @param mode Fullname of the mode whose id we are looking for
/// @return -1 in case of failure, else the matching SHAPE_ID* integer
int cursor_mode_str2int(const char *mode)
{
    for(int current_mode = 0; current_mode < kCsrShpIdxAllIndexCount; current_mode++)
    {
        if(strcmp(shape_table[current_mode].full_name, mode) == 0)
        {
            return current_mode;
        }
    }

    ERROR_LOG("Unknown mode %s", mode);

    return -1;
}


/// Return the index into shape_table[] for the current mode.
int cursor_get_mode_idx(void)
{
    if(curmod == kInsertShowMatchMode)
    {
        return kCsrShpIdxShowMatchParen;
    }
    else if(curmod & kModFlgVReplace)
    {
        return kCsrShpIdxReplace;
    }
    else if(curmod & kModFlgReplace)
    {
        return kCsrShpIdxReplace;
    }
    else if(curmod & kInsertMode)
    {
        return kCsrShpIdxInsert;
    }
    else if(curmod & kCmdLineMode)
    {
        if(cmdline_at_end())
        {
            return kCsrShpIdxCmdNormal;
        }
        else if(cmdline_overstrike())
        {
            return kCsrShpIdxCmdReplace;
        }
        else
        {
            return kCsrShpIdxCmdInsert;
        }
    }
    else if(finish_op)
    {
        return kCsrShpIdxOperatorPend;
    }
    else if(VIsual_active)
    {
        if(*p_sel == 'e')
        {
            return kCsrShpIdxVisualExclus;
        }
        else
        {
            return kCsrShpIdxVisual;
        }
    }
    else
    {
        return kCsrShpIdxNormal;
    }
}
