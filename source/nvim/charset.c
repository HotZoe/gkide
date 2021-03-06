/// @file nvim/charset.c
///
/// Code related to character sets.

#include <assert.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h> // for towupper() and towlower()
#include <inttypes.h>

#include "nvim/nvim.h"
#include "nvim/ascii.h"
#include "nvim/charset.h"
#include "nvim/farsi.h"
#include "nvim/func_attr.h"
#include "nvim/indent.h"
#include "nvim/main.h"
#include "nvim/mark.h"
#include "nvim/mbyte.h"
#include "nvim/memline.h"
#include "nvim/memory.h"
#include "nvim/misc1.h"
#include "nvim/garray.h"
#include "nvim/move.h"
#include "nvim/option.h"
#include "nvim/os_unix.h"
#include "nvim/state.h"
#include "nvim/strings.h"
#include "nvim/path.h"
#include "nvim/utils.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "charset.c.generated.h"
#endif

// b_chartab[] is an array with 256 bits,
// each bit representing one of the characters 0-255.
#define SET_CHARTAB(buf, c) \
    (buf)->b_chartab[(unsigned)(c) >> 6] |= (1ull << ((c) & 0x3f))

#define RESET_CHARTAB(buf, c) \
    (buf)->b_chartab[(unsigned)(c) >> 6] &= ~(1ull << ((c) & 0x3f))

#define GET_CHARTAB_TAB(chartab, c) \
    ((chartab)[(unsigned)(c) >> 6] & (1ull << ((c) & 0x3f)))

#define GET_CHARTAB(buf, c) \
    GET_CHARTAB_TAB((buf)->b_chartab, c)

enum CharTableVarFlags
{
    /// ::g_chartab flags: mask, nr of display cells (1, 2 or 4)
    kCT_CellMask = 0x07,
     /// ::g_chartab flags: set for printable chars
    kCT_CharPrint= 0x10,
    /// ::g_chartab flags: set for ID chars
    kCT_CharID   = 0x20,
    /// ::g_chartab flags: set for file name chars
    kCT_CharFName= 0x40,
};

/// Table used below, see init_chartab() for an explanation
static uchar_kt g_chartab[256];
static bool chartab_initialized = false;

/// Fill ::g_chartab.
/// Also fills @b curbuf->b_chartab with flags
/// for keyword characters for current buffer.
///
/// Depends on the option settings:
/// - iskeyword
/// - isident
/// - isfname
/// - isprint
/// - encoding
///
/// The index in g_chartab[] is the character when first byte is up to 0x80,
/// if the first byte is 0x80 and above it depends on further bytes.
///
/// The contents of g_chartab[]:
/// - The lower two bits, masked by kCT_CellMask, give the number of display
///   cells the character occupies (1 or 2). Not valid for UTF-8 above 0x80.
///
/// - kCT_CharPrint bit is set when the character is printable (no need to
///   translate the character before displaying it). Note that only DBCS
///   characters can have 2 display cells and still be printable.
///
/// - kCT_CharFName bit is set when the character can be in a file name.
///
/// - kCT_CharID bit is set when the character can be in an identifier.
///
/// @return
/// FAIL if 'iskeyword', 'isident', 'isfname'
/// or 'isprint' option has an error, OK otherwise.
int init_chartab(void)
{
    return buf_init_chartab(curbuf, true);
}

