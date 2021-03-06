/// @file nvim/strings.c

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "nvim/assert.h"
#include "nvim/nvim.h"
#include "nvim/ascii.h"
#include "nvim/strings.h"
#include "nvim/file_search.h"
#include "nvim/buffer.h"
#include "nvim/charset.h"
#include "nvim/diff.h"
#include "nvim/edit.h"
#include "nvim/eval.h"
#include "nvim/ex_cmds.h"
#include "nvim/ex_docmd.h"
#include "nvim/ex_getln.h"
#include "nvim/fileio.h"
#include "nvim/func_attr.h"
#include "nvim/fold.h"
#include "nvim/func_attr.h"
#include "nvim/getchar.h"
#include "nvim/mark.h"
#include "nvim/mbyte.h"
#include "nvim/memfile.h"
#include "nvim/memline.h"
#include "nvim/memory.h"
#include "nvim/message.h"
#include "nvim/misc1.h"
#include "nvim/move.h"
#include "nvim/option.h"
#include "nvim/ops.h"
#include "nvim/os_unix.h"
#include "nvim/path.h"
#include "nvim/quickfix.h"
#include "nvim/regexp.h"
#include "nvim/screen.h"
#include "nvim/search.h"
#include "nvim/spell.h"
#include "nvim/syntax.h"
#include "nvim/tag.h"
#include "nvim/window.h"
#include "nvim/os/os.h"
#include "nvim/os/shell.h"
#include "nvim/eval/encode.h"

#include "generated/config/config.h"

/// A version of strchr() that returns a pointer
/// to the terminating NUL if it doesn't find @b c.
///
/// @param str The string to search.
/// @param c   The char to look for.
///
/// @returns
/// a pointer to the first instance of @b c,
/// or to the NUL terminator if not found.
char *xstrchrnul(const char *str, char c)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    char *p = strchr(str, c);

    return p ? p : (char *)(str + strlen(str));
}

/// Replaces every instance of @b c with @b x.
///
/// @warning
/// Will read past 'str + strlen(str)' if 'c == NUL'.
///
/// @param str  A NUL-terminated string.
/// @param c    The unwanted byte.
/// @param x    The replacement.
void xstrchrsub(char *str, char c, char x)
FUNC_ATTR_NONNULL_ALL
{
    assert(c != '\0');

    while((str = strchr(str, c)))
    {
        *str++ = x;
    }
}

/// Counts the number of occurrences of @b c in @b str.
///
/// @warning Unsafe if 'c == NUL'.
///
/// @param str Pointer to the string to search.
/// @param c   The byte to search for.
///
/// @returns the number of occurrences of @b c in @b str.
size_t xstrcnt(const char *str, char c)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
{
    assert(c != 0);
    size_t cnt = 0;

    while((str = strchr(str, c)))
    {
        cnt++;
        str++; // Skip the instance of c.
    }

    return cnt;
}

/// Copies the string pointed to by src (including the
/// terminating NUL character) into the array pointed to by dst.
///
/// @warning
/// If copying takes place between objects that overlap,
/// the behavior is undefined.
///
/// Nvim version of POSIX 2008 stpcpy(3).
/// We do not require POSIX 2008, so implement our own version.
///
/// @param dst
/// @param src
///
/// @returns
/// pointer to the terminating NUL char copied into the dst buffer.
/// This is the only difference with strcpy(), which returns dst.
char *xstpcpy(char *restrict dst, const char *restrict src)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_WARN_UNUSED_RESULT
{
    const size_t len = strlen(src);

    return (char *)memcpy(dst, src, len + 1) + len;
}

/// Copies not more than n bytes (bytes that follow a NUL character are not
/// copied) from the array pointed to by src to the array pointed to by dst.
///
/// If a NUL character is written to the destination, xstpncpy() returns the
/// address of the first such NUL character. Otherwise, it shall return
/// &dst[maxlen].
///
/// @warning
/// If copying takes place between objects that overlap,
/// the behavior is undefined.
///
/// WARNING: xstpncpy will ALWAYS write maxlen bytes.
/// If src is shorter than maxlen, zeroes will be written
/// to the remaining bytes.
///
/// @param dst
/// @param src
/// @param maxlen
char *xstpncpy(char *restrict dst, const char *restrict src, size_t maxlen)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_WARN_UNUSED_RESULT
{
    const char *p = memchr(src, '\0', maxlen);

    if(p)
    {
        size_t srclen = (size_t)(p - src);
        memcpy(dst, src, srclen);
        memset(dst + srclen, 0, maxlen - srclen);
        return dst + srclen;
    }
    else
    {
        memcpy(dst, src, maxlen);
        return dst + maxlen;
    }
}

/// Copy a NUL-terminated string into a sized buffer
///
/// Compatible with *BSD strlcpy:
/// the result is always a valid NUL-terminated string
/// that fits in the buffer (unless, of course, the buffer
/// size is zero). It does not pad out the result like strncpy() does.
///
/// @param dst    Buffer to store the result
/// @param src    String to be copied
/// @param dsize  Size of @b dst
///
/// @return       strlen(src). If retval >= dstsize, truncation occurs.
size_t xstrncpy(char *restrict dst, const char *restrict src, size_t dsize)
FUNC_ATTR_NONNULL_ALL
{
    size_t slen = strlen(src);

    if(dsize)
    {
        size_t len = MIN(slen, dsize - 1);
        memcpy(dst, src, len);
        dst[len] = '\0';
    }

    return slen; // Does not include NUL.
}

/// Appends @b src to string @b dst of size @b dsize (unlike strncat,
/// dsize is the full size of @b dst, not space left). At most dsize-1
/// characters will be copied. Always NUL terminates. @b src and @b dst
/// may overlap.
///
/// @see vim_strcat from Vim.
/// @see strlcat from OpenBSD.
///
/// @param dst      Buffer to be appended-to. Must have a NUL byte.
/// @param src      String to put at the end of @b dst.
/// @param dsize    Size of @b dst including NUL byte. Must be greater than 0.
///
/// @return         strlen(src) + strlen(initial dst)
///                 If retval >= dsize, truncation occurs.
size_t xstrncat(char *const dst, const char *const src, const size_t dsize)
FUNC_ATTR_NONNULL_ALL
{
    assert(dsize > 0);
    const size_t dlen = strlen(dst);

    assert(dlen < dsize);
    const size_t slen = strlen(src);

    if(slen > dsize - dlen - 1)
    {
        memmove(dst + dlen, src, dsize - dlen - 1);
        dst[dsize - 1] = '\0';
    }
    else
    {
        memmove(dst + dlen, src, slen + 1);
    }

    return slen + dlen; // Does not include NUL.
}

/// strdup() wrapper
///
/// @param str 0-terminated string that will be copied
///
/// @return pointer to a copy of the string
///
/// @see xmalloc()
char *xstrdup(const char *str)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_WARN_UNUSED_RESULT

{
    return xmemdupz(str, strlen(str));
}

/// strndup() wrapper
///
/// @param str 0-terminated string that will be copied
///
/// @return pointer to a copy of the string
///
/// @see xmalloc()
char *xstrndup(const char *str, size_t len)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_WARN_UNUSED_RESULT
{
    char *p = memchr(str, '\0', len);
    return xmemdupz(str, p ? (size_t)(p - str) : len);
}

