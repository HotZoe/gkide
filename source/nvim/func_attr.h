/// @file nvim/func_attr.h
///
/// - Keep <b>FUNC_ATTR_*</b> macros untouched, are used by the C preprocessor
///   to generate the *.i files, which are used by
///   <b>source/nvim/generators/gen_header.lua</b>:
///   - undefined @b DEFINE_FUNC_ATTRIBUTES
///   -   defined @b DEFINE_KEEP_ATTRIBUTES
///
/// - Macros defined as `__attribute__((*))` are used by generated header files:
///   - defined @b DEFINE_FUNC_ATTRIBUTES
///   - defined @b DEFINE_KEEP_ATTRIBUTES or undefined @b DEFINE_KEEP_ATTRIBUTES
///
///
/// - Empty macros values are used for *.c files, which is:
///   - undefined @b DEFINE_FUNC_ATTRIBUTES
///   - undefined @b DEFINE_KEEP_ATTRIBUTES
///
/// - <b>FUNC_ATTR_*</b> macros should be in *.c files,
///   to auto generated declarations.
/// - <b>REAL_FATTR_*</b> macros should be in *.c files,
///   not auto generated declarations.
///

// gcc and clang expose their version as follows:
//
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
// gcc 4.7.2:
//   __GNUC__          = 4
//   __GNUC_MINOR__    = 7
//   __GNUC_PATCHLEVEL = 2
//
// clang 3.4 (claims compat with gcc 4.2.1):
//   __GNUC__          = 4
//   __GNUC_MINOR__    = 2
//   __GNUC_PATCHLEVEL = 1
//   __clang__         = 1
//   __clang_major__   = 3
//   __clang_minor__   = 4
//
// To view the default defines of these compilers, you can perform:
//
// $ gcc -E -dM - </dev/null
// $ echo | clang -dM -E -

#include "nvim/macros.h"

// clean up FUNC_ATTR_* anyway
#ifdef FUNC_ATTR_MALLOC
    #undef FUNC_ATTR_MALLOC
#endif

#ifdef FUNC_ATTR_ALLOC_SIZE
    #undef FUNC_ATTR_ALLOC_SIZE
#endif

#ifdef FUNC_ATTR_ALLOC_SIZE_PROD
    #undef FUNC_ATTR_ALLOC_SIZE_PROD
#endif

#ifdef FUNC_ATTR_ALLOC_ALIGN
    #undef FUNC_ATTR_ALLOC_ALIGN
#endif

#ifdef FUNC_ATTR_PURE
    #undef FUNC_ATTR_PURE
#endif

#ifdef FUNC_ATTR_CONST
    #undef FUNC_ATTR_CONST
#endif

#ifdef FUNC_ATTR_WARN_UNUSED_RESULT
    #undef FUNC_ATTR_WARN_UNUSED_RESULT
#endif

#ifdef FUNC_ATTR_ALWAYS_INLINE
    #undef FUNC_ATTR_ALWAYS_INLINE
#endif

#ifdef FUNC_ATTR_UNUSED
    #undef FUNC_ATTR_UNUSED
#endif

#ifdef FUNC_ATTR_NONNULL_ALL
    #undef FUNC_ATTR_NONNULL_ALL
#endif

#ifdef FUNC_ATTR_NONNULL_ARG
    #undef FUNC_ATTR_NONNULL_ARG
#endif

#ifdef FUNC_ATTR_NONNULL_RET
    #undef FUNC_ATTR_NONNULL_RET
#endif

#ifdef FUNC_ATTR_NORETURN
    #undef FUNC_ATTR_NORETURN
#endif

