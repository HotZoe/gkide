/// @headerfile ""

#ifndef SNAIL_APP_ATTRIBUTES_H
#define SNAIL_APP_ATTRIBUTES_H

// For all gnulikes: gcc, clang, intel
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#ifdef __GNUG__
    #define FUNC_ATTR_PURE               __attribute__((pure))
    #define FUNC_ATTR_CONST              __attribute__((const))
    #define FUNC_ATTR_MALLOC             __attribute__((malloc))
    #define FUNC_ATTR_UNUSED             __attribute__((unused))
    #define FUNC_ATTR_NONNULL_ALL        __attribute__((nonnull))
    #define FUNC_ATTR_NORETURN           __attribute__((noreturn))
    #define FUNC_ATTR_ALWAYS_INLINE      __attribute__((always_inline))
    #define FUNC_ATTR_ALLOC_ALIGN(x)     __attribute__((alloc_align(x)))
    #define FUNC_ATTR_NONNULL_RET        __attribute__((returns_nonnull))
    #define FUNC_ATTR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
    #define FUNC_ATTR_NONNULL_ARGS(...)  __attribute__((nonnull(__VA_ARGS__)))
    #define VARS_ATTR_UNUSED             __attribute__((unused))
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

    #define VARS_ATTR_UNUSED
#endif // __GNUG__

#endif // SNAIL_APP_ATTRIBUTES_H