/// strdup() wrapper
///
/// Unlike xstrdup() allocates a new empty string if it receives NULL.
char *xstrdupnul(const char *const str)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(str == NULL)
    {
        return xmallocz(0);
    }
    else
    {
        return xstrdup(str);
    }
}

/// Returns true if strings @b a and @b b are equal.
/// Arguments may be NULL.
bool xstrequal(const char *a, const char *b)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return (a == NULL && b == NULL) || (a && b && strcmp(a, b) == 0);
}

/// Case-insensitive string-equal.
bool xstriequal(const char *a, const char *b)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return (a == NULL && b == NULL) || (a && b && ustricmp(a, b) == 0);
}

/// Copy @b string into newly allocated memory.
uchar_kt *ustrdup(const uchar_kt *string)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    return (uchar_kt *)xstrdup((char *)string);
}

/// Copy up to @b len bytes of @b string into newly allocated memory and
/// terminate with a NUL. The allocated memory always has size @b len+1,
/// even when @b string is shorter.
uchar_kt *ustrndup(const uchar_kt *string, size_t len)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    // strncpy is intentional: some parts of Vim use 'string' shorter
    // than 'len' and expect the remainder to be zeroed out.
    return (uchar_kt *)strncpy(xmallocz(len), (char *)string, len);
}

/// Same as ustrdup(), but any characters found in esc_chars are
/// preceded by a backslash.
uchar_kt *ustrdup_escape(const uchar_kt *string,
                         const uchar_kt *esc_chars)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    return ustrdup_escape_ext(string, esc_chars, '\\', false);
}

/// Same as ustrdup_escape(), but when "bsl" is true also escape
/// characters where rem_backslash() would remove the backslash.
/// Escape the characters with "cc".
uchar_kt *ustrdup_escape_ext(const uchar_kt *string,
                             const uchar_kt *esc_chars,
                             uchar_kt cc, bool bsl)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    // First count the number of backslashes required.
    // Then allocate the memory and insert them.
    size_t length = 1; // count the trailing NUL

    for(const uchar_kt *p = string; *p; p++)
    {
        size_t l;

        if((l = (size_t)(*mb_ptr2len)(p)) > 1)
        {
            length += l; // count a multibyte char
            p += l - 1;
            continue;
        }

        if(ustrchr(esc_chars, *p) != NULL || (bsl && rem_backslash(p)))
        {
            ++length; // count a backslash
        }

        ++length; // count an ordinary char
    }

    uchar_kt *escaped_string = xmalloc(length);
    uchar_kt *p2 = escaped_string;

    for(const uchar_kt *p = string; *p; p++)
    {
        size_t l;

        if((l = (size_t)(*mb_ptr2len)(p)) > 1)
        {
            memcpy(p2, p, l);
            p2 += l;
            p += l - 1; // skip multibyte char
            continue;
        }

        if(ustrchr(esc_chars, *p) != NULL || (bsl && rem_backslash(p)))
        {
            *p2++ = cc;
        }

        *p2++ = *p;
    }

    *p2 = NUL;

    return escaped_string;
}

/// Save a copy of an unquoted string
///
/// Turns string like `a\bc"def\"ghi\\\n"jkl` into `a\bcdef"ghi\\njkl`, for use
/// in shell_build_argv: the only purpose of backslash is making next character
/// be treated literally inside the double quotes, if this character is
/// backslash or quote.
///
/// @param[in]  string  String to copy.
/// @param[in]  length  Length of the string to copy.
///
/// @return [allocated] Copy of the string.
char *xstrdup_unquoted(const char *const string, const size_t length)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_WARN_UNUSED_RESULT
{
    size_t ret_length = 0;
    bool inquote = false;
    const char *const string_end = string + length;

    for(const char *p = string; p < string_end; p++)
    {
        if(*p == '"')
        {
            inquote = !inquote;
        }
        else if(*p == '\\'
                && inquote
                && p + 1 < string_end
                && (p[1] == '\\' || p[1] == '"'))
        {
            ret_length++;
            p++;
        }
        else
        {
            ret_length++;
        }
    }

    char *const ret = xmallocz(ret_length);
    char *rp = ret;
    inquote = false;

    for(const char *p = string; p < string_end; p++)
    {
        if(*p == '"')
        {
            inquote = !inquote;
        }
        else if(*p == '\\'
                && inquote
                && p + 1 < string_end
                && (p[1] == '\\' || p[1] == '"'))
        {
            *rp++ = *(++p);
        }
        else
        {
            *rp++ = *p;
        }
    }

    return ret;
}

/// Escape "string" for use as a shell argument with system().
/// This uses single quotes, except when we know we need to use
/// double quotes (MS-Windows without 'shellslash' set).
/// Escape a newline, depending on the 'shell' option.
/// When "do_special" is true also replace "!", "%", "#" and things
/// starting with "<" like "<cfile>".
/// When "do_newline" is false do not escape newline unless it is csh shell.
///
/// @return the result in allocated memory.
uchar_kt *ustrdup_escape_shell(const uchar_kt *string,
                               bool do_special,
                               bool do_newline)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    uchar_kt *d;
    uchar_kt *escaped_string;
    size_t l;
    int csh_like;

    // Only csh and similar shells expand '!' within single quotes.  For sh and
    // the like we must not put a backslash before it, it will be taken
    // literally. If do_special is set the '!' will be escaped twice.
    // Csh also needs to have "\n" escaped twice when do_special is set.
    csh_like = csh_like_shell();

    // First count the number of extra bytes required.
    size_t length = ustrlen(string) + 3; // two quotes and a trailing NUL

    for(const uchar_kt *p = string; *p != NUL; mb_ptr_adv(p))
    {
    #ifdef HOST_OS_WINDOWS
        if(!p_ssl)
        {
            if(*p == '"')
            {
                length++; // " -> ""
            }
        }
        else
    #endif
        {
            if(*p == '\'')
            {
                length += 3; // ' => '\''
            }
        }

        if((*p == '\n' && (csh_like || do_newline))
           || (*p == '!' && (csh_like || do_special)))
        {
            ++length; // insert backslash

            if(csh_like && do_special)
            {
                ++length; // insert backslash
            }
        }

        if(do_special && find_cmdline_var(p, &l) >= 0)
        {
            ++length; // insert backslash
            p += l - 1;
        }
    }

    // Allocate memory for the result and fill it.
    escaped_string = xmalloc(length);
    d = escaped_string;

    // add opening quote
#ifdef HOST_OS_WINDOWS
    if(!p_ssl)
    {
        *d++ = '"';
    }
    else
#endif
    {
        *d++ = '\'';
    }

    for(const uchar_kt *p = string; *p != NUL;)
    {
    #ifdef HOST_OS_WINDOWS
        if(!p_ssl)
        {
            if(*p == '"')
            {
                *d++ = '"';
                *d++ = '"';
                p++;
                continue;
            }
        }
        else
    #endif
        {
            if(*p == '\'')
            {
                *d++ = '\'';
                *d++ = '\\';
                *d++ = '\'';
                *d++ = '\'';
                ++p;
                continue;
            }
        }

        if((*p == '\n' && (csh_like || do_newline))
           || (*p == '!' && (csh_like || do_special)))
        {
            *d++ = '\\';

            if(csh_like && do_special)
            {
                *d++ = '\\';
            }

            *d++ = *p++;
            continue;
        }

        if(do_special && find_cmdline_var(p, &l) >= 0)
        {
            *d++ = '\\'; // insert backslash

            while(--l != SIZE_MAX) // copy the var
            {
                *d++ = *p++;
            }

            continue;
        }

        mb_copy_char(&p, &d);
    }

    // add terminating quote and finish with a NUL
