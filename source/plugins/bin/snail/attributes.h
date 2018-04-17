/// @file plugins/bin/snail/attributes.h
///
/// - FATTR_*   REAL_FATTR_*    Function attributes
/// - VATTR_*   REAL_VATTR_*    Variable attributes
/// - TATTR_*   REAL_TATTR_*    Type attributes
/// - LATTR_*   REAL_LATTR_*    Label attributes
/// - EATTR_*   REAL_EATTR_*    Enumerator attributes
/// - SATTR_*   REAL_SATTR_*    Statement attributes
/// - XATTR_*   REAL_XATTR_*    All other attributes
///
/// https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
/// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
///
/// gcc and clang expose their version as follows:
///
/// gcc 4.7.2:
///   __GNUC__          = 4
///   __GNUC_MINOR__    = 7
///   __GNUC_PATCHLEVEL = 2
///
/// clang 3.4 (claims compat with gcc 4.2.1):
///   __GNUC__          = 4
///   __GNUC_MINOR__    = 2
///   __GNUC_PATCHLEVEL = 1
///   __clang__         = 1
///   __clang_major__   = 3
///   __clang_minor__   = 4
///
/// To view the default defines of these compilers, you can perform:
///
/// $ gcc -E -dM - </dev/null
/// $ echo | clang -dM -E -

#ifndef PLUGIN_SNAIL_ATTRIBUTES_H
#define PLUGIN_SNAIL_ATTRIBUTES_H

#ifdef __GNUG__
    #define FATTR_PURE                  __attribute__((__pure__))
    #define FATTR_CONST                 __attribute__((__const__))
    #define FATTR_MALLOC                __attribute__((__malloc__))
    #define FATTR_UNUSED                __attribute__((__unused__))
    #define FATTR_NONNULL_ALL           __attribute__((__nonnull__))
    #define FATTR_NORETURN              __attribute__((__noreturn__))
    #define FATTR_ALWAYS_INLINE         __attribute__((__always_inline__))
    #define FATTR_ALLOC_ALIGN(x)        __attribute__((__alloc_align__(x)))
    #define FATTR_NONNULL_RET           __attribute__((__returns_nonnull__))
    #define FATTR_WARN_UNUSED_RESULT    __attribute__((__warn_unused_result__))
    #define FATTR_NONNULL_ARGS(...)     __attribute__((__nonnull__(__VA_ARGS__)))

    #define VATTR_UNUSED_MAYBE(v)       v __attribute__((__unused__))
    #define VATTR_UNUSED_MATCH(v)       UNUSED_##v  __attribute__((__unused__))
#else
    #define FATTR_PURE
    #define FATTR_CONST
    #define FATTR_MALLOC
    #define FATTR_UNUSED
    #define FATTR_NONNULL_ALL
    #define FATTR_NORETURN
    #define FATTR_ALWAYS_INLINE
    #define FATTR_ALLOC_ALIGN(x)
    #define FATTR_NONNULL_RET
    #define FATTR_WARN_UNUSED_RESULT
    #define FATTR_NONNULL_ARGS(...)

    #define VATTR_UNUSED_MAYBE(v)       v
    #define VATTR_UNUSED_MATCH(v)       UNUSED_##v
#endif

#define FUNC_ARGS_UNUSED_MUST(v)        (void)(v)
#define FUNC_ARGS_UNUSED_TODO(v)        (void)(v)

#endif // PLUGIN_SNAIL_ATTRIBUTES_H
