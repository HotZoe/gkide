/// @file nvim/ugrid.h

#ifndef NVIM_UGRID_H
#define NVIM_UGRID_H

#include "nvim/ui.h"
#include "nvim/globals.h"

typedef struct ucell_s ucell_st;
typedef struct ugrid_s ugrid_st;

struct ucell_s
{
    char data[6 * MAX_MCO + 1];
    uihl_attr_st attrs;
};

struct ugrid_s
{
    int top;
    int bot;
    int left;
    int right;
    int row;
    int col;
    int bg;
    int fg;
    int width;
    int height;
    uihl_attr_st attrs;
    ucell_st **cells;
};

#define EMPTY_ATTRS \
    ((uihl_attr_st) { false, false, false, false, false, -1, -1, -1 })

#define UGRID_FOREACH_CELL(grid, top, bot, left, right, code) \
    do                                                        \
    {                                                         \
        for(int row = top; row <= bot; row++)                 \
        {                                                     \
            ucell_st *row_cells = (grid)->cells[row];         \
            for(int col = left; col <= right; col++)          \
            {                                                 \
                ucell_st *cell = row_cells + col;             \
                (void)(cell);                                 \
                code;                                         \
            }                                                 \
        }                                                     \
    } while(0)

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "ugrid.h.generated.h"
#endif

#endif // NVIM_UGRID_H