#ifndef DEFINE_REAL_FUNC_ATTRIBUTES
    #define DEFINE_REAL_FUNC_ATTRIBUTES
    #ifdef __GNUC__
        // For all gnulikes: gcc, clang, intel.
        // place these after the argument list of the function declaration,
        // not definition, like: void myfunc(void) REAL_FATTR_ALWAYS_INLINE;
        #define REAL_FATTR_MALLOC                __attribute__((__malloc__))
        #define REAL_FATTR_ALLOC_ALIGN(x)        __attribute__((__alloc_align__(x)))
        #define REAL_FATTR_PURE                  __attribute__((__pure__))
        #define REAL_FATTR_CONST                 __attribute__((__const__))
        #define REAL_FATTR_WARN_UNUSED_RESULT    __attribute__((__warn_unused_result__))
        #define REAL_FATTR_ALWAYS_INLINE         __attribute__((__always_inline__))
        #define REAL_FATTR_UNUSED                __attribute__((__unused__))
        #define REAL_FATTR_NONNULL_ALL           __attribute__((__nonnull__))
        #define REAL_FATTR_NONNULL_ARG(...)      __attribute__((__nonnull__(__VA_ARGS__)))
        #define REAL_FATTR_NORETURN              __attribute__((__noreturn__))

        #if NVIM_HAS_ATTRIBUTE(returns_nonnull)
        #define REAL_FATTR_NONNULL_RET           __attribute__((__returns_nonnull__))
        #endif

        #if NVIM_HAS_ATTRIBUTE(alloc_size)
        #define REAL_FATTR_ALLOC_SIZE(x)         __attribute__((__alloc_size__(x)))
        #define REAL_FATTR_ALLOC_SIZE_PROD(x, y) __attribute__((__alloc_size__(x, y)))
        #endif

        /// the arguments not always used,
        /// e.g: used for windows build, but not for linux
        #define REAL_ARGS_ATTR_UNUSED_MAYBE(v)   v __attribute__((__unused__))

        /// the arguments always not used, and you get an error
        /// if you try use @b v in the code anywhere.
        ///
        /// @note
        /// it does not work for arguments which contain parenthesis, e.g.:
        ///
        /// ~~~~~~~~~~~~~~~{.c}
        /// int func(char (*names)[3]);
        ///
        /// // do not work
        /// int func(char (*REAL_FATTR_ARG_UNUSED_REALY(names))[3]);
        /// int func(char (*UNUSED_names __attribute__((__unused__)))[3]);
        /// ~~~~~~~~~~~~~~~
        ///
        /// so, those type arguments, use `(void)names;` in the function body
        /// instead the macro is defined following, see FUNC_ARGS_UNUSED_TODO
        ///
        /// refactoring the function and related function pointer
        #define REAL_ARGS_ATTR_UNUSED_REALY(v)   UNUSED_##v  __attribute__((__unused__))
    #endif

    // The following is used to replace the macros in .c files
    // Define attributes that are not defined for this compiler.
    #ifndef REAL_FATTR_MALLOC
        #define REAL_FATTR_MALLOC
    #endif

    #ifndef REAL_FATTR_ALLOC_SIZE
        #define REAL_FATTR_ALLOC_SIZE(x)
    #endif

    #ifndef REAL_FATTR_ALLOC_SIZE_PROD
        #define REAL_FATTR_ALLOC_SIZE_PROD(x, y)
    #endif

    #ifndef REAL_FATTR_ALLOC_ALIGN
        #define REAL_FATTR_ALLOC_ALIGN(x)
    #endif

    #ifndef REAL_FATTR_PURE
        #define REAL_FATTR_PURE
    #endif

    #ifndef REAL_FATTR_CONST
        #define REAL_FATTR_CONST
    #endif

    #ifndef REAL_FATTR_WARN_UNUSED_RESULT
        #define REAL_FATTR_WARN_UNUSED_RESULT
    #endif

    #ifndef REAL_FATTR_ALWAYS_INLINE
        #define REAL_FATTR_ALWAYS_INLINE
    #endif

    #ifndef REAL_FATTR_UNUSED
        #define REAL_FATTR_UNUSED
    #endif

    #ifndef REAL_FATTR_NONNULL_ALL
        #define REAL_FATTR_NONNULL_ALL
    #endif

    #ifndef REAL_FATTR_NONNULL_ARG
        #define REAL_FATTR_NONNULL_ARG(...)
    #endif

    #ifndef REAL_FATTR_NONNULL_RET
        #define REAL_FATTR_NONNULL_RET
    #endif

    #ifndef REAL_FATTR_NORETURN
        #define REAL_FATTR_NORETURN
    #endif

    #ifndef REAL_ARGS_ATTR_UNUSED_MAYBE
        #define REAL_ARGS_ATTR_UNUSED_MAYBE(v)   v
    #endif

    #ifndef REAL_ARGS_ATTR_UNUSED_REALY
        #define REAL_ARGS_ATTR_UNUSED_REALY(v)   UNUSED_##v
    #endif

    /// the unused args must keep, such as to fit the function pointer
    #define FUNC_ARGS_UNUSED_MUST(v)     (void)(v)
    /// refactoring the function or just remove the unused args or todo
    #define FUNC_ARGS_UNUSED_TODO(v)     (void)(v)
