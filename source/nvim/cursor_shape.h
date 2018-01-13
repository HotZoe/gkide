/// @file nvim/cursor_shape.h

#ifndef NVIM_CURSOR_SHAPE_H
#define NVIM_CURSOR_SHAPE_H

#include "nvim/types.h"
#include "nvim/api/private/defs.h"

/// struct to store values from 'guicursor'
/// and 'mouseshape' Indexes in shape_table[]
typedef enum mode_shape_e
{
    kCsrShpIdxNormal        = 0,   ///< Normal mode
    kCsrShpIdxVisual        = 1,   ///< Visual mode
    kCsrShpIdxInsert        = 2,   ///< Insert mode
    kCsrShpIdxReplace       = 3,   ///< Replace mode
    kCsrShpIdxCmdNormal     = 4,   ///< Command line Normal mode
    kCsrShpIdxCmdInsert     = 5,   ///< Command line Insert mode
    kCsrShpIdxCmdReplace    = 6,   ///< Command line Replace mode
    kCsrShpIdxOperatorPend  = 7,   ///< Operator-pending mode
    kCsrShpIdxVisualExclus  = 8,   ///< Visual mode with 'selection' exclusive
    kCsrShpIdxOnCmdL        = 9,   ///< On command line
    kCsrShpIdxOnStatusL     = 10,  ///< On status line
    kCsrShpIdxDragStatusL   = 11,  ///< dragging a status line
    kCsrShpIdxOnVertSepL    = 12,  ///< On vertical separator line
    kCsrShpIdxDragVertSepL  = 13,  ///< dragging a vertical separator line
    kCsrShpIdxHitReturnMore = 14,  ///< Hit-return or More
    kCsrShpIdxHitReturnMoreL= 15,  ///< Hit-return or More in last line
    kCsrShpIdxShowMatchParen= 16,  ///< showing matching paren
    kCsrShpIdxAllIndexCount = 17   ///< all index count, must be last one
} mode_shape_et;

typedef enum
{
    kCsrShpBlock      = 0, ///< block cursor
    kCsrShpHorizontal = 1, ///< horizontal bar cursor
    kCsrShpVertical   = 2, ///< vertical bar cursor
} cursor_shape_et;

#define MSHAPE_NUMBERED 1000   ///< offset for shapes identified by number
#define MSHAPE_HIDE     1      ///< hide mouse pointer

#define SHAPE_MOUSE     1      ///< used for mouse pointer shape
#define SHAPE_CURSOR    2      ///< used for text cursor shape

typedef struct cursor_info_s
{
    char *full_name;        ///< mode description
    cursor_shape_et shape;  ///< cursor shape: one of the SHAPE_ defines
    int mshape;             ///< mouse shape: one of the MSHAPE defines
    int percentage;         ///< percentage of cell for bar
    long blinkwait;         ///< blinking, wait time before blinking starts
    long blinkon;           ///< blinking, on time
    long blinkoff;          ///< blinking, off time
    int id;                 ///< highlight group ID
    int id_lm;              ///< highlight group ID for :lmap mode
    char *name;             ///< mode short name
    char used_for;          ///< SHAPE_MOUSE and/or SHAPE_CURSOR
} cursor_info_st;

extern cursor_info_st shape_table[kCsrShpIdxAllIndexCount];

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "cursor_shape.h.generated.h"
#endif

#endif // NVIM_CURSOR_SHAPE_H