/// Helper for ::init_chartab
///
/// @param global false: only set @b buf->b_chartab
///
/// @return
/// FAIL if 'iskeyword', 'isident', 'isfname'
/// or 'isprint' option has an error, OK otherwise.
int buf_init_chartab(filebuf_st *buf, int global)
{
    int c;

    if(global)
    {
        // Set the default size for printable characters:
        // - from <Space> to '~' is 1, printable
        // - others are 2, not printable
        // - inits all 'isident' and 'isfname' flags to false
        c = 0;

        // 0x00 - 0x1F, not printable
        while(c < ' ')
        {
            g_chartab[c++] = (dy_flags & DY_UHEX) ? 4 : 2;
        }

        // 0x20 - 0x7E, printable
        while(c <= '~')
        {
            g_chartab[c++] = 1 + kCT_CharPrint;
        }

        // 0x7F - 0xFF, None ASCII
        if(p_altkeymap)
        {
            while(c < YE)
            {
                g_chartab[c++] = 1 + kCT_CharPrint;
            }
        }

        // 0x7F - 0xFF, None ASCII
        while(c < 0x100)
        {
            if(c >= 0xa0)
            {
                // UTF-8: bytes 0xa0 - 0xff are printable (latin1)
                g_chartab[c++] = kCT_CharPrint + 1;
            }
            else
            {
                // the rest is unprintable by default
                g_chartab[c++] = (dy_flags & DY_UHEX) ? 4 : 2;
            }
        }

        // Assume that every multi-byte char is a filename character.
        for(c = 1; c < 0x100; c++)
        {
            if(c >= 0xa0)
            {
                g_chartab[c] |= kCT_CharFName;
            }
        }
    }

    // Init word char flags all to false
    memset(buf->b_chartab, 0, (size_t)32);

    // In lisp mode the '-' character is included in keywords.
    if(buf->b_p_lisp)
    {
        SET_CHARTAB(buf, '-');
    }

    // Walk through the 'isident', 'iskeyword', 'isfname'
    // and 'isprint' options.
    //
    // Each option is a list of characters, character numbers
    // or ranges, separated by commas, e.g.: "200-210,x,#-178,-"
    for(int i = global ? 0 : 3; i <= 3; i++)
    {
        const uchar_kt *p = NULL;

        switch(i)
        {
            case 0: // 'isident'
                p = p_isi;
                break;

            case 1: // 'isprint'
                p = p_isp;
                break;

            case 2: // 'isfname'
                p = p_isf;
                break;

            case 3: // 'iskeyword'
                p = buf->b_p_isk;
                break;

            default:
                break;
        }

        while(p && *p)
        {
            bool tilde = false;
            bool do_isalpha = false;

            if((*p == '^') && (p[1] != NUL))
            {
                tilde = true;
                ++p;
            }

            if(ascii_isdigit(*p))
            {
                c = getdigits_int((uchar_kt **)&p);
            }
            else
            {
                c = mb_ptr2char_adv(&p);
            }

            int c2 = -1;

            if((*p == '-') && (p[1] != NUL))
            {
                ++p;

                if(ascii_isdigit(*p))
                {
                    c2 = getdigits_int((uchar_kt **)&p);
                }
                else
                {
                    c2 = mb_ptr2char_adv(&p);
                }
            }

            if((c <= 0)
               || (c >= 256)
               || ((c2 < c) && (c2 != -1))
               || (c2 >= 256)
               || !((*p == NUL) || (*p == ',')))
            {
                return FAIL;
            }

            if(c2 == -1) // not a range
            {
                // A single '@' (not "@-@"):
                // Decide on letters being ID/printable/keyword chars with
                // standard function isalpha(). This takes care of locale for
                // single-byte characters).
                if(c == '@')
                {
                    do_isalpha = true;
                    c = 1;
                    c2 = 255;
                }
                else
                {
                    c2 = c;
                }
            }

            while(c <= c2)
            {
                // Use the MB_ functions here, because isalpha()
                // doesn't work properly when 'encoding' is "latin1"
                // and the locale is "C".
                if(!do_isalpha
                   || mb_islower(c)
                   || mb_isupper(c)
                   || (p_altkeymap && (F_isalpha(c) || F_isdigit(c))))
                {
                    switch(i)
                    {
                        case 0: // (re)set ID flag
                            if(tilde)
                            {
                                g_chartab[c] &= (uint8_t)~kCT_CharID;
                            }
                            else
                            {
                                g_chartab[c] |= kCT_CharID;
                            }

                            break;

                        case 1: // (re)set printable

                            // For double-byte we keep the cell width, so
                            // that we can detect it from the first byte.
                            if(((c < ' ')
                                || (c > '~')
                                || (p_altkeymap
                                    && (F_isalpha(c) || F_isdigit(c)))))
                            {
                                if(tilde)
                                {
                                    g_chartab[c] =
                                        (uint8_t)((g_chartab[c] & ~kCT_CellMask)
                                                  + ((dy_flags & DY_UHEX) ? 4:2));
                                    g_chartab[c] &= (uint8_t)~kCT_CharPrint;
                                }
                                else
                                {
                                    g_chartab[c] =
                                        (uint8_t)((g_chartab[c] & ~kCT_CellMask) + 1);
                                    g_chartab[c] |= kCT_CharPrint;
                                }
                            }

                            break;

                        case 2: // (re)set fname flag
                            if(tilde)
                            {
                                g_chartab[c] &= (uint8_t)~kCT_CharFName;
                            }
                            else
                            {
                                g_chartab[c] |= kCT_CharFName;
                            }

                            break;

                        case 3: // (re)set keyword flag
                            if(tilde)
                            {
                                RESET_CHARTAB(buf, c);
                            }
                            else
                            {
                                SET_CHARTAB(buf, c);
                            }

                            break;

                        default:
                            break;
                    }
                }

                ++c;
            }

            c = *p;
            p = skip_to_option_part(p);

            if((c == ',') && (*p == NUL))
            {
                // Trailing comma is not allowed.
                return FAIL;
            }
        }
    }

    chartab_initialized = true;
    return OK;
}

/// Translate any special characters in buf[bufsize] in-place.
///
/// The result is a string with only printable characters, but if
/// there is not enough room, not all characters will be translated.
///
/// @param buf
/// @param bufsize
void trans_characters(uchar_kt *buf, int bufsize)
{
    int len; // length of string needing translation
    int room; // room in buffer after string
    uchar_kt *trs; // translated character
    int trs_len; // length of trs[]
    len = (int)ustrlen(buf);
    room = bufsize - len;

    while(*buf != 0)
    {
        // Assume a multi-byte character doesn't need translation.
        if((trs_len = (*mb_ptr2len)(buf)) > 1)
        {
            len -= trs_len;
        }
        else
        {
            trs = transchar_byte(*buf);
            trs_len = (int)ustrlen(trs);

            if(trs_len > 1)
            {
                room -= trs_len - 1;

                if(room <= 0)
                {
                    return;
                }

                memmove(buf + trs_len, buf + 1, (size_t)len);
            }

            memmove(buf, trs, (size_t)trs_len);
            --len;
        }

        buf += trs_len;
    }
}

/// Translate a string into allocated memory,
/// replacing special chars with printable chars.
///
/// @param s
///
/// @return translated string
uchar_kt *transstr(uchar_kt *s)
FUNC_ATTR_NONNULL_RETURN
{
    int c;
    uchar_kt *res;
    uchar_kt *p;
    size_t l;
    uchar_kt hexbuf[11];

    // Compute the length of the result, taking
    // account of unprintable multi-byte characters.
    size_t len = 0;
    p = s;

    while(*p != NUL)
    {
        if((l = (size_t)(*mb_ptr2len)(p)) > 1)
        {
            c = (*mb_ptr2char)(p);
            p += l;

            if(is_print_char(c))
            {
                len += l;
            }
            else
            {
                transchar_hex(hexbuf, c);
                len += ustrlen(hexbuf);
            }
        }
        else
        {
            l = (size_t)byte2cells(*p++);

            if(l > 0)
            {
                len += l;
            }
            else
            {
                // illegal byte sequence
                len += 4;
            }
        }
    }

    res = xmallocz(len);

    *res = NUL;
    p = s;

    while(*p != NUL)
    {
        if((l = (size_t)(*mb_ptr2len)(p)) > 1)
        {
            c = (*mb_ptr2char)(p);

            if(is_print_char(c))
            {
                // append printable multi-byte char
                ustrncat(res, p, l);
            }
            else
            {
                transchar_hex(res + ustrlen(res), c);
            }

            p += l;
        }
        else
        {
            ustrcat(res, transchar_byte(*p++));
        }
    }

    return res;
}

