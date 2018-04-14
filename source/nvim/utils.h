/// @file nvim/utils.h

#ifndef NVIM_UTILS_H
#define NVIM_UTILS_H

#include <time.h>
#include <stdint.h>

/// convert flags for str_to_num()
typedef enum str_to_num_e
{
    kStrToNumBin  = 1, ///< convert string to binary
    kStrToNumOct  = 2, ///< convert string to octonary
    kStrToNumHex  = 4, ///< convert string to hexadecimal
    kStrToNumAll  = (kStrToNumBin + kStrToNumOct + kStrToNumHex),
    kStrToNumOne  = 8, ///< only when ONE of the above is used

} str_to_num_et;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "utils.h.generated.h"
#endif

#endif // NVIM_SPELL_H