#ifdef HOST_OS_WINDOWS
    if(!p_ssl)
    {
        *d++ = '"';
    }
    else
#endif
    {
        *d++ = '\'';
    }

    *d = NUL;

    return escaped_string;
}

/// Like ustrdup(), but make all characters uppercase.
/// This uses ASCII lower-to-upper case translation, language independent.
uchar_kt *ustrdup_upper(const uchar_kt *string)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    uchar_kt *p1;
    p1 = ustrdup(string);
    xstr_to_upper(p1);
    return p1;
}

/// Like ustrndup(), but make all characters uppercase.
/// This uses ASCII lower-to-upper case translation, language independent.
uchar_kt *ustrndup_to_upper(const uchar_kt *string, size_t len)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    uchar_kt *p1 = ustrndup(string, len);
    xstr_to_upper(p1);
    return p1;
}

/// ASCII lower-to-upper case translation, language independent.
void xstr_to_upper(uchar_kt *p)
FUNC_ATTR_NONNULL_ALL
{
    uchar_kt c;

    while((c = *p) != NUL)
    {
        *p++ = (uchar_kt)(c < 'a' || c > 'z' ? c : c - 0x20);
    }
}

/// Make given string all upper-case or all lower-case
///
/// Handles multi-byte characters as good as possible.
///
/// @param[in]  orig  Input string.
/// @param[in]  upper If true make uppercase, otherwise lowercase
///
/// @return [allocated] upper/lower cased string.
char *xstrdup_case_convert(const char *const orig, bool upper)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    char *res = xstrdup(orig);
    char *p = res;

    while(*p != NUL)
    {
        int l;
        int c = utf_ptr2char((const uchar_kt *)p);
        int uc = upper ? mb_toupper(c) : mb_tolower(c);

        // Reallocate string when byte count changes.
        // This is rare, thus it's OK to do another malloc()/free().
        l = utf_ptr2len((const uchar_kt *)p);
        int newl = utf_char2len(uc);

        if(newl != l)
        {
            // todo: use xrealloc() in strup_save()
            char *s = xmalloc(ustrlen(res) + (size_t)(1 + newl - l));
            memcpy(s, res, (size_t)(p - res));
            ustrcpy(s + (p - res) + newl, p + l);
            p = s + (p - res);
            xfree(res);
            res = s;
        }

        utf_char2bytes(uc, (uchar_kt *)p);
        p += newl;
    }

    return res;
}

/// delete spaces at the end of a string
void ustr_del_trailing_spaces(uchar_kt *ptr)
FUNC_ATTR_NONNULL_ALL
{
    uchar_kt *q;

    q = ptr + ustrlen(ptr);

    while(--q > ptr
          && ascii_iswhite(q[0])
          && q[-1] != '\\'
          && q[-1] != Ctrl_V)
    {
        *q = NUL;
    }
}

#if (!defined(HAVE_FUN_STRCASECMP) && !defined(HAVE_STRICMP))
/// Compare two strings, ignoring case, using current locale.
/// Doesn't work for multi-byte characters.
/// return 0 for match, < 0 for smaller, > 0 for bigger
int xstricmp(const char *s1, const char *s2)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
{
    int i;

    for(;;)
    {
        i = (int)TOLOWER_LOC(*s1) - (int)TOLOWER_LOC(*s2);

        if(i != 0)
        {
            return i; // this character different
        }

        if(*s1 == NUL)
        {
            break; // strings match until NUL
        }

        ++s1;
        ++s2;
    }

    return 0; // strings match
}
#endif

#if (!defined(HAVE_FUN_STRNCASECMP) && !defined(HAVE_STRNICMP))
/// Compare two strings, for length "len", ignoring case,
/// using current locale. Doesn't work for multi-byte characters.
/// return 0 for match, < 0 for smaller, > 0 for bigger
int xstrnicmp(const char *s1, const char *s2, size_t len)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
{
    int i;

    while(len > 0)
    {
        i = (int)TOLOWER_LOC(*s1) - (int)TOLOWER_LOC(*s2);

        if(i != 0)
        {
            return i; // this character different
        }

        if(*s1 == NUL)
        {
            break; // strings match until NUL
        }

        ++s1;
        ++s2;
        --len;
    }

    return 0; // strings match
}
#endif

/// like strchr(), which handles multibyte strings
///
/// @param[in]  string  String to search in.
/// @param[in]  c       Character to search for.
///
/// @return
/// Pointer to the first byte of the found character in string or NULL
/// if it was not found or character is invalid. NUL character is never
/// found, use strlen() instead.
uchar_kt *ustrchr(const uchar_kt *const string, const int c)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(c <= 0)
    {
        return NULL;
    }
    else if(c < 0x80)
    {
        return (uchar_kt *)strchr((const char *)string, c);
    }
    else
    {
        char u8char[MB_MAXBYTES + 1];
        const int len = utf_char2bytes(c, (uchar_kt *)u8char);
        u8char[len] = NUL;
        return (uchar_kt *)strstr((const char *)string, u8char);
    }
}

/// Search for last occurrence of "c" in "string".
/// Return NULL if not found.
/// Does not handle multi-byte char for "c"!
uchar_kt *ustrrchr(const uchar_kt *string, int c)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
{
    const uchar_kt *retval = NULL;
    const uchar_kt *p = string;

    while(*p)
    {
        if(*p == c)
        {
            retval = p;
        }

        mb_ptr_adv(p);
    }

    return (uchar_kt *) retval;
}

/// Sort an array of strings.

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "strings.c.generated.h"
#endif

static int sort_compare(const void *s1, const void *s2)
FUNC_ATTR_NONNULL_ALL
{
    return ustrcmp(*(char **)s1, *(char **)s2);
}

void ustr_quick_sort(uchar_kt **files, int count)
{
    qsort((void *)files, (size_t)count, sizeof(uchar_kt *), sort_compare);
}

/// Return true if string "s" contains a non-ASCII character (128 or higher).
/// When "s" is NULL false is returned.
bool has_non_ascii(const uchar_kt *s)
FUNC_ATTR_PURE
{
    const uchar_kt *p;

    if(s != NULL)
    {
        for(p = s; *p != NUL; ++p)
        {
            if(*p >= 128)
            {
                return true;
            }
        }
    }

    return false;
}

char *xstrdup_concat(const char *restrict str1,
                     const char *restrict str2)
{
    return (char *)ustrdup_concat((const uchar_kt *)str1,
                                  (const uchar_kt *)str2);
}

/// Concatenate two strings and return the result in allocated memory.
uchar_kt *ustrdup_concat(const uchar_kt *restrict str1,
                         const uchar_kt *restrict str2)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    size_t l = ustrlen(str1);
    uchar_kt *dest = xmalloc(l + ustrlen(str2) + 1);
    ustrcpy(dest, str1);
    ustrcpy(dest + l, str2);
    return dest;
}

static const char *const e_printf =
    N_("E766: Insufficient arguments for printf()");