/// Catch 22: g_chartab[] can't be initialized before the options are
/// initialized, and initializing options may cause transchar() to be called!
/// When chartab_initialized == false don't use g_chartab[].
/// Does NOT work for multi-byte characters, c must be <= 255.
/// Also doesn't work for the first byte of a multi-byte,
/// "c" must be a character!
static uchar_kt transchar_buf[7];

/// Translates a character
///
/// @param c
///
/// @return translated character.
uchar_kt *transchar(int c)
{
    int i = 0;

    if(IS_SPECIAL(c))
    {
        // special key code, display as ~@ char
        transchar_buf[0] = '~';
        transchar_buf[1] = '@';
        i = 2;
        c = K_SECOND(c);
    }

    if((!chartab_initialized
        && (((c >= ' ') && (c <= '~')) || (p_altkeymap && F_ischar(c))))
       || ((c < 256) && is_print_char_strict(c)))
    {
        // printable character
        transchar_buf[i] = (uchar_kt)c;
        transchar_buf[i + 1] = NUL;
    }
    else
    {
        transchar_nonprint(transchar_buf + i, c);
    }

    return transchar_buf;
}

/// Like transchar(), but called with a byte instead of a character.
/// Checks for an illegal UTF-8 byte.
///
/// @param c
///
/// @return pointer to translated character in transchar_buf.
uchar_kt *transchar_byte(int c)
{
    if(c >= 0x80)
    {
        transchar_nonprint(transchar_buf, c);
        return transchar_buf;
    }

    return transchar(c);
}

/// Convert non-printable character to two or more printable characters in
/// "buf[]".  "buf" needs to be able to hold five bytes.
/// Does NOT work for multi-byte characters, c must be <= 255.
///
/// @param buf
/// @param c
void transchar_nonprint(uchar_kt *buf, int c)
{
    if(c == NL)
    {
        // we use newline in place of a NUL
        c = NUL;
    }
    else if((c == CAR) && (get_fileformat(curbuf) == EOL_MAC))
    {
        // we use CR in place of  NL in this case
        c = NL;
    }

    if(dy_flags & DY_UHEX)
    {
        // 'display' has "uhex"
        transchar_hex(buf, c);
    }
    else if(c <= 0x7f)
    {
        // 0x00 - 0x1f and 0x7f
        buf[0] = '^';

        // DEL displayed as ^?
        buf[1] = (uchar_kt)(c ^ 0x40);
        buf[2] = NUL;
    }
    else
    {
        transchar_hex(buf, c);
    }
}

/// Convert a non-printable character to hex.
///
/// @param buf
/// @param c
void transchar_hex(uchar_kt *buf, int c)
{
    int i = 0;
    buf[0] = '<';

    if(c > 255)
    {
        buf[++i] = (uchar_kt)num_to_hex((unsigned)c >> 12);
        buf[++i] = (uchar_kt)num_to_hex((unsigned)c >> 8);
    }

    buf[++i] = (uchar_kt)(num_to_hex((unsigned)c >> 4));
    buf[++i] = (uchar_kt)(num_to_hex((unsigned)c));
    buf[++i] = '>';
    buf[++i] = NUL;
}

/// Return number of display cells occupied by byte "b".
///
/// Caller must make sure 0 <= b <= 255.
/// For multi-byte mode "b" must be the first byte of a character.
/// A TAB is counted as two cells: "^I".
/// This will return 0 for bytes >= 0x80, because the number of
/// cells depends on further bytes in UTF-8.
///
/// @param b
///
/// @reeturn Number of display cells.
int byte2cells(int b)
{
    if(b >= 0x80)
    {
        return 0;
    }

    return g_chartab[b] & kCT_CellMask;
}

/// Return number of display cells occupied by character "c".
///
/// "c" can be a special key (negative number) in which case 3 or
/// 4 is returned. A TAB is counted as two cells: "^I" or four: "<09>".
///
/// @param c
///
/// @return Number of display cells.
int char2cells(int c)
{
    if(IS_SPECIAL(c))
    {
        return char2cells(K_SECOND(c)) + 2;
    }

    if(c >= 0x80)
    {
        // UTF-8: above 0x80 need to check the value
        return utf_char2cells(c);
    }

    return g_chartab[c & 0xff] & kCT_CellMask;
}

/// Return number of display cells occupied by character at "*p".
/// A TAB is counted as two cells: "^I" or four: "<09>".
///
/// @param p
///
/// @return number of display cells.
int ptr2cells(const uchar_kt *p)
{
    // For UTF-8 we need to look at more bytes if the first byte is >= 0x80.
    if(*p >= 0x80)
    {
        return utf_ptr2cells(p);
    }

    // For DBCS we can tell the cell count from the first byte.
    return g_chartab[*p] & kCT_CellMask;
}

