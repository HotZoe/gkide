/// @file nvim/ascii.h

#ifndef NVIM_ASCII_H
#define NVIM_ASCII_H

#include <stdbool.h>

#include "nvim/func_attr.h"
#include "nvim/os/os_defs.h"

// Definitions of various common control characters.
// see:
// http://www.ascii-code.com/
// https://en.wikipedia.org/wiki/ASCII

#define NUL             '\000'  ///< ^@  \\0  Null
#define BELL            '\007'  ///< ^G  \\a  Bell
#define BS              '\010'  ///< ^H  \\b  Backspace
#define TAB             '\011'  ///< ^I  \\t  Horizontal Tab

#define NL              '\012'  ///< ^J  \\n  Line Feed
#define NL_STR          "\012"

#define FF              '\014'  ///< ^N       Shift Out
#define CAR             '\015'  ///< ^O       Shift In, CR is used by Mac OS X

#define ESC             '\033'  ///< ^[  \\e  Escape
#define ESC_STR         "\033"

#define DEL              0x7F   ///< ^?	      Delete
#define DEL_STR         "\177"

#define CSI              0x9B   ///< Control Sequence Introducer
#define CSI_STR         "\233"

#define DCS              0x90   ///< Device Control String
#define STERM            0x9c   ///< String Terminator

#define POUND            0xA3   ///<

#define CharOrd(x)      ((uint8_t)(x) < 'a' ? (uint8_t)(x) - 'A' : (uint8_t)(x) - 'a')
#define CharOrdLow(x)   ((uint8_t)(x) - 'a')
#define CharOrdUp(x)    ((uint8_t)(x) - 'A')
#define ROT13(c, a)     (((((c) - (a)) + 13) % 26) + (a))

#define Ctrl_chr(x)     (TOUPPER_ASC(x) ^ 0x40) ///< '?' -> DEL, '@' -> ^@, etc.
#define Meta(x)         ((x) | 0x80)

#define Ctrl_AT         0   ///< Ctrl + @ = @ - 64
#define Ctrl_A          1   ///< Ctrl + A = A - 64
#define Ctrl_B          2   ///< Ctrl + B = B - 64
#define Ctrl_C          3   ///< Ctrl + C = C - 64
#define Ctrl_D          4   ///< Ctrl + D = D - 64
#define Ctrl_E          5   ///< Ctrl + E = E - 64
#define Ctrl_F          6   ///< Ctrl + F = F - 64
#define Ctrl_G          7   ///< Ctrl + G = G - 64
#define Ctrl_H          8   ///< Ctrl + H = H - 64
#define Ctrl_I          9   ///< Ctrl + I = I - 64
#define Ctrl_J          10  ///< Ctrl + J = J - 64
#define Ctrl_K          11  ///< Ctrl + K = K - 64
#define Ctrl_L          12  ///< Ctrl + L = L - 64
#define Ctrl_M          13  ///< Ctrl + M = M - 64
#define Ctrl_N          14  ///< Ctrl + N = N - 64
#define Ctrl_O          15  ///< Ctrl + O = O - 64
#define Ctrl_P          16  ///< Ctrl + P = P - 64
#define Ctrl_Q          17  ///< Ctrl + Q = Q - 64
#define Ctrl_R          18  ///< Ctrl + R = R - 64
#define Ctrl_S          19  ///< Ctrl + S = S - 64
#define Ctrl_T          20  ///< Ctrl + T = T - 64
#define Ctrl_U          21  ///< Ctrl + U = U - 64
#define Ctrl_V          22  ///< Ctrl + V = V - 64
#define Ctrl_W          23  ///< Ctrl + W = W - 64
#define Ctrl_X          24  ///< Ctrl + X = X - 64
#define Ctrl_Y          25  ///< Ctrl + Y = Y - 64
#define Ctrl_Z          26  ///< Ctrl + Z = Z - 64
//                      27  ///< Ctrl + [ = [ - 64 is equal ESC
#define Ctrl_BSL        28  ///< ctrl + backslash
#define Ctrl_RSB        29  ///< ctrl + ] = ] - 64
#define Ctrl_HAT        30  ///< ctrl + ^ = ^ - 64
#define Ctrl__          31  ///< ctrl + _

#define CTRL_F_STR      "\006"
#define CTRL_H_STR      "\010"
#define CTRL_V_STR      "\026"

// Character that separates dir names in a path.
#ifdef BACKSLASH_IN_FILENAME
    #define PATHSEP        psepc
    #define PATHSEPSTR     pseps
#else
    #define PATHSEP        '/'
    #define PATHSEPSTR     "/"
#endif

static inline bool ascii_iswhite(int)
REAL_FATTR_CONST
REAL_FATTR_ALWAYS_INLINE;

static inline bool ascii_isdigit(int)
REAL_FATTR_CONST
REAL_FATTR_ALWAYS_INLINE;

static inline bool ascii_isspace(int)
REAL_FATTR_CONST
REAL_FATTR_ALWAYS_INLINE;

static inline bool ascii_isxdigit(int)
REAL_FATTR_CONST
REAL_FATTR_ALWAYS_INLINE;

static inline bool ascii_isbdigit(int)
REAL_FATTR_CONST
REAL_FATTR_ALWAYS_INLINE;

/// Checks if @b c is a space or tab character.
///
/// @see ascii_isdigit()
static inline bool ascii_iswhite(int c)
{
    return c == ' ' || c == '\t';
}

/// Check whether character is a decimal digit.
///
/// Library isdigit() function is officially locale-dependent and, for
/// example, returns true for superscript 1 (¹) in locales where encoding
/// contains it in lower 8 bits. Also avoids crashes in case c is below
/// 0 or above 255: library functions are officially defined as accepting
/// only EOF and unsigned char values (otherwise it is undefined behaviour)
/// what may be used for some optimizations
/// (e.g. simple `return isdigit_table[c];`).
static inline bool ascii_isdigit(int c)
{
    return c >= '0' && c <= '9';
}

/// Checks if `c` is a hexadecimal digit,
/// that is, one of 0-9, a-f, A-F.
///
/// @see {ascii_isdigit}
static inline bool ascii_isxdigit(int c)
{
    return (c >= '0' && c <= '9')
            || (c >= 'a' && c <= 'f')
            || (c >= 'A' && c <= 'F');
}

/// Checks if `c` is a binary digit, that is, 0-1.
///
/// @see {ascii_isdigit}
static inline bool ascii_isbdigit(int c)
{
    return (c == '0' || c == '1');
}

/// Checks if `c` is a white-space character,
/// that is, one of \f, \n, \r, \t, \v.
///
/// @see {ascii_isdigit}
static inline bool ascii_isspace(int c)
{
    return (c >= 9 && c <= 13) || c == ' ';
}

#endif // NVIM_ASCII_H
