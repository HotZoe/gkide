/// @file nvim/eval/gc.c

#include "nvim/eval/typval.h"
#include "nvim/eval/gc.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "eval/gc.c.generated.h"
#endif

dict_T *gc_first_dict = NULL; ///< Head of list of all dictionaries
list_st *gc_first_list = NULL; ///< Head of list of all lists