/// Get number argument from idxp entry in tvs
///
/// Will give an error message for VimL entry with invalid type or for
/// insufficient entries.
///
/// @param[in]  tvs
/// List of VimL values. List is terminated by kNvarUnknown value.
///
/// @param[in,out] idxp
/// Index in a list. Will be incremented. Indexing starts at 1.
///
/// @return Number value or 0 in case of error.
static number_kt tv_nr(typval_st *tvs, int *idxp)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    int idx = *idxp - 1;
    number_kt n = 0;

    if(tvs[idx].v_type == kNvarUnknown)
    {
        EMSG(_(e_printf));
    }
    else
    {
        (*idxp)++;
        bool err = false;
        n = tv_get_number_chk(&tvs[idx], &err);

        if(err)
        {
            n = 0;
        }
    }

    return n;
}

/// Get string argument from idxp entry in tvs
///
/// Will give an error message for VimL entry with invalid type or for
/// insufficient entries.
///
/// @param[in] tvs
/// List of VimL values. List is terminated by kNvarUnknown value.
///
/// @param[in,out] idxp
/// Index in a list. Will be incremented.
///
/// @param[out] tofree
/// If the idxp entry in tvs is not a String or a Number,
/// it will be converted to String in the same format
/// as ":echo" and stored in "*tofree". The caller must free "*tofree".
///
/// @return String value or NULL in case of error.
static const char *tv_str(typval_st *tvs, int *idxp, char **const tofree)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    int idx = *idxp - 1;
    const char *s = NULL;

    if(tvs[idx].v_type == kNvarUnknown)
    {
        EMSG(_(e_printf));
    }
    else
    {
        (*idxp)++;

        if(tvs[idx].v_type == kNvarString || tvs[idx].v_type == kNvarNumber)
        {
            s = tv_get_string_chk(&tvs[idx]);
            *tofree = NULL;
        }
        else
        {
            s = *tofree = encode_tv2echo(&tvs[idx], NULL);
        }
    }

    return s;
}

/// Get pointer argument from the next entry in tvs
///
/// Will give an error message for VimL entry with invalid type or for
/// insufficient entries.
///
/// @param[in]  tvs       List of typval_st values.
/// @param[in,out]  idxp  Pointer to the index of the current value.
///
/// @return Pointer stored in typval_st or NULL.
static const void *tv_ptr(const typval_st *const tvs, int *const idxp)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
#define OFF(attr) offsetof(typval_val_ut, attr)

    STATIC_ASSERT(OFF(v_string) == OFF(v_list)
                  && OFF(v_string) == OFF(v_dict)
                  && OFF(v_string) == OFF(v_partial)
                  && sizeof(tvs[0].vval.v_string) == sizeof(tvs[0].vval.v_list)
                  && sizeof(tvs[0].vval.v_string) == sizeof(tvs[0].vval.v_dict)
                  && sizeof(tvs[0].vval.v_string) == sizeof(tvs[0].vval.v_partial),
                  "Strings, dictionaries, lists and partials are expected to "
                  "be pointers, so that all three of them can be accessed "
                  "via v_string");

#undef OFF

    const int idx = *idxp - 1;

    if(tvs[idx].v_type == kNvarUnknown)
    {
        EMSG(_(e_printf));
        return NULL;
    }
    else
    {
        (*idxp)++;
        return tvs[idx].vval.v_string;
    }
}

/// Get float argument from idxp entry in tvs
///
/// Will give an error message for VimL entry with invalid type or for
/// insufficient entries.
///
/// @param[in] tvs
/// List of VimL values. List is terminated by kNvarUnknown value.
///
/// @param[in,out] idxp
/// Index in a list. Will be incremented.
///
/// @return Floating-point value or zero in case of error.
static float_kt tv_float(typval_st *const tvs, int *const idxp)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    int idx = *idxp - 1;
    float_kt f = 0;

    if(tvs[idx].v_type == kNvarUnknown)
    {
        EMSG(_(e_printf));
    }
    else
    {
        (*idxp)++;

        if(tvs[idx].v_type == kNvarFloat)
        {
            f = tvs[idx].vval.v_float;
        }
        else if(tvs[idx].v_type == kNvarNumber)
        {
            f = tvs[idx].vval.v_number;
        }
        else
        {
            EMSG(_("E807: Expected Float argument for printf()"));
        }
    }

    return f;
}

// This code was included to provide a portable vsnprintf() and snprintf().
// Some systems may provide their own, but we always use this one for
// consistency.
//
// This code is based on snprintf.c - a portable implementation of snprintf
// by Mark Martinec <mark.martinec@ijs.si>, Version 2.2, 2000-10-06.
// Included with permission.  It was heavily modified to fit in Vim.
// The original code, including useful comments, can be found here:
//
//     http://www.ijs.si/software/snprintf/
//
// This snprintf() only supports the following conversion specifiers:
// s, c, b, B, d, u, o, x, X, p  (and synonyms: i, D, U, O - see below)
// with flags: '-', '+', ' ', '0' and '#'.
// An asterisk is supported for field width as well as precision.
//
// Limited support for floating point was added: 'f', 'e', 'E', 'g', 'G'.
//
// Length modifiers 'h' (short int), 'l' (long int) and
// "ll" (long long int) are supported.
//
// The locale is not used, the string is used as a byte string.  This is only
// relevant for double-byte encodings where the second byte may be '%'.
//
// It is permitted for "str_m" to be zero, and it is permitted to specify NULL
// pointer for resulting string argument if "str_m" is zero (as per ISO C99).
//
// The return value is the number of characters which would be generated
// for the given input, excluding the trailing NUL. If this value
// is greater or equal to "str_m", not all characters from the result
// have been stored in str, output bytes beyond the ("str_m"-1) -th character
// are discarded. If "str_m" is greater than zero it is guaranteed
// the resulting string will be NUL-terminated.
//
// xvsnprintf() can be invoked with either "va_list" or a list of
// "typval_st". When the latter is not used it must be NULL.

/// Append a formatted value to the string
///
/// @param str      string buffer to operating
/// @param str_m    the size of the string buffer
/// @param fmt      the format string
/// @param ...      the arguments to append
///
/// @see xvsnprintf().
int xvsnprintf_add(char *str, size_t str_m, char *fmt, ...)
{
    const size_t len = strlen(str);
    size_t space; // the left space can be used in the buffer

    if(str_m <= len)
    {
        space = 0;
    }
    else
    {
        space = str_m - len;
    }

    va_list ap;
    va_start(ap, fmt);
    const int str_l = xvsnprintf(str + len, space, fmt, ap, NULL);
    va_end(ap);

    return str_l;
}

/// Write formatted value to the string
///
/// @param[out] str    String to write to.
/// @param[in]  str_m  String length.
/// @param[in]  fmt    String format.
///
/// @return
/// Number of bytes excluding NUL byte that would be written to the
/// string if str_m was greater or equal to the return value.
int xsnprintf(char *str, size_t str_m, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int str_l = xvsnprintf(str, str_m, fmt, ap, NULL);
    va_end(ap);
    return str_l;
}

// Return the representation of infinity for printf() function:
// "-inf", "inf", "+inf", " inf", "-INF", "INF", "+INF" or " INF".
static const char *infinity_str(bool positive,
                                char fmt_spec,
                                int force_sign,
                                int space_for_positive)
{
    static const char *table[] = {
        "-inf", "inf", "+inf", " inf",
        "-INF", "INF", "+INF", " INF"
    };

    int idx = positive * (1 + force_sign + force_sign * space_for_positive);

    if(ASCII_ISUPPER(fmt_spec))
    {
        idx += 4;
    }

    return table[idx];
}