/// Return the number of characters 'c' will take on the screen, taking
/// into account the size of a tab.
/// Use a define to make it fast, this is used very often!!!
/// Also see getvcol() below.
///
/// @param p
/// @param col
///
/// @return Number of characters.
#define RET_WIN_BUF_CHARTABSIZE(wp, buf, p, col)     \
    if(*(p) == TAB                                   \
       && (!(wp)->w_o_curbuf.wo_list || lcs_tab1))   \
    {                                                \
        const int ts = (int) (buf)->b_p_ts;          \
        return (ts - (int)(col % ts));               \
    }                                                \
    else                                             \
    {                                                \
        return ptr2cells(p);                         \
    }

int chartabsize(uchar_kt *p, columnum_kt col)
{
    RET_WIN_BUF_CHARTABSIZE(curwin, curbuf, p, col)
}

static int win_chartabsize(win_st *wp, uchar_kt *p, columnum_kt col)
{
    RET_WIN_BUF_CHARTABSIZE(wp, wp->w_buffer, p, col)
}

/// Return the number of characters the string 's' will take on the screen,
/// taking into account the size of a tab.
///
/// @param s
///
/// @return Number of characters the string will take on the screen.
int linetabsize(uchar_kt *s)
{
    return linetabsize_col(0, s);
}

/// Like linetabsize(), but starting at column "startcol".
///
/// @param startcol
/// @param s
///
/// @return Number of characters the string will take on the screen.
int linetabsize_col(int startcol, uchar_kt *s)
{
    columnum_kt col = startcol;
    uchar_kt *line = s; // pointer to start of line, for breakindent

    while(*s != NUL)
    {
        col += lbr_chartabsize_adv(line, &s, col);
    }

    return (int)col;
}

/// Like linetabsize(), but for a given window instead of the current one.
///
/// @param wp
/// @param line
/// @param len
///
/// @return Number of characters the string will take on the screen.
unsigned int win_linetabsize(win_st *wp, uchar_kt *line, columnum_kt len)
{
    columnum_kt col = 0;

    for(uchar_kt *s = line;
        *s != NUL && (len == MAXCOL || s < line + len);
        mb_ptr_adv(s))
    {
        col += win_lbr_chartabsize(wp, line, s, col, NULL);
    }

    return (unsigned int)col;
}

/// Check that @b c is a normal identifier character:
/// Letters and characters from the @b isident option.
///
/// @param  c  character to check
bool is_id_char(int c)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return c > 0 && c < 0x100 && (g_chartab[c] & kCT_CharID);
}

/// Check that @b c is a keyword character:
/// Letters and characters from 'iskeyword' option for current buffer.
/// For multi-byte characters mb_get_class() is used (builtin rules).
///
/// @param  c  character to check
bool is_kwc(int c)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return is_kwc_buf(c, curbuf);
}

/// Check that "c" is a keyword character
/// Letters and characters from 'iskeyword' option for given buffer.
/// For multi-byte characters mb_get_class() is used (builtin rules).
///
/// @param[in]  c  Character to check.
/// @param[in]  chartab  Buffer chartab.
bool is_kwc_tab(const int c, const uint64_t *const chartab)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return (c >= 0x100
            ? (utf_class(c) >= 2)
            : (c > 0 && GET_CHARTAB_TAB(chartab, c) != 0));
}

/// Check that "c" is a keyword character:
/// Letters and characters from 'iskeyword' option for given buffer.
/// For multi-byte characters mb_get_class() is used (builtin rules).
///
/// @param  c    character to check
/// @param  buf  buffer whose keywords to use
bool is_kwc_buf(int c, filebuf_st *buf)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ARG(2)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return is_kwc_tab(c, buf->b_chartab);
}

/// Just like is_kwc() but uses a pointer to the (multi-byte) character.
///
/// @param  p  pointer to the multi-byte character
///
/// @return true if "p" points to a keyword character.
bool is_kwc_ptr(uchar_kt *p)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(MB_BYTE2LEN(*p) > 1)
    {
        return mb_get_class(p) >= 2;
    }

    return GET_CHARTAB(curbuf, *p) != 0;
}

/// Just like is_kwc_buf() but uses a pointer to the (multi-byte)
/// character.
///
/// @param  p    pointer to the multi-byte character
/// @param  buf  buffer whose keywords to use
///
/// @return true if "p" points to a keyword character.
bool is_kwc_ptr_buf(uchar_kt *p, filebuf_st *buf)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(MB_BYTE2LEN(*p) > 1)
    {
        return mb_get_class(p) >= 2;
    }

    return GET_CHARTAB(buf, *p) != 0;
}

/// Check that @b c is a valid file-name character.
/// Assume characters above 0x100 are valid (multi-byte).
///
/// @param  c  character to check
bool is_file_name_char(int c)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    return c >= 0x100 || (c > 0 && (g_chartab[c] & kCT_CharFName));
}

/// Check that "c" is a valid file-name character or a wildcard character
/// Assume characters above 0x100 are valid (multi-byte).
/// Explicitly interpret ']' as a wildcard character as path_has_wildcard("]")
/// returns false.
///
/// @param  c  character to check
bool is_file_name_char_or_wildcard(int c)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    uchar_kt buf[2];
    buf[0] = (uchar_kt)c;
    buf[1] = NUL;
    return is_file_name_char(c) || c == ']' || path_has_wildcard(buf);
}

/// Check that "c" is a printable character.
/// Assume characters above 0x100 are printable for double-byte encodings.
///
/// @param  c  character to check
bool is_print_char(int c)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(c >= 0x100)
    {
        return utf_printable(c);
    }

    return c >= 0x100 || (c > 0 && (g_chartab[c] & kCT_CharPrint));
}

