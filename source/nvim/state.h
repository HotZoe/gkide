/// @file nvim/state.h

#ifndef NVIM_STATE_H
#define NVIM_STATE_H

#include <stddef.h>

typedef struct nvim_state_s nvim_state_st;

typedef int(*state_check_callback_ft)(nvim_state_st *state);
typedef int(*state_execute_callback_ft)(nvim_state_st *state, int key);

struct nvim_state_s
{
    state_check_callback_ft check;
    state_execute_callback_ft execute;
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "state.h.generated.h"
#endif

#endif // NVIM_STATE_H
