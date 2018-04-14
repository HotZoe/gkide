/// @file nvim/strings.h

#ifndef NVIM_STRINGS_H
#define NVIM_STRINGS_H

#include <stdbool.h>
#include <stdarg.h>

#include "nvim/types.h"
#include "nvim/eval/typval.h"

// defines to avoid typecasts from (uchar_kt *) to (char *) and back
#define ustrlen(s)          strlen((char *)(s))
#define ustrcpy(d, s)       strcpy((char *)(d), (char *)(s))
#define ustrncpy(d, s, n)   strncpy((char *)(d), (char *)(s), (size_t)(n))
#define ustrlcpy(d, s, n)   xstrncpy((char *)(d), (char *)(s), (size_t)(n))
#define ustrcmp(d, s)       strcmp((char *)(d), (char *)(s))
#define ustrncmp(d, s, n)   strncmp((char *)(d), (char *)(s), (size_t)(n))

/// Like strcpy() but allows overlapped source and destination.
#define xstrmove(d, s)      memmove((d), (s), ustrlen(s) + 1)

#ifdef HAVE_FUN_STRCASECMP
    #define ustricmp(d, s)  strcasecmp((char *)(d), (char *)(s))
#else
    #ifdef HAVE_STRICMP
        #define ustricmp(d, s)  stricmp((char *)(d), (char *)(s))
    #else
        #define ustricmp(d, s)  xstricmp((char *)(d), (char *)(s))
    #endif
#endif

#ifdef HAVE_FUN_STRNCASECMP
    #define ustrnicmp(d, s, n) \
        strncasecmp((char *)(d), (char *)(s), (size_t)(n))
#else
    #ifdef HAVE_STRNICMP
        #define ustrnicmp(d, s, n) \
            strnicmp((char *)(d), (char *)(s), (size_t)(n))
    #else
        #define ustrnicmp(d, s, n) \
            xstrnicmp((char *)(d), (char *)(s), (size_t)(n))
    #endif
#endif

#define ustrcat(d, s)       strcat((char *)(d), (char *)(s))
#define ustrncat(d, s, n)   strncat((char *)(d), (char *)(s), (size_t)(n))
#define ustrlcat(d, s, n)   xstrncat((char *)(d), (char *)(s), (size_t)(n))

#define xstrpbrk(s, cs)     (uchar_kt *)strpbrk((char *)(s), (char *)(cs))

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "strings.h.generated.h"
#endif

#endif // NVIM_STRINGS_H