/// Strict version of is_print_char(c), don't return true if "c" is the head
/// byte of a double-byte character.
///
/// @param  c  character to check
///
/// @return true if "c" is a printable character.
bool is_print_char_strict(int c)
FUNC_ATTR_PURE
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(c >= 0x100)
    {
        return utf_printable(c);
    }

    return c > 0 && (g_chartab[c] & kCT_CharPrint);
}

/// like chartabsize(), but also check for line breaks on the screen
///
/// @param line
/// @param s
/// @param col
///
/// @return The number of characters taken up on the screen.
int lbr_chartabsize(uchar_kt *line, unsigned char *s, columnum_kt col)
{
    if(!curwin->w_o_curbuf.wo_lbr
       && (*p_sbr == NUL)
       && !curwin->w_o_curbuf.wo_bri)
    {
        if(curwin->w_o_curbuf.wo_wrap)
        {
            return win_nolbr_chartabsize(curwin, s, col, NULL);
        }

        RET_WIN_BUF_CHARTABSIZE(curwin, curbuf, s, col)
    }

    return win_lbr_chartabsize(curwin, line == NULL ? s: line, s, col, NULL);
}

/// Call lbr_chartabsize() and advance the pointer.
///
/// @param line
/// @param s
/// @param col
///
/// @return The number of characters take up on the screen.
int lbr_chartabsize_adv(uchar_kt *line, uchar_kt **s, columnum_kt col)
{
    int retval;
    retval = lbr_chartabsize(line, *s, col);
    mb_ptr_adv(*s);
    return retval;
}

/// This function is used very often, keep it fast!!!!
///
/// If "headp" not NULL, set *headp to the size of what we for 'showbreak'
/// string at start of line.  Warning: *headp is only set if it's a non-zero
/// value, init to 0 before calling.
///
/// @param wp
/// @param line
/// @param s
/// @param col
/// @param headp
///
/// @return The number of characters taken up on the screen.
int win_lbr_chartabsize(win_st *wp,
                        uchar_kt *line,
                        uchar_kt *s,
                        columnum_kt col,
                        int *headp)
{
    int n;
    int added;
    uchar_kt *ps;
    columnum_kt col2;
    columnum_kt colmax;
    int numberextra;
    int mb_added = 0;
    columnum_kt col_adj = 0; // col + screen size of tab

    // No 'linebreak', 'showbreak' and 'breakindent': return quickly.
    if(!wp->w_o_curbuf.wo_lbr
        && !wp->w_o_curbuf.wo_bri
        && (*p_sbr == NUL))
    {
        if(wp->w_o_curbuf.wo_wrap)
        {
            return win_nolbr_chartabsize(wp, s, col, headp);
        }

        RET_WIN_BUF_CHARTABSIZE(wp, wp->w_buffer, s, col)
    }

    // First get normal size, without 'linebreak'
    int size = win_chartabsize(wp, s, col);
    int c = *s;

    if(*s == TAB)
    {
        col_adj = size - 1;
    }

    // If 'linebreak' set check at a blank before a non-blank if the line
    // needs a break here
    if(wp->w_o_curbuf.wo_lbr
       && vim_isbreak(c)
       && !vim_isbreak(s[1])
       && wp->w_o_curbuf.wo_wrap
       && (wp->w_width != 0))
    {
        // Count all characters from first non-blank after a blank up to next
        // non-blank after a blank.
        numberextra = win_col_off(wp);
        col2 = col;
        colmax = (columnum_kt)(wp->w_width - numberextra - col_adj);

        if(col >= colmax)
        {
            colmax += col_adj;
            n = colmax + win_col_off2(wp);

            if(n > 0)
            {
                colmax += (((col - colmax) / n) + 1) * n - col_adj;
            }
        }

        for(;;)
        {
            ps = s;
            mb_ptr_adv(s);
            c = *s;

            if(!((c != NUL)
                 && (vim_isbreak(c)
                     || (!vim_isbreak(c) && ((col2 == col)
                     || !vim_isbreak(*ps))))))
            {
                break;
            }

            col2 += win_chartabsize(wp, s, col2);

            if(col2 >= colmax)
            {
                // doesn't fit
                size = colmax - col + col_adj;
                break;
            }
        }
    }
    else if((size == 2)
            && (MB_BYTE2LEN(*s) > 1)
            && wp->w_o_curbuf.wo_wrap
            && in_win_border(wp, col))
    {
        // Count the ">" in the last column.
        ++size;
        mb_added = 1;
    }

    // May have to add something for 'breakindent' and/or 'showbreak'
    // string at start of line.
    // Set *headp to the size of what we add.
    added = 0;

    if((*p_sbr != NUL || wp->w_o_curbuf.wo_bri)
       && wp->w_o_curbuf.wo_wrap
       && (col != 0))
    {
        columnum_kt sbrlen = 0;
        int numberwidth = win_col_off(wp);
        numberextra = numberwidth;
        col += numberextra + mb_added;

        if(col >= (columnum_kt)wp->w_width)
        {
            col -= wp->w_width;
            numberextra = wp->w_width - (numberextra - win_col_off2(wp));

            if(col >= numberextra && numberextra > 0)
            {
                col %= numberextra;
            }

            if(*p_sbr != NUL)
            {
                sbrlen = (columnum_kt)mb_charlen(p_sbr);

                if(col >= sbrlen)
                {
                    col -= sbrlen;
                }
            }

            if(col >= numberextra && numberextra > 0)
            {
                col %= numberextra;
            }
            else if(col > 0 && numberextra > 0)
            {
                col += numberwidth - win_col_off2(wp);
            }

            numberwidth -= win_col_off2(wp);
        }

        if(col == 0 || (col + size + sbrlen > (columnum_kt)wp->w_width))
        {
            added = 0;

            if(*p_sbr != NUL)
            {
                if(size + sbrlen + numberwidth > (columnum_kt)wp->w_width)
                {
                    // Calculate effective window width.
                    int width = (columnum_kt)wp->w_width - sbrlen - numberwidth;

                    int prev_width =
                        col ? ((columnum_kt)wp->w_width - (sbrlen + col)) : 0;

                    if(width == 0)
                    {
                        width = (columnum_kt)wp->w_width;
                    }

                    added += ((size - prev_width) / width) * ustr_scrsize(p_sbr);

                    if((size - prev_width) % width)
                    {
                        // Wrapped, add another length of 'sbr'.
                        added += ustr_scrsize(p_sbr);
                    }
                }
                else
                {
                    added += ustr_scrsize(p_sbr);
                }
            }

            if(wp->w_o_curbuf.wo_bri)
            {
                added += get_breakindent_win(wp, line);
            }

            size += added;

            if(col != 0)
            {
                added = 0;
            }
        }
    }

    if(headp != NULL)
    {
        *headp = added + mb_added;
    }

    return size;
}