/// Write formatted value to the string
///
/// @param[out] str
/// String to write to.
///
/// @param[in]  str_m
/// String length.
///
/// @param[in]  fmt
/// String format.
///
/// @param[in]  ap
/// Values that should be formatted. Ignored if tvs is not NULL.
///
/// @param[in]  tvs
/// Values that should be formatted, for printf() VimL function.
/// Must be NULL in other cases.
///
/// @return
/// Number of bytes excluding NUL byte that would be written to the
/// string if str_m was greater or equal to the return value.
int xvsnprintf(char *str,
               size_t str_m,
               const char *fmt,
               va_list ap,
               typval_st *const tvs)
{
    size_t str_l = 0;
    bool str_avail = str_l < str_m;
    const char *p = fmt;
    int arg_idx = 1;

    if(!p)
    {
        p = "";
    }

    while(*p)
    {
        if(*p != '%')
        {
            // copy up to the next '%' or NUL without any changes
            size_t n = (size_t)(xstrchrnul(p + 1, '%') - p);

            if(str_avail)
            {
                size_t avail = str_m - str_l;
                memmove(str + str_l, p, MIN(n, avail));
                str_avail = n < avail;
            }

            p += n;
            assert(n <= SIZE_MAX - str_l);
            str_l += n;
        }
        else
        {
            size_t min_field_width = 0, precision = 0;
            int zero_padding = 0, precision_specified = 0, justify_left = 0;
            int alternate_form = 0, force_sign = 0;

            // if both ' ' and '+' flags appear, ' ' flag should be ignored
            int space_for_positive = 1;

            // allowed values: \0, h, l, 2 (for ll), z, L
            char length_modifier = '\0';

            // temporary buffer for simple numeric->string conversion

// 1e308 seems reasonable as the maximum printable
#define TMP_LEN  350

            char tmp[TMP_LEN];

            // string address in case of string argument
            const char *str_arg = NULL;

            // natural field width of arg without padding and sign unsigned
            // char argument value (only defined for c conversion); standard
            // explicitly states the char argument for the c conversion is
            // unsigned
            size_t str_arg_l;
            unsigned char uchar_arg;

            // number of zeros to be inserted for numeric conversions as
            // required by the precision or minimal field width
            size_t number_of_zeros_to_pad = 0;

            // index into tmp where zero padding is to be inserted
            size_t zero_padding_insertion_ind = 0;

            char fmt_spec = '\0'; // current conversion specifier character
            char *tofree = NULL; // buffer for 's' and 'S' specs
            p++;  // skip '%'

            while(true) // parse flags
            {
                switch(*p)
                {
                    case '0':
                        zero_padding = 1;
                        p++;
                        continue;

                    case '-':
                        justify_left = 1;
                        p++;
                        continue;

                    // if both '0' and '-' flags appear, '0' should be ignored
                    case '+':
                        force_sign = 1;
                        space_for_positive = 0;
                        p++;
                        continue;

                    case ' ':
                        force_sign = 1;
                        p++;
                        continue;

                    // if both ' ' and '+' flags appear, ' ' should be ignored
                    case '#':
                        alternate_form = 1;
                        p++;
                        continue;

                    case '\'':
                        p++;
                        continue;

                    default:
                        break;
                }

                break;
            }

            if(*p == '*') // parse field width
            {
                p++;
                const int j = tvs
                              ? (int)tv_nr(tvs, &arg_idx)
                              : va_arg(ap, int);

                if(j >= 0)
                {
                    min_field_width = (size_t)j;
                }
                else
                {
                    min_field_width = (size_t)-j;
                    justify_left = 1;
                }
            }
            else if(ascii_isdigit((int)(*p)))
            {
                // size_t could be wider than unsigned int; make sure
                // we treat argument like common implementations do
                unsigned int uj = (unsigned)(*p++ - '0');

                while(ascii_isdigit((int)(*p)))
                {
                    uj = 10 * uj + (unsigned int)(*p++ - '0');
                }

                min_field_width = uj;
            }

            if(*p == '.') // parse precision
            {
                p++;
                precision_specified = 1;

                if(*p == '*')
                {
                    const int j = tvs
                                  ? (int)tv_nr(tvs, &arg_idx)
                                  : va_arg(ap, int);
                    p++;

                    if(j >= 0)
                    {
                        precision = (size_t)j;
                    }
                    else
                    {
                        precision_specified = 0;
                        precision = 0;
                    }
                }
                else if(ascii_isdigit((int)(*p)))
                {
                    // size_t could be wider than unsigned int; make sure
                    // we treat argument like common implementations do
                    unsigned int uj = (unsigned)(*p++ - '0');

                    while(ascii_isdigit((int)(*p)))
                    {
                        uj = 10 * uj + (unsigned int)(*p++ - '0');
                    }

                    precision = uj;
                }
            }

            // parse 'h', 'l', 'll' and 'z' length modifiers
            if(*p == 'h' || *p == 'l' || *p == 'z')
            {
                length_modifier = *p;
                p++;

                if(length_modifier == 'l' && *p == 'l') // ll, encoded as 2
                {
                    length_modifier = '2';
                    p++;
                }
            }

            fmt_spec = *p;

            switch(fmt_spec) // common synonyms
            {
                case 'i':
                    fmt_spec = 'd';
                    break;

                case 'D':
                    fmt_spec = 'd';
                    length_modifier = 'l';
                    break;

                case 'U':
                    fmt_spec = 'u';
                    length_modifier = 'l';
                    break;

                case 'O':
                    fmt_spec = 'o';
                    length_modifier = 'l';
                    break;

                default:
                    break;
            }

            // get parameter value, do initial processing
            switch(fmt_spec)
            {
                // '%' and 'c' behave similar to 's'
                // regarding flags and field widths
                case '%':
                case 'c':
                case 's':
                case 'S':
                    str_arg_l = 1;

                    switch(fmt_spec)
                    {
                        case '%':
                            str_arg = p;
                            break;

                        case 'c':
                        {
                            const int j = tvs
                                          ? (int)tv_nr(tvs, &arg_idx)
                                          : va_arg(ap, int);
                            // standard demands unsigned char
                            uchar_arg = (unsigned char)j;
                            str_arg = (char *)&uchar_arg;
                            break;
                        }

                        case 's':
                        case 'S':
                            str_arg = tvs
                                      ? tv_str(tvs, &arg_idx, &tofree)
                                      : va_arg(ap, const char *);

                            if(!str_arg)
                            {
                                str_arg = "[NULL]";
                                str_arg_l = 6;
                            }
                            else if(!precision_specified)
                            {
                                // make sure not to address string
                                // beyond the specified precision
                                str_arg_l = strlen(str_arg);
                            }
                            else if(precision == 0)
                            {
                                // truncate string if necessary as
                                // requested by precision
                                str_arg_l = 0;
                            }
                            else
                            {
                                // memchr on HP does not like n > 2^31
                                //
                                // TODO(elmart):
                                // check if this still holds / is relevant
                                str_arg_l =
                                    (size_t)((char *)xmemscan(str_arg,
                                                              NUL,
                                                              MIN(precision,
                                                              0x7fffffff))
                                             - str_arg);
                            }

                            if(fmt_spec == 'S')
                            {
                                if(min_field_width != 0)
                                {
                                    min_field_width +=
                                        (strlen(str_arg)
                                         - mb_string2cells((uchar_kt *)str_arg));
                                }

                                if(precision)
                                {
                                    const char *p1 = str_arg;

                                    for(size_t i = 0; i < precision && *p1; i++)
                                    {
                                        p1 += mb_ptr2len((const uchar_kt *)p1);
                                    }

                                    precision = (size_t)(p1 - str_arg);
                                    str_arg_l = (size_t)(p1 - str_arg);
                                }
                            }

                            break;

                        default:
                            break;
                    }

                    break;

                case 'd':
                case 'u':
                case 'b':
                case 'B':
                case 'o':
                case 'x':
                case 'X':
                case 'p':
                {
                    // u, b, B, o, x, X and p conversion specifiers imply
                    // the value is unsigned; d implies a signed value
                    // 0 if numeric argument is zero (or if pointer is NULL
                    // for 'p'), +1 if greater than zero (or non NULL for 'p'),
                    // -1 if negative (unsigned argument is never negative)
                    int arg_sign = 0;
                    intmax_t arg = 0;
                    uintmax_t uarg = 0;

                    // only defined for p conversion
                    const void *ptr_arg = NULL;

                    if(fmt_spec == 'p')
                    {
                        ptr_arg = tvs
                                  ? tv_ptr(tvs, &arg_idx)
                                  : va_arg(ap, void *);

                        if(ptr_arg)
                        {
                            arg_sign = 1;
                        }
                    }
                    else if(fmt_spec == 'd')
                    {
                        switch(length_modifier) // signed
                        {
                            case '\0':
                            case 'h':
                            {
                                // char and short arguments are passed as int
                                arg = (tvs
                                       ? (int)tv_nr(tvs, &arg_idx)
                                       : va_arg(ap, int));
                                break;
                            }

                            case 'l':
                            {
                                arg = (tvs
                                       ? (long)tv_nr(tvs, &arg_idx)
                                       : va_arg(ap, long));
                                break;
                            }

                            case '2':
                            {
                                arg = (tvs
                                       ? (long long)tv_nr(tvs, &arg_idx)
                                       : va_arg(ap, long long));
                                break;
                            }

                            case 'z':
                            {
                                arg = (tvs
                                       ? (ptrdiff_t)tv_nr(tvs, &arg_idx)
                                       : va_arg(ap, ptrdiff_t));
                                break;
                            }
                        }

                        if(arg > 0)
                        {
                            arg_sign =  1;
                        }
                        else if(arg < 0)
                        {
                            arg_sign = -1;
                        }
                    }
                    else
                    {
                        switch(length_modifier) // unsigned
                        {
                            case '\0':
                            case 'h':
                            {
                                uarg = (tvs
                                        ? (unsigned)tv_nr(tvs, &arg_idx)
                                        : va_arg(ap, unsigned));
                                break;
                            }

                            case 'l':
                            {
                                uarg = (tvs
                                        ? (unsigned long)tv_nr(tvs, &arg_idx)
                                        : va_arg(ap, unsigned long));
                                break;
                            }

                            case '2':
                            {
                                uarg = (uintmax_t)(unsigned long long)(
                                           tvs
                                           ? ((unsigned long long)
                                              tv_nr(tvs, &arg_idx))
                                           : va_arg(ap, unsigned long long));
                                break;
                            }

                            case 'z':
                            {
                                uarg = (tvs
                                        ? (size_t)tv_nr(tvs, &arg_idx)
                                        : va_arg(ap, size_t));
                                break;
                            }
                        }

                        arg_sign = (uarg != 0);
                    }

                    str_arg = tmp;
                    str_arg_l = 0;

                    // For d, i, u, o, x, and X conversions, if precision
                    // is specified, '0' flag should be ignored. This is
                    // so with Solaris 2.6, Digital UNIX 4.0, HPUX 10,
                    // Linux, FreeBSD, NetBSD; but not with Perl.
                    if(precision_specified)
                    {
                        zero_padding = 0;
                    }

                    if(fmt_spec == 'd')
                    {
                        if(force_sign && arg_sign >= 0)
                        {
                            tmp[str_arg_l++] = space_for_positive ? ' ' : '+';
                        }

                        // leave negative numbers for snprintf to handle, to
                        // avoid handling tricky cases like (short int)-32768
                    }
                    else if(alternate_form)
                    {
                        if(arg_sign != 0
                           && (fmt_spec == 'x'
                               || fmt_spec == 'X'
                               || fmt_spec == 'b'
                               || fmt_spec == 'B'))
                        {
                            tmp[str_arg_l++] = '0';
                            tmp[str_arg_l++] = fmt_spec;
                        }

                        // alternate form should have no
                        // effect for p * conversion, but ...
                    }

                    zero_padding_insertion_ind = str_arg_l;

                    if(!precision_specified)
                    {
                        precision = 1; // default precision is 1
                    }

                    if(precision == 0 && arg_sign == 0)
                    {
                        // when zero value is formatted with an explicit
                        // precision 0, resulting formatted string is empty
                        // (d, i, u, b, B, o, x, X, p)
                    }
                    else
                    {
                        switch(fmt_spec)
                        {
                            case 'p': // pointer
                            {
                                str_arg_l +=
                                    (size_t)snprintf(tmp + str_arg_l,
                                                     sizeof(tmp) - str_arg_l,
                                                     "%p",
                                                     ptr_arg);
                                break;
                            }

                            case 'd': // signed
                            {
                                str_arg_l +=
                                    (size_t)snprintf(tmp + str_arg_l,
                                                     sizeof(tmp) - str_arg_l,
                                                     "%" PRIdMAX,
                                                     arg);
                                break;
                            }

                            case 'b':
                            case 'B': // binary
                            {
                                size_t bits = 0;

                                for(bits = sizeof(uintmax_t) * 8; bits > 0; bits--)
                                {
                                    if((uarg >> (bits - 1)) & 0x1)
                                    {
                                        break;
                                    }
                                }

                                while(bits > 0)
                                {
                                    tmp[str_arg_l++] =
                                        ((uarg >> --bits) & 0x1) ? '1' : '0';
                                }

                                break;
                            }

                            default: // unsigned
                            {
                                // construct a simple format
                                // string for snprintf
                                char f[] = "%" PRIuMAX;

                                f[sizeof("%" PRIuMAX) - 1 - 1] = fmt_spec;

                                assert(PRIuMAX[sizeof(PRIuMAX) - 1 - 1] == 'u');

                                str_arg_l +=
                                    (size_t)snprintf(tmp + str_arg_l,
                                                     sizeof(tmp) - str_arg_l,
                                                     f,
                                                     uarg);
                                break;
                            }

                            assert(str_arg_l < sizeof(tmp));
                        }

                        // include the optional minus sign and possible
                        // "0x" in the region before the zero padding
                        // insertion point
                        if(zero_padding_insertion_ind < str_arg_l
                           && tmp[zero_padding_insertion_ind] == '-')
                        {
                            zero_padding_insertion_ind++;
                        }

                        if(zero_padding_insertion_ind + 1 < str_arg_l
                           && tmp[zero_padding_insertion_ind]   == '0'
                           && (tmp[zero_padding_insertion_ind + 1] == 'x'
                               || tmp[zero_padding_insertion_ind + 1] == 'X'
                               || tmp[zero_padding_insertion_ind + 1] == 'b'
                               || tmp[zero_padding_insertion_ind + 1] == 'B'))
                        {
                            zero_padding_insertion_ind += 2;
                        }
                    }

                    size_t num_of_digits = str_arg_l
                                           - zero_padding_insertion_ind;

                    if(alternate_form
                       && fmt_spec == 'o'
                       // unless zero is already the first character
                       && !(zero_padding_insertion_ind < str_arg_l
                            && tmp[zero_padding_insertion_ind] == '0'))
                    {
                        // assure leading zero for
                        // alternate-form octal numbers
                        if(!precision_specified
                           || precision < num_of_digits + 1)
                        {
                            // precision is increased to force the first
                            // character to be zero, except if a zero value
                            // is formatted with an explicit precision of zero
                            precision = num_of_digits + 1;
                        }
                    }

                    // zero padding to specified precision?
                    if(num_of_digits < precision)
                    {
                        number_of_zeros_to_pad = precision - num_of_digits;
                    }

                    // zero padding to specified minimal field width?
                    if(!justify_left && zero_padding)
                    {
                        const int n =
                            (int)(min_field_width
                                  - (str_arg_l + number_of_zeros_to_pad));

                        if(n > 0)
                        {
                            number_of_zeros_to_pad += (size_t)n;
                        }
                    }

                    break;
                }

                case 'f':
                case 'F':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                {
                    // floating point
                    char format[40];
                    int remove_trailing_zeroes = false;

                    double f = tvs
                               ? tv_float(tvs, &arg_idx)
                               : va_arg(ap, double);

                    double abs_f = f < 0 ? -f : f;

                    if(fmt_spec == 'g' || fmt_spec == 'G')
                    {
                        // can't use %g directly, cause it prints "1.0" as "1"
                        if((abs_f >= 0.001 && abs_f < 10000000.0)
                           || abs_f == 0.0)
                        {
                            fmt_spec = ASCII_ISUPPER(fmt_spec) ? 'F' : 'f';
                        }
                        else
                        {
                            fmt_spec = fmt_spec == 'g' ? 'e' : 'E';
                        }

                        remove_trailing_zeroes = true;
                    }

                    if(isinf((float) f)
                       || (strchr("fF", fmt_spec) != NULL && abs_f > 1.0e307))
                    {
                        xstrncpy(tmp,
                                 infinity_str(f > 0.0,
                                              fmt_spec,
                                              force_sign,
                                              space_for_positive),
                                 sizeof(tmp));

                        str_arg_l = strlen(tmp);
                        zero_padding = 0;
                    }
                    else if(isnan((float) f))
                    {
                        // Not a number: nan or NAN
                        memmove(tmp,
                                ASCII_ISUPPER(fmt_spec) ? "NAN" : "nan",
                                4);

                        str_arg_l = 3;
                        zero_padding = 0;
                    }
                    else
                    {
                        format[0] = '%';
                        size_t l = 1;

                        if(force_sign)
                        {
                            format[l++] = space_for_positive ? ' ' : '+';
                        }

                        if(precision_specified)
                        {
                            size_t max_prec = TMP_LEN - 10;

                            // make sure we don't get more digits
                            // than we have room for
                            if((fmt_spec == 'f' || fmt_spec == 'F')
                               && abs_f > 1.0)
                            {
                                max_prec -= (size_t)log10(abs_f);
                            }

                            if(precision > max_prec)
                            {
                                precision = max_prec;
                            }

                            l += (size_t)snprintf(format + l,
                                                  sizeof(format) - l,
                                                  ".%d",
                                                  (int)precision);
                        }

                        // Cast to char to avoid a
                        // conversion warning on Ubuntu 12.04.
                        format[l] = (char)(fmt_spec == 'F' ? 'f' : fmt_spec);

                        format[l + 1] = NUL;
                        assert(l + 1 < sizeof(format)); // Regular float number

                        str_arg_l = (size_t)snprintf(tmp,
                                                     sizeof(tmp),
                                                     format,
                                                     f);

                        assert(str_arg_l < sizeof(tmp));

                        if(remove_trailing_zeroes)
                        {
                            int i;
                            char *tp;

                            // using %g or %G: remove superfluous zeroes
                            if(fmt_spec == 'f' || fmt_spec == 'F')
                            {
                                tp = tmp + str_arg_l - 1;
                            }
                            else
                            {
                                tp = (char *)ustrchr((uchar_kt *)tmp,
                                                        fmt_spec == 'e'
                                                        ? 'e' : 'E');

                                if(tp)
                                {
                                    // remove superfluous '+' and
                                    // leading zeroes from exponent
                                    if(tp[1] == '+')
                                    {
                                        // change "1.0e+07" to "1.0e07"
                                        xstrmove(tp + 1, tp + 2);
                                        str_arg_l--;
                                    }

                                    i = (tp[1] == '-') ? 2 : 1;

                                    while(tp[i] == '0')
                                    {
                                        // change "1.0e07" to "1.0e7"
                                        xstrmove(tp + i, tp + i + 1);
                                        str_arg_l--;
                                    }

                                    tp--;
                                }
                            }

                            if(tp != NULL && !precision_specified)
                            {
                                // remove trailing zeroes, but
                                // keep the one just after a dot
                                while(tp > tmp + 2
                                      && *tp == '0'
                                      && tp[-1] != '.')
                                {
                                    xstrmove(tp, tp + 1);
                                    tp--;
                                    str_arg_l--;
                                }
                            }
                        }
                        else
                        {
                            // Be consistent:
                            // some printf("%e") use 1.0e+12 and some
                            // 1.0e+012; remove one zero in the last case.
                            char *tp = (char *)ustrchr((uchar_kt *)tmp,
                                                          fmt_spec == 'e'
                                                          ? 'e' : 'E');

                            if(tp
                               && (tp[1] == '+' || tp[1] == '-')
                               && tp[2] == '0'
                               && ascii_isdigit(tp[3])
                               && ascii_isdigit(tp[4]))
                            {
                                xstrmove(tp + 2, tp + 3);
                                str_arg_l--;
                            }
                        }
                    }

                    if(zero_padding
                       && min_field_width > str_arg_l
                       && (tmp[0] == '-' || force_sign))
                    {
                        // Padding 0's should be inserted after the sign.
                        number_of_zeros_to_pad = min_field_width - str_arg_l;
                        zero_padding_insertion_ind = 1;
                    }

                    str_arg = tmp;
                    break;
                }

                default:
                    // unrecognized conversion specifier,
                    // keep format string as-is turn zero
                    // padding off for non-numeric conversion
                    zero_padding = 0;
                    justify_left = 1;
                    min_field_width = 0; // reset flags

                    // discard the unrecognized conversion, just keep
                    // the unrecognized conversion character
                    str_arg = p;
                    str_arg_l = 0;

                    if(*p)
                    {
                        str_arg_l++; // include invalid conversion specifier
                    }

                    break; // unchanged if not at end-of-string
            }

            if(*p)
            {
                p++; // step over the just processed conversion specifier
            }

            // insert padding to the left as requested by min_field_width;
            // this does not include the zero padding in case of numerical
            // conversions
            if(!justify_left)
            {
                assert(str_arg_l <= SIZE_MAX - number_of_zeros_to_pad);

                if(min_field_width > str_arg_l + number_of_zeros_to_pad)
                {
                    // left padding with blank or zero
                    size_t pn = min_field_width
                                - (str_arg_l + number_of_zeros_to_pad);

                    if(str_avail)
                    {
                        size_t avail = str_m - str_l;

                        memset(str + str_l,
                               zero_padding ? '0' : ' ',
                               MIN(pn, avail));

                        str_avail = pn < avail;
                    }

                    assert(pn <= SIZE_MAX - str_l);
                    str_l += pn;
                }
            }

            // zero padding as requested by the precision or by the minimal
            // field width for numeric conversions required?
            if(number_of_zeros_to_pad == 0)
            {
                // will not copy first part of numeric right now,
                // force it to be copied later in its entirety
                zero_padding_insertion_ind = 0;
            }
            else
            {
                // insert first part of numerics
                // (sign or '0x') before zero padding
                if(zero_padding_insertion_ind > 0)
                {
                    size_t zn = zero_padding_insertion_ind;

                    if(str_avail)
                    {
                        size_t avail = str_m - str_l;
                        memmove(str + str_l, str_arg, MIN(zn, avail));
                        str_avail = zn < avail;
                    }

                    assert(zn <= SIZE_MAX - str_l);
                    str_l += zn;
                }

                // insert zero padding as requested
                // by precision or min field width
                if(number_of_zeros_to_pad > 0)
                {
                    size_t zn = number_of_zeros_to_pad;

                    if(str_avail)
                    {
                        size_t avail = str_m - str_l;
                        memset(str + str_l, '0', MIN(zn, avail));
                        str_avail = zn < avail;
                    }

                    assert(zn <= SIZE_MAX - str_l);
                    str_l += zn;
                }
            }

            // insert formatted string
            // (or as-is conversion specifier for unknown conversions)
            if(str_arg_l > zero_padding_insertion_ind)
            {
                size_t sn = str_arg_l - zero_padding_insertion_ind;

                if(str_avail)
                {
                    size_t avail = str_m - str_l;

                    memmove(str + str_l,
                            str_arg + zero_padding_insertion_ind,
                            MIN(sn, avail));

                    str_avail = sn < avail;
                }

                assert(sn <= SIZE_MAX - str_l);
                str_l += sn;
            }

            // insert right padding
            if(justify_left)
            {
                assert(str_arg_l <= SIZE_MAX - number_of_zeros_to_pad);

                if(min_field_width > str_arg_l + number_of_zeros_to_pad)
                {
                    // right blank padding to the field width
                    size_t pn = min_field_width
                                - (str_arg_l + number_of_zeros_to_pad);

                    if(str_avail)
                    {
                        size_t avail = str_m - str_l;
                        memset(str + str_l, ' ', MIN(pn, avail));
                        str_avail = pn < avail;
                    }

                    assert(pn <= SIZE_MAX - str_l);
                    str_l += pn;
                }
            }

            xfree(tofree);
        }
    }

    if(str_m > 0)
    {
        // make sure the string is nul-terminated even at the expense of
        // overwriting the last character (shouldn't happen, but just in case)
        str[str_l <= str_m - 1 ? str_l : str_m - 1] = '\0';
    }

    if(tvs && tvs[arg_idx - 1].v_type != kNvarUnknown)
    {
        EMSG(_("E767: Too many arguments to printf()"));
    }

    // return the number of characters formatted (excluding trailing nul
    // character); that is, the number of characters that would have been
    // written to the buffer if it were large enough.
    return (int)str_l;
}

