/// @file nvim/macros.h

#ifndef NVIM_MACROS_H
#define NVIM_MACROS_H

#include "generated/config/config.h"

// EXTERN is only defined in main.c.
// That's where global variables are actually defined and initialized.
#ifndef EXTERN
    #define EXTERN extern
    #define INIT(...)
#else
    #ifndef INIT
        #define INIT(...) __VA_ARGS__
    #endif
#endif

#ifndef MIN
    #define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef MAX
    #define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

/// String with length
///
/// For use in functions which accept (char *s, size_t len) pair in arguments.
///
/// @param[in]  s  Static string.
///
/// @return
/// - s, sizeof(s) - 1
#define S_LEN(s)     (s), (sizeof(s) - 1)

/// return TRUE if the line is empty
#define lineempty(p) (*ml_get(p) == NUL)

/// return TRUE if the current buffer is empty
#define bufempty()   (curbuf->b_ml.ml_line_count == 1 \
                      && *ml_get((linenum_kt)1) == NUL)

/// toupper() and tolower() that use the current locale.
///
/// @warning
/// Only call TOUPPER_LOC() and TOLOWER_LOC() with a character in
/// the range 0 - 255. toupper()/tolower() on some systems can't handle others.
///
/// @note
/// It is often better to use mb_tolower() and mb_toupper(), because many
/// toupper() and tolower() implementations only work for ASCII.
#define TOUPPER_LOC      toupper
#define TOLOWER_LOC      tolower

// toupper() and tolower() for ASCII only and ignore the current locale.
#define TOUPPER_ASC(c)   (((c) < 'a' || (c) > 'z') ? (c) : (c) - ('a' - 'A'))
#define TOLOWER_ASC(c)   (((c) < 'A' || (c) > 'Z') ? (c) : (c) + ('a' - 'A'))

// Like isalpha() but reject non-ASCII characters.
// Can't be used with a special key (negative value).
#define ASCII_ISLOWER(c)   ((unsigned)(c) >= 'a' && (unsigned)(c) <= 'z')
#define ASCII_ISUPPER(c)   ((unsigned)(c) >= 'A' && (unsigned)(c) <= 'Z')
#define ASCII_ISALPHA(c)   (ASCII_ISUPPER(c) || ASCII_ISLOWER(c))
#define ASCII_ISALNUM(c)   (ASCII_ISALPHA(c) || ascii_isdigit(c))

/// Returns empty string if it is NULL.
#define EMPTY_IF_NULL(x)   ((x) ? (x) : (uchar_kt *)"")

/// Adjust chars in a language according to 'langmap' option.
/// NOTE that there is no noticeable overhead if 'langmap' is not set.
/// When set the overhead for characters < 256 is small.
/// Don't apply 'langmap' if the character comes from the Stuff buffer
/// or from a mapping and the langnoremap option was set.
/// The do-while is just to ignore a ';' after the macro.
#define LANGMAP_ADJUST(c, condition)      \
    do                                    \
    {                                     \
        if(*p_langmap                     \
           && (condition)                 \
           && (p_lrm || KeyTyped)         \
           && !KeyStuffed                 \
           && (c) >= 0)                   \
        {                                 \
            if((c) < 256)                 \
            {                             \
                c = langmap_mapchar[c];   \
            }                             \
            else                          \
            {                             \
                c = langmap_adjust_mb(c); \
            }                             \
        }                                 \
    } while(0)

/// vim_isbreak() is used very often if 'linebreak' is set,
/// use a macro to make it work fast.
#define vim_isbreak(c)    (breakat_flags[(uchar_kt)(c)])

// no CR-LF translation
#define WRITEBIN    "wb"
#define READBIN     "rb"
#define APPENDBIN   "ab"

// mch_open_rw(): invoke os_open() with third argument for user R/W.
// open in rw------- mode
#if defined(UNIX)
    #define mch_open_rw(n, f)      os_open((n), (f), (mode_t)0600)
#elif defined(HOST_OS_WINDOWS)
    #define mch_open_rw(n, f)      os_open((n), (f), S_IREAD | S_IWRITE)
#else
    #define mch_open_rw(n, f)      os_open((n), (f), 0)
#endif

#define mch_fopen(n, p)      fopen((n), (p))
#define REPLACE_NORMAL(s)    (((s) & kModFlgReplace) && !((s) & kModFlgVReplace))

#define UTF_COMPOSINGLIKE(p1, p2)  utf_composinglike((p1), (p2))

/// Whether to draw the vertical bar on the right side of the cell.
#define CURSOR_BAR_RIGHT  \
    (curwin->w_o_curbuf.wo_rl && (!(curmod & kCmdLineMode) || cmdmsg_rl))

#define RESET_BINDING(wp)  \
    (wp)->w_o_curbuf.wo_scb = FALSE; (wp)->w_o_curbuf.wo_crb = FALSE

/// Calculate the length of a C array.
///
/// This should be called with a real array. Calling this with a pointer
/// is an error. A mechanism to detect many (though not all) of those errors
/// at compile time is implemented. It works by the second division producing
/// a division by zero in those cases (-Wdiv-by-zero in GCC).
#define ARRAY_SIZE(arr) \
    ((sizeof(arr)/sizeof((arr)[0])) / ((size_t)(!(sizeof(arr) % sizeof((arr)[0])))))

#ifdef RGB
    // avoid RGB redefined warnings when build under
    // windows using Msys2 "wingdi.h" also defined RGB macro
    #undef RGB
#endif

#define RGB(r, g, b)   ((r << 16) | (g << 8) | b)
#define STR_(x)        #x
#define STR(x)         STR_(x)

// __has_attribute
//
// This function-like macro takes a single identifier argument that is
// the name of a GNU-style attribute. It evaluates to 1  if the attribute
// is supported by the current compilation target, or 0 if not.
//
// for details, see: http://clang.llvm.org/docs/LanguageExtensions.html

#ifndef __has_attribute
    // Compatibility with non-clang compilers.
    #define NVIM_HAS_ATTRIBUTE(x) 0
#elif defined(__clang__) \
    && __clang__ == 1    \
    && (__clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ <= 5))
    // Starting in Clang 3.6, __has_attribute was fixed to only report true for
    // GNU-style attributes.  Prior to that, it reported true if _any_ backend
    // supported the attribute.
    #define NVIM_HAS_ATTRIBUTE(x) 0
#else
    #define NVIM_HAS_ATTRIBUTE __has_attribute
#endif

#if NVIM_HAS_ATTRIBUTE(fallthrough)
    // details see:
    // https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
    #define FALL_THROUGH_ATTRIBUTE    __attribute__((fallthrough))
#else
    #define FALL_THROUGH_ATTRIBUTE
#endif

/// Change type of structure pointers: cast `struct a *` to `struct b *`
///
/// Used to silence PVS errors.
///
/// @param  Type  Structure to cast to.
/// @param  obj   Object to cast.
///
/// @return ((Type *)obj).
#define STRUCT_CAST(Type, obj)   ((Type *)(obj))

#define JUST_SHOW_IT(x)  #x
#define TO_STRING(x)     JUST_SHOW_IT(x)

#endif // NVIM_MACROS_H