/// Like win_lbr_chartabsize(), except that we know 'linebreak' is off and
/// 'wrap' is on. This means we need to check for a double-byte character
/// that doesn't fit at the end of the screen line.
///
/// @param wp
/// @param s
/// @param col
/// @param headp
///
/// @return The number of characters take up on the screen.
static int win_nolbr_chartabsize(win_st *wp,
                                 uchar_kt *s,
                                 columnum_kt col,
                                 int *headp)
{
    int n;

    if((*s == TAB) && (!wp->w_o_curbuf.wo_list || lcs_tab1))
    {
        n = (int)wp->w_buffer->b_p_ts;
        return n - (col % n);
    }

    n = ptr2cells(s);

    // Add one cell for a double-width character in the last column of the
    // window, displayed with a ">".
    if((n == 2) && (MB_BYTE2LEN(*s) > 1) && in_win_border(wp, col))
    {
        if(headp != NULL)
        {
            *headp = 1;
        }

        return 3;
    }

    return n;
}

/// Check that virtual column "vcol" is in the rightmost column of window "wp".
///
/// @param  wp    window
/// @param  vcol  column number
bool in_win_border(win_st *wp, columnum_kt vcol)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ARG(1)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    int width1; // width of first line (after line number)
    int width2; // width of further lines

    if(wp->w_width == 0)
    {
        // there is no border
        return false;
    }

    width1 = wp->w_width - win_col_off(wp);

    if((int)vcol < width1 - 1)
    {
        return false;
    }

    if((int)vcol == width1 - 1)
    {
        return true;
    }

    width2 = width1 + win_col_off2(wp);

    if(width2 <= 0)
    {
        return false;
    }

    return (vcol - width1) % width2 == width2 - 1;
}

/// Get virtual column number of pos.
/// - start: on the first position of this character (TAB, ctrl)
/// - cursor: where the cursor is on this character (first char, except for TAB)
/// - end: on the last position of this character (TAB, ctrl)
///
/// This is used very often, keep it fast!
///
/// @param wp
/// @param pos
/// @param start
/// @param cursor
/// @param end
void getvcol(win_st *wp,
             apos_st *pos,
             columnum_kt *start,
             columnum_kt *cursor,
             columnum_kt *end)
{
    columnum_kt vcol;
    uchar_kt *ptr; // points to current char
    uchar_kt *posptr; // points to char at pos->col
    uchar_kt *line; // start of the line
    int incr;
    int head;
    int ts = (int)wp->w_buffer->b_p_ts;
    int c;
    vcol = 0;
    line = ptr = ml_get_buf(wp->w_buffer, pos->lnum, false);

    if(pos->col == MAXCOL)
    {
        // continue until the NUL
        posptr = NULL;
    }
    else
    {
        // Special check for an empty line, which can happen on exit, when
        // ml_get_buf() always returns an empty string.
        if(*ptr == NUL)
        {
            pos->col = 0;
        }

        posptr = ptr + pos->col;
        posptr -= utf_head_off(line, posptr);
    }

    // This function is used very often, do some speed optimizations.
    // When 'list', 'linebreak', 'showbreak' and 'breakindent' are not set
    // use a simple loop.
    // Also use this when 'list' is set but tabs take their normal size.
    if((!wp->w_o_curbuf.wo_list || (lcs_tab1 != NUL))
       && !wp->w_o_curbuf.wo_lbr
       && (*p_sbr == NUL)
       && !wp->w_o_curbuf.wo_bri)
    {
        for(;;)
        {
            head = 0;
            c = *ptr;

            // make sure we don't go past the end of the line
            if(c == NUL)
            {
                // NUL at end of line only takes one column
                incr = 1;
                break;
            }

            // A tab gets expanded, depending on the current column
            if(c == TAB)
            {
                incr = ts - (vcol % ts);
            }
            else
            {
                // For utf-8, if the byte is >= 0x80, need to look at
                // further bytes to find the cell width.
                if(c >= 0x80)
                {
                    incr = utf_ptr2cells(ptr);
                }
                else
                {
                    incr = g_chartab[c] & kCT_CellMask;
                }

                // If a double-cell char doesn't fit at the end of a line
                // it wraps to the next line, it's like this char is three
                // cells wide.
                if((incr == 2)
                   && wp->w_o_curbuf.wo_wrap
                   && (MB_BYTE2LEN(*ptr) > 1)
                   && in_win_border(wp, vcol))
                {
                    incr++;
                    head = 1;
                }
            }

            if((posptr != NULL) && (ptr >= posptr))
            {
                // character at pos->col
                break;
            }

            vcol += incr;
            mb_ptr_adv(ptr);
        }
    }
    else
    {
        for(;;)
        {
            // A tab gets expanded, depending on the current column
            head = 0;
            incr = win_lbr_chartabsize(wp, line, ptr, vcol, &head);

            // make sure we don't go past the end of the line
            if(*ptr == NUL)
            {
                // NUL at end of line only takes one column
                incr = 1;
                break;
            }

            if((posptr != NULL) && (ptr >= posptr))
            {
                // character at pos->col
                break;
            }

            vcol += incr;
            mb_ptr_adv(ptr);
        }
    }

    if(start != NULL)
    {
        *start = vcol + head;
    }

    if(end != NULL)
    {
        *end = vcol + incr - 1;
    }

    if(cursor != NULL)
    {
        if((*ptr == TAB)
           && (curmod & kNormalMode)
           && !wp->w_o_curbuf.wo_list
           && !virtual_active()
           && !(VIsual_active && ((*p_sel == 'e') || ltoreq(*pos, VIsual))))
        {
            // cursor at end
            *cursor = vcol + incr - 1;
        }
        else
        {
            // cursor at start
            *cursor = vcol + head;
        }
    }
}

