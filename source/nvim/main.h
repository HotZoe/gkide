/// @file nvim/main.h

#ifndef NVIM_MAIN_H
#define NVIM_MAIN_H

#include "nvim/normal.h"
#include "nvim/event/loop.h"

extern main_loop_st main_loop;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "main.h.generated.h"
#endif

#endif // NVIM_MAIN_H
