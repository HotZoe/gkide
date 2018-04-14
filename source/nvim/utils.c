/// @file nvim/utils.c

#include "nvim/types.h"
#include "nvim/ascii.h"
#include "nvim/utils.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "utils.c.generated.h"
#endif

/// Writes @b time_var to @b buf[8].
void time_to_bytes(time_t time_var, uint8_t buf[8])
{
    // time_t can be up to 8 bytes in size, more than uintmax_t
    // in 32 bits systems, thus we can't use put_bytes() here.
    for(size_t i = 7, bufi = 0; bufi < 8; i--, bufi++)
    {
        buf[bufi] = (uint8_t)((uint64_t)time_var >> (i * 8));
    }
}

/// convert number to ascii string
///
/// @param[in]  num     the number to convert
/// @param[in]  radix   the base for output, bigger then zero
/// @param[i/o] string  the buffer to write, NUL ending
///
/// @note
/// - the @b buf must have enough space for the output string.
/// - ignore all signed/unsigned, all treat as unsigned number
///
/// @return
/// if success, return the @b buf; otherwise, return NULL
char *num_to_str(long num, int radix, char *string)
{
    if(NULL == string || radix <= 0)
    {
        return NULL;
    }

    char index[] = "0123456789ABCDEF";

    if(num < 0)
    {
        num = -num;
    }

    // convert to reversed order
    int i = 0;
    do
    {
        string[i++] = index[num%radix];
        num /= radix;
    } while(num);

    string[i] = NUL;

    char swap;
    for(int j=0; j<=(i-1)/2; j++)
    {
        swap = string[j];
        string[j] = string[i-1-j];
        string[i-1-j] = swap;
    }

    return string;
}

/// Convert a string into a long and/or unsigned long, taking care of
/// hexadecimal, octal and binary numbers.  Accepts a '-' sign.
/// If "prep" is not NULL, returns a flag to indicate the type of the number:
///  -  0     decimal
///  - '0'    octal
///  - 'B'    bin
///  - 'b'    bin
///  - 'X'    hex
///  - 'x'    hex
///
/// - If "len" is not NULL, the length of the number in characters is returned.
/// - If "nptr" is not NULL, the signed result is returned in it.
/// - If "unptr" is not NULL, the unsigned result is returned in it.
/// - If "what" contains kStrToNumBin recognize binary numbers.
/// - If "what" contains kStrToNumOct recognize octal numbers.
/// - If "what" contains kStrToNumHex recognize hex numbers.
/// - If "what" contains kStrToNumOne always assume bin/oct/hex.
/// - If maxlen > 0, check at a maximum maxlen chars.
///
/// @param start
/// @param prep   Returns type of number 0 = decimal, 'x' or 'X' is hex,
///               '0' = octal, 'b' or 'B' is bin
/// @param len    Returns the detected length of number.
/// @param what   Recognizes what number passed.
/// @param nptr   Returns the signed result.
/// @param unptr  Returns the unsigned result.
/// @param maxlen Max length of string to check.
void str_to_num(const uchar_kt *const start,
                int *const prep,
                int *const len,
                const int what,
                long *const nptr,
                unsigned long *const unptr,
                const int maxlen)
{
    const uchar_kt *ptr = start;
    int pre = 0; // default is decimal
    bool negative = false;
    unsigned long un = 0;

    if(ptr[0] == '-')
    {
        negative = true;
        ptr++;
    }

    // Recognize hex, octal and bin.
    if((ptr[0] == '0')
        && (ptr[1] != '8')
        && (ptr[1] != '9')
        && (maxlen == 0 || maxlen > 1))
    {
        pre = ptr[1];

        if((what & kStrToNumHex)
           && ((pre == 'X') || (pre == 'x'))
           && ascii_isxdigit(ptr[2])
           && (maxlen == 0 || maxlen > 2))
        {
            // hexadecimal
            ptr += 2;
        }
        else if((what & kStrToNumBin)
                && ((pre == 'B') || (pre == 'b'))
                && ascii_isbdigit(ptr[2])
                && (maxlen == 0 || maxlen > 2))
        {
            // binary
            ptr += 2;
        }
        else
        {
            // decimal or octal, default is decimal
            pre = 0;

            if(what & kStrToNumOct)
            {
                // Don't interpret "0", "08" or "0129" as octal.
                for(int n = 1; ascii_isdigit(ptr[n]); ++n)
                {
                    if(ptr[n] > '7')
                    {
                        // can't be octal
                        pre = 0;
                        break;
                    }

                    if(ptr[n] >= '0')
                    {
                        // assume octal
                        pre = '0';
                    }

                    if(n == maxlen)
                    {
                        break;
                    }
                }
            }
        }
    }

    // Do the string-to-numeric conversion "manually" to avoid sscanf quirks.
    int n = 1;

    if((pre == 'B') || (pre == 'b') || what == kStrToNumBin + kStrToNumOne)
    {
        // bin
        if(pre != 0)
        {
            n += 2; // skip over "0b"
        }

        while('0' <= *ptr && *ptr <= '1')
        {
            un = 2 * un + (unsigned long)(*ptr - '0');
            ptr++;

            if(n++ == maxlen)
            {
                break;
            }
        }
    }
    else if((pre == '0') || what == kStrToNumOct + kStrToNumOne)
    {
        // octal
        while('0' <= *ptr && *ptr <= '7')
        {
            un = 8 * un + (unsigned long)(*ptr - '0');
            ptr++;

            if(n++ == maxlen)
            {
                break;
            }
        }
    }
    else if((pre == 'X') || (pre == 'x') || what == kStrToNumHex + kStrToNumOne)
    {
        // hex
        if(pre != 0)
        {
            n += 2; // skip over "0x"
        }

        while(ascii_isxdigit(*ptr))
        {
            un = 16 * un + (unsigned long)hex_to_num(*ptr);
            ptr++;

            if(n++ == maxlen)
            {
                break;
            }
        }
    }
    else
    {
        // decimal
        while(ascii_isdigit(*ptr))
        {
            un = 10 * un + (unsigned long)(*ptr - '0');
            ptr++;

            if(n++ == maxlen)
            {
                break;
            }
        }
    }

    if(prep != NULL)
    {
        *prep = pre;
    }

    if(len != NULL)
    {
        *len = (int)(ptr - start);
    }

    if(nptr != NULL)
    {
        if(negative)
        {
            // account for leading '-' for decimal numbers
            *nptr = -(long)un;
        }
        else
        {
            *nptr = (long)un;
        }
    }

    if(unptr != NULL)
    {
        *unptr = un;
    }
}

/// Return the value of a single hex character.
/// Only valid when the argument is '0' - '9', 'A' - 'F' or 'a' - 'f'.
///
/// @param c
///
/// @return The value of the hex character.
int hex_to_num(int c)
{
    if((c >= 'a') && (c <= 'f'))
    {
        return c - 'a' + 10;
    }

    if((c >= 'A') && (c <= 'F'))
    {
        return c - 'A' + 10;
    }

    return c - '0';
}

/// Convert the lower 4 bits of byte "c" to its hex character.
/// Lower case letters are used to avoid the confusion of <F1>
/// being 0xf1 or function key 1.
///
/// @param c
///
/// @return the hex character.
unsigned num_to_hex(unsigned c)
{
    if((c & 0xf) <= 9)
    {
        return (c & 0xf) + '0';
    }

    return (c & 0xf) - 10 + 'a';
}