/// Get virtual cursor column in the current window, pretending 'list' is off.
///
/// @param posp
///
/// @retujrn The virtual cursor column.
columnum_kt getvcol_nolist(apos_st *posp)
{
    int list_save = curwin->w_o_curbuf.wo_list;
    columnum_kt vcol;

    curwin->w_o_curbuf.wo_list = false;
    getvcol(curwin, posp, NULL, &vcol, NULL);
    curwin->w_o_curbuf.wo_list = list_save;

    return vcol;
}

/// Get virtual column in virtual mode.
///
/// @param wp
/// @param pos
/// @param start
/// @param cursor
/// @param end
void getvvcol(win_st *wp,
              apos_st *pos,
              columnum_kt *start,
              columnum_kt *cursor,
              columnum_kt *end)
{
    columnum_kt col;
    columnum_kt coladd;
    columnum_kt endadd;
    uchar_kt *ptr;

    if(virtual_active())
    {
        // For virtual mode, only want one value
        getvcol(wp, pos, &col, NULL, NULL);
        coladd = pos->coladd;
        endadd = 0;

        // Cannot put the cursor on part of a wide character.
        ptr = ml_get_buf(wp->w_buffer, pos->lnum, false);

        if(pos->col < (columnum_kt)ustrlen(ptr))
        {
            int c = (*mb_ptr2char)(ptr + pos->col);

            if((c != TAB) && is_print_char(c))
            {
                endadd = (columnum_kt)(char2cells(c) - 1);

                if(coladd > endadd)
                {
                    // past end of line
                    endadd = 0;
                }
                else
                {
                    coladd = 0;
                }
            }
        }

        col += coladd;

        if(start != NULL)
        {
            *start = col;
        }

        if(cursor != NULL)
        {
            *cursor = col;
        }

        if(end != NULL)
        {
            *end = col + endadd;
        }
    }
    else
    {
        getvcol(wp, pos, start, cursor, end);
    }
}

/// Get the leftmost and rightmost virtual column of pos1 and pos2.
/// Used for Visual block mode.
///
/// @param wp
/// @param pos1
/// @param pos2
/// @param left
/// @param right
void getvcols(win_st *wp,
              apos_st *pos1,
              apos_st *pos2,
              columnum_kt *left,
              columnum_kt *right)
{
    columnum_kt from1;
    columnum_kt from2;
    columnum_kt to1;
    columnum_kt to2;

    if(lt(*pos1, *pos2))
    {
        getvvcol(wp, pos1, &from1, NULL, &to1);
        getvvcol(wp, pos2, &from2, NULL, &to2);
    }
    else
    {
        getvvcol(wp, pos2, &from1, NULL, &to1);
        getvvcol(wp, pos1, &from2, NULL, &to2);
    }

    if(from2 < from1)
    {
        *left = from2;
    }
    else
    {
        *left = from1;
    }

    if(to2 > to1)
    {
        if((*p_sel == 'e') && (from2 - 1 >= to1))
        {
            *right = from2 - 1;
        }
        else
        {
            *right = to2;
        }
    }
    else
    {
        *right = to1;
    }
}

/// skipwhite: skip over ' ' and **\t**
///
/// @param[in]  q  String to skip in.
///
/// @return Pointer to character after the skipped whitespace.
uchar_kt *skipwhite(const uchar_kt *q)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_WARN_UNUSED_RESULT
{
    const uchar_kt *p = q;

    while(ascii_iswhite(*p))
    {
        p++;
    }

    return (uchar_kt *)p;
}

/// Skip over digits
///
/// @param[in]  q  String to skip digits in.
///
/// @return Pointer to the character after the skipped digits.
uchar_kt *skipdigits(const uchar_kt *q)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
FUNC_ATTR_WARN_UNUSED_RESULT
{
    const uchar_kt *p = q;

    while(ascii_isdigit(*p))
    {
        // skip to next non-digit
        p++;
    }

    return (uchar_kt *)p;
}

/// skip over binary digits
///
/// @param q pointer to string
///
/// @return Pointer to the character after the skipped digits.
const char *skipbin(const char *q)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    const char *p = q;

    while(ascii_isbdigit(*p))
    {
        // skip to next non-digit
        p++;
    }

    return p;
}

/// skip over digits and hex characters
///
/// @param q
///
/// @return
/// Pointer to the character after the skipped digits and hex characters.
uchar_kt *skiphex(uchar_kt *q)
{
    uchar_kt *p = q;

    while(ascii_isxdigit(*p))
    {
        // skip to next non-digit
        p++;
    }

    return p;
}

