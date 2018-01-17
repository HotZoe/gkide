/// @file plugins/bin/snail/attributes.h

#ifndef PLUGIN_SNAIL_ATTRIBUTES_H
#define PLUGIN_SNAIL_ATTRIBUTES_H

// For all gnulikes: gcc, clang, intel
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#ifdef __GNUG__
    #define FUNC_ATTR_PURE                   __attribute__((__pure__))
    #define FUNC_ATTR_CONST                  __attribute__((__const__))
    #define FUNC_ATTR_MALLOC                 __attribute__((__malloc__))
    #define FUNC_ATTR_UNUSED                 __attribute__((__unused__))
    #define FUNC_ATTR_NONNULL_ALL            __attribute__((__nonnull__))
    #define FUNC_ATTR_NORETURN               __attribute__((__noreturn__))
    #define FUNC_ATTR_ALWAYS_INLINE          __attribute__((__always_inline__))
    #define FUNC_ATTR_ALLOC_ALIGN(x)         __attribute__((__alloc_align__(x)))
    #define FUNC_ATTR_NONNULL_RET            __attribute__((__returns_nonnull__))
    #define FUNC_ATTR_WARN_UNUSED_RESULT     __attribute__((__warn_unused_result__))
    #define FUNC_ATTR_NONNULL_ARGS(...)      __attribute__((__nonnull__(__VA_ARGS__)))

    #define FUNC_ATTR_ARGS_UNUSED_MAYBE(v)   v __attribute__((__unused__))
    #define FUNC_ATTR_ARGS_UNUSED_REALY(v)   UNUSED_##v  __attribute__((__unused__))
#else
    #define FUNC_ATTR_PURE
    #define FUNC_ATTR_CONST
    #define FUNC_ATTR_MALLOC
    #define FUNC_ATTR_UNUSED
    #define FUNC_ATTR_NONNULL_ALL
    #define FUNC_ATTR_NORETURN
    #define FUNC_ATTR_ALWAYS_INLINE
    #define FUNC_ATTR_ALLOC_ALIGN(x)
    #define FUNC_ATTR_NONNULL_RET
    #define FUNC_ATTR_WARN_UNUSED_RESULT
    #define FUNC_ATTR_NONNULL_ARGS(...)

    #define FUNC_ATTR_ARGS_UNUSED_MAYBE(v)   v
    #define FUNC_ATTR_ARGS_UNUSED_REALY(v)   UNUSED_##v
#endif

#endif // PLUGIN_SNAIL_ATTRIBUTES_H
