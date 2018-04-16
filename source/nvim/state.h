/// @file nvim/state.h

#ifndef NVIM_STATE_H
#define NVIM_STATE_H

#include <stddef.h>

/// nvim main loop status code
///
/// @note use by callback function of type
/// - @b nvim_state_check_ft, for nvim state check
/// - @b nvim_state_execute_ft, for nvim cmd execute
typedef enum loop_status_e
{
    kNLSC_ExitNvim = 0, ///< nvim exit, main loop return
    kNLSC_Continue = 1, ///< state continue, loop go on
    kNLSC_NextLoop = 2, ///< new loop iteration, skip the following part
    kNLSC_GetKey   = 3, ///< get another key
} loop_status_et;

typedef struct nvim_state_s nvim_state_st;

typedef int (*nvim_state_check_ft)(nvim_state_st *state);
typedef int (*nvim_state_execute_ft)(nvim_state_st *state, int key);

struct nvim_state_s
{
    nvim_state_check_ft check;
    nvim_state_execute_ft execute;
};

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "state.h.generated.h"
#endif

#endif // NVIM_STATE_H
