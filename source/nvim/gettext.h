/// @file nvim/gettext.h

#ifndef NVIM_GETTEXT_H
#define NVIM_GETTEXT_H

#include "generated/config/confignvim.h"

#ifdef FOUND_WORKING_LIBINTL
    #include <libintl.h>

    #define _(x)    gettext((char *)(x))

    // no-operation macro
    #ifdef gettext_noop
        #define N_(x)    gettext_noop(x)
    #else
        #define N_(x)    x
    #endif
#else
    #define _(x)                           ((char *)(x))
    #define N_(x)                          x
    #define bindtextdomain(x, y)           /* empty */
    #define bind_textdomain_codeset(x, y)  /* empty */
    #define textdomain(x)                  /* empty */
#endif

#endif // NVIM_GETTEXT_H