/// skip to digit (or NUL after the string)
///
/// @param q
///
/// @return Pointer to the digit or (NUL after the string).
uchar_kt *skiptodigit(uchar_kt *q)
{
    uchar_kt *p = q;

    while(*p != NUL && !ascii_isdigit(*p))
    {
        // skip to next digit
        p++;
    }

    return p;
}

/// skip to binary character (or NUL after the string)
///
/// @param q pointer to string
///
/// @return Pointer to the binary character or (NUL after the string).
const char *skiptobin(const char *q)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RETURN
{
    const char *p = q;

    while(*p != NUL && !ascii_isbdigit(*p))
    {
        // skip to next digit
        p++;
    }

    return p;
}

/// skip to hex character (or NUL after the string)
///
/// @param q
///
/// @return
/// Pointer to the hex character or (NUL after the string).
uchar_kt *skiptohex(uchar_kt *q)
{
    uchar_kt *p = q;

    while(*p != NUL && !ascii_isxdigit(*p))
    {
        // skip to next digit
        p++;
    }

    return p;
}

/// Skip over text until ' ' or '\t' or NUL
///
/// @param[in]  p  Text to skip over.
///
/// @return
/// Pointer to the next whitespace or NUL character.
uchar_kt *skiptowhite(const uchar_kt *p)
{
    while(*p != ' ' && *p != '\t' && *p != NUL)
    {
        p++;
    }

    return (uchar_kt *)p;
}

/// skiptowhite_esc: Like skiptowhite(), but also skip escaped chars
///
/// @param p
///
/// @return Pointer to the next whitespace character.
uchar_kt *skiptowhite_esc(uchar_kt *p)
{
    while(*p != ' ' && *p != '\t' && *p != NUL)
    {
        if(((*p == '\\') || (*p == Ctrl_V)) && (*(p + 1) != NUL))
        {
            ++p;
        }

        ++p;
    }

    return p;
}

/// Get a number from a string and skip over it, signalling overflows
///
/// @param[out]  pp  A pointer to a pointer to uchar_kt.
///                  It will be advanced past the read number.
/// @param[out]  nr  Number read from the string.
///
/// @return OK on success, FAIL on error/overflow
int getdigits_safe(uchar_kt **pp, intmax_t *nr)
{
    errno = 0;
    *nr = strtoimax((char *)(*pp), (char **)pp, 10);

    if((*nr == INTMAX_MIN || *nr == INTMAX_MAX) && errno == ERANGE)
    {
        return FAIL;
    }

    return OK;
}

/// Get a number from a string and skip over it.
///
/// @param[out]  pp  A pointer to a pointer to uchar_kt.
///                  It will be advanced past the read number.
///
/// @return Number read from the string.
intmax_t getdigits(uchar_kt **pp)
{
    intmax_t number;
    int ret = getdigits_safe(pp, &number);
    (void)ret; // Avoid "unused variable" warning in Release build
    assert(ret == OK);
    return number;
}

/// Get an int number from a string. Like getdigits(), but restricted to `int`.
int getdigits_int(uchar_kt **pp)
{
    intmax_t number = getdigits(pp);

#if HOST_SIZEOF_INTMAX_T > HOST_SIZEOF_INT
    assert(number >= INT_MIN && number <= INT_MAX);
#endif

    return (int)number;
}

/// Get a long number from a string.
/// Like getdigits(), but restricted to `long`.
long getdigits_long(uchar_kt **pp)
{
    intmax_t number = getdigits(pp);

#if HOST_SIZEOF_INTMAX_T > HOST_SIZEOF_LONG
    assert(number >= LONG_MIN && number <= LONG_MAX);
#endif

    return (long)number;
}

/// Check that "lbuf" is empty or only contains blanks.
///
/// @param  lbuf  line buffer to check
bool is_blank_line(uchar_kt *lbuf)
{
    uchar_kt *p = skipwhite(lbuf);
    return *p == NUL || *p == '\r' || *p == '\n';
}

/// Check that "str" starts with a backslash that should be removed.
/// For Windows this is only done when the character after the
/// backslash is not a normal file name character.
/// '$' is a valid file name character, we don't remove the backslash before
/// it.  This means it is not possible to use an environment variable after a
/// backslash.  "C:\$VIM\doc" is taken literally, only "$VIM\doc" works.
/// Although "\ name" is valid, the backslash in "Program\ files" must be
/// removed.  Assume a file name doesn't start with a space.
/// For multi-byte names, never remove a backslash before a non-ascii
/// character, assume that all multi-byte characters are valid file name
/// characters.
///
/// @param  str  file path string to check
bool rem_backslash(const uchar_kt *str)
FUNC_ATTR_PURE
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
#ifdef BACKSLASH_IN_FILENAME
    return str[0] == '\\'
           && str[1] < 0x80
           && (str[1] == ' '
               || (str[1] != NUL
                   && str[1] != '*'
                   && str[1] != '?'
                   && !is_file_name_char(str[1])));
#else
    return str[0] == '\\' && str[1] != NUL;
#endif
}

/// Halve the number of backslashes in a file name argument.
///
/// @param p
void backslash_halve(uchar_kt *p)
{
    for(; *p; ++p)
    {
        if(rem_backslash(p))
        {
            xstrmove(p, p + 1);
        }
    }
}

/// backslash_halve() plus save the result in allocated memory.
///
/// @param p
///
/// @return String with the number of backslashes halved.
uchar_kt *backslash_halve_save(uchar_kt *p)
{
    // TODO(philix): simplify and improve backslash_halve_save algorithm
    uchar_kt *res = ustrdup(p);
    backslash_halve(res);
    return res;
}
