/// @file nvim/state.h

#ifndef NVIM_STATE_H
#define NVIM_STATE_H

#include <stddef.h>

typedef struct nvim_state_s nvim_state_st;

typedef int (*nvim_state_check_ft)(nvim_state_st *state);
typedef int (*nvim_state_execute_ft)(nvim_state_st *state, int key);

struct nvim_state_s
{
    nvim_state_check_ft check;
    nvim_state_execute_ft execute;
};

/// nvim state check code
typedef enum state_check_code_e
{
    kNSCC_ExitNvim = 0, ///< nvim exit, loop stop
    kNSCC_Continue = 1, ///< state continue, loop go on
    kNSCC_LoopNext = 2, ///< next state check, loop next iteration
} state_check_code_et;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "state.h.generated.h"
#endif

#endif // NVIM_STATE_H