#endif

// set up FUNC_ATTR_* anyway
#ifdef DEFINE_FUNC_ATTRIBUTES
    // used only by header files *.h
    #define FUNC_API_ASYNC
    #define FUNC_API_NOEXPORT
    #define FUNC_API_REMOTE_ONLY
    #define FUNC_API_SINCE(x)
    #define FUNC_API_DEPRECATED_SINCE(x)

    #define FUNC_ATTR_MALLOC                REAL_FATTR_MALLOC
    #define FUNC_ATTR_ALLOC_SIZE(x)         REAL_FATTR_ALLOC_SIZE(x)
    #define FUNC_ATTR_ALLOC_SIZE_PROD(x, y) REAL_FATTR_ALLOC_SIZE_PROD(x, y)
    #define FUNC_ATTR_ALLOC_ALIGN(x)        REAL_FATTR_ALLOC_ALIGN(x)
    #define FUNC_ATTR_PURE                  REAL_FATTR_PURE
    #define FUNC_ATTR_CONST                 REAL_FATTR_CONST
    #define FUNC_ATTR_WARN_UNUSED_RESULT    REAL_FATTR_WARN_UNUSED_RESULT
    #define FUNC_ATTR_ALWAYS_INLINE         REAL_FATTR_ALWAYS_INLINE
    #define FUNC_ATTR_UNUSED                REAL_FATTR_UNUSED
    #define FUNC_ATTR_NONNULL_ALL           REAL_FATTR_NONNULL_ALL
    #define FUNC_ATTR_NONNULL_ARG(...)      REAL_FATTR_NONNULL_ARG(__VA_ARGS__)
    #define FUNC_ATTR_NONNULL_RET           REAL_FATTR_NONNULL_RET
    #define FUNC_ATTR_NORETURN              REAL_FATTR_NORETURN

    // used by header files *.h and source files *.c
    /// @todo refactoring the function or just remove the unused args
    #define FUNC_ARGS_UNUSED_REALY(v)       REAL_ARGS_ATTR_UNUSED_REALY(v)
    /// for some reason the function argument maybe not used
    #define FUNC_ARGS_UNUSED_MAYBE(v)       REAL_ARGS_ATTR_UNUSED_MAYBE(v)
#elif !defined(DEFINE_KEEP_ATTRIBUTES)
    // used only by source files *.c
    #define FUNC_ATTR_MALLOC
    #define FUNC_ATTR_ALLOC_SIZE(x)
    #define FUNC_ATTR_ALLOC_SIZE_PROD(x, y)
    #define FUNC_ATTR_ALLOC_ALIGN(x)
    #define FUNC_ATTR_PURE
    #define FUNC_ATTR_CONST
    #define FUNC_ATTR_WARN_UNUSED_RESULT
    #define FUNC_ATTR_ALWAYS_INLINE
    #define FUNC_ATTR_UNUSED
    #define FUNC_ATTR_NONNULL_ALL
    #define FUNC_ATTR_NONNULL_ARG(...)
    #define FUNC_ATTR_NONNULL_RET
    #define FUNC_ATTR_NORETURN
#endif

// keep the FUNC_ATTR_* macros not touched for the generated *.i
