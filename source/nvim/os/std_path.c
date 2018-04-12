/// @file nvim/os/std_path.c

#include <stdbool.h>

#include "nvim/path.h"
#include "nvim/os/os.h"
#include "nvim/ascii.h"
#include "nvim/globals.h"
#include "nvim/func_attr.h"
#include "nvim/os/std_path.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "os/std_path.c.generated.h"
#endif

/// Return subpath of '$GKIDE_USR_HOME/auto'
///
/// @param[in]  sub_path        new component of the path
/// @param[in]  trail_pathsep   trailing path separator or not
/// @param[in]  escape_commas   if true, all commas will be escaped
///
/// @note
/// The finally path will be created if it is not exits.
///
/// @return
/// - '$GKIDE_USR_HOME/auto/{sub_path}', allocated string
/// - NULL, if the path not exist and can not created
char *stdpath_user_auto_subpath(const char *sub_path,
                                const bool trail_pathsep,
                                const bool escape_commas)
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_NONNULL_RET
{
    assert(NULL != sub_path);
    assert(NULL != gkide_usr_home);

    char *str = concat_fnames_realloc(xstrdup(gkide_usr_home), "auto", true);
    str = concat_fnames_realloc(str, sub_path, true);
    size_t len = strlen(str); // c-string length, no NUL

    if(escape_commas || trail_pathsep)
    {
        size_t add_len = 0; // number of char to add
        // number of commas to escape
        size_t num = (escape_commas ? std_memcnt(str, ',', len) : 0);

        add_len += trail_pathsep ? 1 : 0;
        add_len += escape_commas ? num : 0;

        str = xrealloc(str, len + add_len + 1); // add NUL
        memset(str + len, 0, add_len + 1); // init the new space
        len += add_len; // new string length

        for(size_t i = 0; escape_commas && i < len; i++)
        {
            if(str[i] == ',')
            {
                // Shift a byte backward
                memmove(str + i + 1, str + i, len - i);
                str[i] = '\\'; // add a backslash
                i++; // skip new add backslash, point to the commas
            }
        }

        if(trail_pathsep)
        {
            str[len-1] = OS_PATH_SEP_CHAR;
        }

        str[len] = NUL;
    }

    // check the path existence, if not, then create it.
    if(!os_path_exists((uchar_kt *)str))
    {
        char *failed_dir = NULL;

        if(os_mkdir_recurse(str, 0755, &failed_dir) != kLibuvSuccess)
        {
            return NULL;
        }
    }

    return str;
}