/// Return the number of character cells string "s[len]" will take on the
/// screen, counting TABs as two characters: "^I".
///
/// 's' must be non-null.
///
/// @param s
/// @param len
///
/// @return Number of character cells.
int ustr_scrsize_len(uchar_kt *s, int len)
{
    assert(s != NULL);
    int size = 0;

    while(*s != NUL && --len >= 0)
    {
        int l = (*mb_ptr2len)(s);
        size += ptr2cells(s);
        s += l;
        len -= l - 1;
    }

    return size;
}

/// Return the number of character cells string "s" will take on the screen,
/// counting TABs as two characters: "^I".
///
/// 's' must be non-null.
///
/// @param s
///
/// @return number of character cells.
int ustr_scrsize(uchar_kt *s)
{
    return ustr_scrsize_len(s, (int)MAXCOL);
}

/// Convert the string to do ignore-case comparing.
/// Use the current locale.
///
/// @param str      input string to convert
/// @param orglen   the input string length
/// @param buf      the buffer to write the converted string
/// @param buflen   the buffer length
///
/// @return the converted string
/// - if @b buf is NULL, return an allocated string
/// - put the result in @b buf, limited by @b buflen, and return @b buf
uchar_kt *ustr_foldcase(uchar_kt *str, int orglen, uchar_kt *buf, int buflen)
FUNC_ATTR_NONNULL_RETURN
{
    int i;
    garray_st ga;
    int len = orglen;

    #define GA_CHAR(i)    ((uchar_kt *)ga.ga_data)[i]
    #define GA_PTR(i)     ((uchar_kt *)ga.ga_data + i)
    #define STR_CHAR(i)   (buf == NULL ? GA_CHAR(i) : buf[i])
    #define STR_PTR(i)    (buf == NULL ? GA_PTR(i) : buf + i)

    // Copy "str" into "buf" or allocated memory, unmodified.
    if(buf == NULL)
    {
        ga_init(&ga, 1, 10);
        ga_grow(&ga, len + 1);
        memmove(ga.ga_data, str, (size_t)len);
        ga.ga_len = len;
    }
    else
    {
        if(len >= buflen)
        {
            // Ugly!
            len = buflen - 1;
        }

        memmove(buf, str, (size_t)len);
    }

    if(buf == NULL)
    {
        GA_CHAR(len) = NUL;
    }
    else
    {
        buf[len] = NUL;
    }

    // Make each character lower case.
    i = 0;

    while(STR_CHAR(i) != NUL)
    {
        int c = utf_ptr2char(STR_PTR(i));
        int olen = utf_ptr2len(STR_PTR(i));
        int lc = mb_tolower(c);

        // Only replace the character when it is not an invalid
        // sequence (ASCII character or more than one byte) and
        // mb_tolower() doesn't return the original character.
        if(((c < 0x80) || (olen > 1)) && (c != lc))
        {
            int nlen = utf_char2len(lc);

            // If the byte length changes need to shift the following
            // characters forward or backward.
            if(olen != nlen)
            {
                if(nlen > olen)
                {
                    if(buf == NULL)
                    {
                        ga_grow(&ga, nlen - olen + 1);
                    }
                    else
                    {
                        if(len + nlen - olen >= buflen)
                        {
                            // out of memory, keep old char
                            lc = c;
                            nlen = olen;
                        }
                    }
                }

                if(olen != nlen)
                {
                    if(buf == NULL)
                    {
                        xstrmove(GA_PTR(i) + nlen, GA_PTR(i) + olen);
                        ga.ga_len += nlen - olen;
                    }
                    else
                    {
                        xstrmove(buf + i + nlen, buf + i + olen);
                        len += nlen - olen;
                    }
                }
            }

            (void)utf_char2bytes(lc, STR_PTR(i));
        }

        // skip to next multi-byte char
        i += (*mb_ptr2len)(STR_PTR(i));
    }

    if(buf == NULL)
    {
        return (uchar_kt *)ga.ga_data;
    }

    #undef GA_CHAR
    #undef GA_PTR
    #undef STR_CHAR
    #undef STR_PTR

    return buf;
}
