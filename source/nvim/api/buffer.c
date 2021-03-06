/// @file nvim/api/buffer.c

// Much of this code was adapted from
// 'if_py_both.h' from the original vim source
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include "nvim/api/buffer.h"
#include "nvim/api/private/helpers.h"
#include "nvim/api/private/defs.h"
#include "nvim/nvim.h"
#include "nvim/buffer.h"
#include "nvim/cursor.h"
#include "nvim/memline.h"
#include "nvim/memory.h"
#include "nvim/misc1.h"
#include "nvim/ex_cmds.h"
#include "nvim/mark.h"
#include "nvim/fileio.h"
#include "nvim/move.h"
#include "nvim/syntax.h"
#include "nvim/window.h"
#include "nvim/undo.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "api/buffer.c.generated.h"
#endif

/// Gets the buffer line count
///
/// @param buffer   Buffer handle
/// @param[out] err Error details, if any
///
/// @return Line count
Integer nvim_buf_line_count(Buffer buffer, error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return 0;
    }

    return buf->b_ml.ml_line_count;
}

/// Gets the current line
///
/// @param[out] err Error details, if any
/// @return Current line string
String nvim_get_current_line(error_st *err)
FUNC_API_SINCE(1)
{
    String rv = { .size = 0 };

    Integer index = convert_index(curwin->w_cursor.lnum - 1);

    Array slice =
        nvim_buf_get_lines(0, curbuf->b_id, index, index+1, true, err);

    if(!ERROR_SET(err) && slice.size)
    {
        rv = slice.items[0].data.string;
    }

    xfree(slice.items);
    return rv;
}

/// Sets the current line
///
/// @param line     Line contents
/// @param[out] err Error details, if any
void nvim_set_current_line(String line, error_st *err)
FUNC_API_SINCE(1)
{
    Object l = STRING_OBJ(line);
    Array array = {
        .items = &l,
        .size = 1
    };

    Integer index = convert_index(curwin->w_cursor.lnum - 1);
    nvim_buf_set_lines(0, curbuf->b_id, index, index+1, true, array, err);
}

/// Deletes the current line
///
/// @param[out] err Error details, if any
void nvim_del_current_line(error_st *err)
FUNC_API_SINCE(1)
{
    Array array = ARRAY_DICT_INIT;
    Integer index = convert_index(curwin->w_cursor.lnum - 1);
    nvim_buf_set_lines(0, curbuf->b_id, index, index+1, true, array, err);
}

/// Retrieves a line range from the buffer
///
/// Indexing is zero-based, end-exclusive. Negative indices are interpreted
/// as length+1+index, i e -1 refers to the index past the end. So to get the
/// last element set start=-2 and end=-1.
///
/// Out-of-bounds indices are clamped to the nearest valid value, unless
/// @b strict_indexing is set.
///
/// @param buffer           Buffer handle
/// @param start            First line index
/// @param end              Last line index (exclusive)
/// @param strict_indexing  Whether out-of-bounds should be an error.
/// @param[out] err         Error details, if any
/// @return Array of lines
ArrayOf(String) nvim_buf_get_lines(uint64_t channel_id,
                                   Buffer buffer,
                                   Integer start,
                                   Integer end,
                                   Boolean strict_indexing,
                                   error_st *err)
FUNC_API_SINCE(1)
{
    Array rv = ARRAY_DICT_INIT;
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return rv;
    }

    bool oob = false;
    start = normalize_index(buf, start, &oob);
    end = normalize_index(buf, end, &oob);

    if(strict_indexing && oob)
    {
        api_set_error(err, kErrorTypeValidation, "Index out of bounds");
        return rv;
    }

    if(start >= end)
    {
        // Return 0-length array
        return rv;
    }

    rv.size = (size_t)(end - start);
    rv.items = xcalloc(sizeof(Object), rv.size);

    for(size_t i = 0; i < rv.size; i++)
    {
        int64_t lnum = start + (int64_t)i;

        if(lnum > LONG_MAX)
        {
            api_set_error(err, kErrorTypeValidation, "Line index is too high");
            goto end;
        }

        const char *bufstr = (char *) ml_get_buf(buf, (linenum_kt) lnum, false);
        Object str = STRING_OBJ(cstr_to_string(bufstr));

        // Vim represents NULs as NLs, but this may confuse clients.
        if(channel_id != VIML_INTERNAL_CALL)
        {
            xstrchrsub(str.data.string.data, '\n', '\0');
        }

        rv.items[i] = str;
    }

end:

    if(ERROR_SET(err))
    {
        for(size_t i = 0; i < rv.size; i++)
        {
            xfree(rv.items[i].data.string.data);
        }

        xfree(rv.items);
        rv.items = NULL;
    }

    return rv;
}

/// Replaces line range on the buffer
///
/// Indexing is zero-based, end-exclusive. Negative indices are interpreted
/// as length+1+index, i e -1 refers to the index past the end. So to change
/// or delete the last element set start=-2 and end=-1.
///
/// To insert lines at a given index, set both start and end to the same index.
/// To delete a range of lines, set replacement to an empty array.
///
/// Out-of-bounds indices are clamped to the nearest valid value, unless
/// `strict_indexing` is set.
///
/// @param buffer           Buffer handle
/// @param start            First line index
/// @param end              Last line index (exclusive)
/// @param strict_indexing  Whether out-of-bounds should be an error.
/// @param replacement      Array of lines to use as replacement
/// @param[out] err         Error details, if any
void nvim_buf_set_lines(uint64_t channel_id,
                        Buffer buffer,
                        Integer start,
                        Integer end,
                        Boolean strict_indexing,
                        ArrayOf(String) replacement, // NOLINT
                        error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return;
    }

    bool oob = false;
    start = normalize_index(buf, start, &oob);
    end = normalize_index(buf, end, &oob);

    if(strict_indexing && oob)
    {
        api_set_error(err, kErrorTypeValidation, "Index out of bounds");
        return;
    }

    if(start > end)
    {
        api_set_error(err, kErrorTypeValidation,
                      "Argument \"start\" is higher than \"end\"");
        return;
    }

    for(size_t i = 0; i < replacement.size; i++)
    {
        if(replacement.items[i].type != kObjectTypeString)
        {
            api_set_error(err, kErrorTypeValidation,
                          "All items in the replacement array must be strings");
            return;
        }

        // Disallow newlines in the middle of the line.
        if(channel_id != VIML_INTERNAL_CALL)
        {
            const String l = replacement.items[i].data.string;

            if(memchr(l.data, NL, l.size))
            {
                api_set_error(err, kErrorTypeValidation,
                              "String cannot contain newlines");
                return;
            }
        }
    }

    win_st *save_curwin = NULL;
    tabpage_st *save_curtab = NULL;
    size_t new_len = replacement.size;
    size_t old_len = (size_t)(end - start);
    ptrdiff_t extra = 0;  // lines added to text, can be negative
    char **lines = (new_len != 0) ? xcalloc(new_len, sizeof(char *)) : NULL;

    for(size_t i = 0; i < new_len; i++)
    {
        const String l = replacement.items[i].data.string;
        // Fill lines[i] with l's contents.
        // Convert NULs to newlines as required by NL-used-for-NUL.
        lines[i] = xmemdupz(l.data, l.size);
        xmemchrsub(lines[i], NUL, NL, l.size);
    }

    try_start();
    bufref_st save_curbuf = { NULL, 0 };
    switch_to_win_for_buf(buf, &save_curwin, &save_curtab, &save_curbuf);

    if(u_save((linenum_kt)(start - 1), (linenum_kt)end) == FAIL)
    {
        api_set_error(err, kErrorTypeException,
                      "Failed to save undo information");
        goto end;
    }

    // If the size of the range is reducing (ie, new_len < old_len) we
    // need to delete some old_len. We do this at the start, by
    // repeatedly deleting line "start".
    size_t to_delete = (new_len < old_len) ? (size_t)(old_len - new_len) : 0;

    for(size_t i = 0; i < to_delete; i++)
    {
        if(ml_delete((linenum_kt)start, false) == FAIL)
        {
            api_set_error(err, kErrorTypeException, "Failed to delete line");
            goto end;
        }
    }

    if(to_delete > 0)
    {
        extra -= (ptrdiff_t)to_delete;
    }

    // For as long as possible, replace the existing old_len with the
    // new old_len. This is a more efficient operation, as it requires
    // less memory allocation and freeing.
    size_t to_replace = old_len < new_len ? old_len : new_len;

    for(size_t i = 0; i < to_replace; i++)
    {
        int64_t lnum = start + (int64_t)i;

        if(lnum > LONG_MAX)
        {
            api_set_error(err, kErrorTypeValidation, "Index value is too high");
            goto end;
        }

        if(ml_replace((linenum_kt)lnum, (uchar_kt *)lines[i], false) == FAIL)
        {
            api_set_error(err, kErrorTypeException, "Failed to replace line");
            goto end;
        }

        // Mark lines that haven't been passed
        // to the buffer as they need to be freed later
        lines[i] = NULL;
    }

    // Now we may need to insert the remaining new old_len
    for(size_t i = to_replace; i < new_len; i++)
    {
        int64_t lnum = start + (int64_t)i - 1;

        if(lnum > LONG_MAX)
        {
            api_set_error(err, kErrorTypeValidation, "Index value is too high");
            goto end;
        }

        if(ml_append((linenum_kt)lnum, (uchar_kt *)lines[i], 0, false) == FAIL)
        {
            api_set_error(err, kErrorTypeException, "Failed to insert line");
            goto end;
        }

        // Same as with replacing, but we also need to free lines
        xfree(lines[i]);
        lines[i] = NULL;
        extra++;
    }

    // Adjust marks. Invalidate any which lie in the
    // changed range, and move any in the remainder of the buffer.
    // Only adjust marks if we managed to switch to a window that holds
    // the buffer, otherwise line numbers will be invalid.
    if(save_curbuf.br_buf == NULL)
    {
        mark_adjust((linenum_kt)start, (linenum_kt)(end - 1), MAXLNUM, (long)extra);
    }

    changed_lines((linenum_kt)start, 0, (linenum_kt)end, (long)extra);

    if(save_curbuf.br_buf == NULL)
    {
        fix_cursor((linenum_kt)start, (linenum_kt)end, (linenum_kt)extra);
    }

end:

    for(size_t i = 0; i < new_len; i++)
    {
        xfree(lines[i]);
    }

    xfree(lines);
    restore_win_for_buf(save_curwin, save_curtab, &save_curbuf);
    try_end(err);
}

/// Gets a buffer-scoped (b:) variable.
///
/// @param buffer     Buffer handle
/// @param name       Variable name
/// @param[out] err   Error details, if any
/// @return Variable value
Object nvim_buf_get_var(Buffer buffer, String name, error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return (Object) OBJECT_INIT;
    }

    return dict_get_value(buf->b_vars, name, err);
}

/// Gets a changed tick of a buffer
///
/// @param[in]  buffer  Buffer handle.
///
/// @return `b:changedtick` value.
Integer nvim_buf_get_changedtick(Buffer buffer, error_st *err)
FUNC_API_SINCE(2)
{
    const filebuf_st *const buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return -1;
    }

    return buf->b_changedtick;
}

/// Get a list of dictionaries describing buffer-local mappings
/// Note that the buffer key in the dictionary will represent the buffer
/// handle where the mapping is present
///
/// @param  mode       The abbreviation for the mode
/// @param  buffer_id  Buffer handle
/// @param[out] err    Error details, if any
/// @returns An array of maparg() like dictionaries describing mappings
ArrayOf(Dictionary) nvim_buf_get_keymap(Buffer buffer,
                                        String mode,
                                        error_st *err)
FUNC_API_SINCE(3)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return (Array)ARRAY_DICT_INIT;
    }

    return keymap_array(mode, buf);
}

/// Sets a buffer-scoped (b:) variable
///
/// @param buffer     Buffer handle
/// @param name       Variable name
/// @param value      Variable value
/// @param[out] err   Error details, if any
void nvim_buf_set_var(Buffer buffer, String name, Object value, error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return;
    }

    dict_set_var(buf->b_vars, name, value, false, false, err);
}

/// Removes a buffer-scoped (b:) variable
///
/// @param buffer     Buffer handle
/// @param name       Variable name
/// @param[out] err   Error details, if any
void nvim_buf_del_var(Buffer buffer, String name, error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return;
    }

    dict_set_var(buf->b_vars, name, NIL, true, false, err);
}

/// Gets a buffer option value
///
/// @param buffer     Buffer handle
/// @param name       Option name
/// @param[out] err   Error details, if any
/// @return Option value
Object nvim_buf_get_option(Buffer buffer, String name, error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return (Object) OBJECT_INIT;
    }

    return get_option_from(buf, SREQ_BUF, name, err);
}

/// Sets a buffer option value. Passing 'nil' as value
/// deletes the option (only works if there's a global fallback)
///
/// @param buffer     Buffer handle
/// @param name       Option name
/// @param value      Option value
/// @param[out] err   Error details, if any
void nvim_buf_set_option(Buffer buffer, String name, Object value, error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return;
    }

    set_option_to(buf, SREQ_BUF, name, value, err);
}

/// Gets the buffer number
///
/// @deprecated The buffer number now is equal to the object id,
///             so there is no need to use this function.
///
/// @param buffer     Buffer handle
/// @param[out] err   Error details, if any
/// @return Buffer number
Integer nvim_buf_get_number(Buffer buffer, error_st *err)
FUNC_API_SINCE(1)
FUNC_API_DEPRECATED_SINCE(2)
{
    Integer rv = 0;
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return rv;
    }

    return buf->b_id;
}

/// Gets the full file name for the buffer
///
/// @param buffer     Buffer handle
/// @param[out] err   Error details, if any
/// @return Buffer name
String nvim_buf_get_name(Buffer buffer, error_st *err)
FUNC_API_SINCE(1)
{
    String rv = STRING_INIT;
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf || buf->b_ffname == NULL)
    {
        return rv;
    }

    return cstr_to_string((char *)buf->b_ffname);
}

/// Sets the full file name for a buffer
///
/// @param buffer     Buffer handle
/// @param name       Buffer name
/// @param[out] err   Error details, if any
void nvim_buf_set_name(Buffer buffer, String name, error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return;
    }

    try_start();
    // Using aucmd_*: autocommands will be executed by rename_buffer
    save_autocmd_st aco;
    aucmd_prepbuf(&aco, buf);
    int ren_ret = rename_buffer((uchar_kt *) name.data);
    aucmd_restbuf(&aco);

    if(try_end(err))
    {
        return;
    }

    if(ren_ret == FAIL)
    {
        api_set_error(err, kErrorTypeException, "Failed to rename buffer");
    }
}

/// Checks if a buffer is valid
///
/// @param buffer Buffer handle
/// @return true if the buffer is valid, false otherwise
Boolean nvim_buf_is_valid(Buffer buffer)
FUNC_API_SINCE(1)
{
    error_st stub = ERROR_INIT;
    Boolean ret = find_buffer_by_handle(buffer, &stub) != NULL;
    api_clear_error(&stub);
    return ret;
}

/// Return a tuple (row,col) representing the position of the named mark
///
/// @param buffer     Buffer handle
/// @param name       Mark name
/// @param[out] err   Error details, if any
/// @return (row, col) tuple
ArrayOf(Integer, 2) nvim_buf_get_mark(Buffer buffer, String name, error_st *err)
FUNC_API_SINCE(1)
{
    Array rv = ARRAY_DICT_INIT;
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return rv;
    }

    if(name.size != 1)
    {
        api_set_error(err, kErrorTypeValidation,
                      "Mark name must be a single character");
        return rv;
    }

    apos_st *posp;
    char mark = *name.data;
    try_start();
    bufref_st save_buf;
    switch_buffer(&save_buf, buf);
    posp = getmark(mark, false);
    restore_buffer(&save_buf);

    if(try_end(err))
    {
        return rv;
    }

    if(posp == NULL)
    {
        api_set_error(err, kErrorTypeValidation, "Invalid mark name");
        return rv;
    }

    ADD(rv, INTEGER_OBJ(posp->lnum));
    ADD(rv, INTEGER_OBJ(posp->col));

    return rv;
}

/// Adds a highlight to buffer.
///
/// This can be used for plugins which dynamically generate highlights to a
/// buffer (like a semantic highlighter or linter). The function adds a single
/// highlight to a buffer. Unlike matchaddpos() highlights follow changes to
/// line numbering (as lines are inserted/removed above the highlighted line),
/// like signs and marks do.
///
/// "src_id" is useful for batch deletion/updating of a set of highlights. When
/// called with src_id = 0, an unique source id is generated and returned.
/// Succesive calls can pass in it as "src_id" to add new highlights to the same
/// source group. All highlights in the same group can then be cleared with
/// nvim_buf_clear_highlight. If the highlight never will be manually deleted
/// pass in -1 for "src_id".
///
/// If "hl_group_st" is the empty string no highlight is added, but a new src_id
/// is still returned. This is useful for an external plugin to synchrounously
/// request an unique src_id at initialization, and later asynchronously add and
/// clear highlights in response to buffer changes.
///
/// @param buffer     Buffer handle
/// @param src_id     Source group to use or 0 to use a new group,
///                   or -1 for ungrouped highlight
/// @param hl_group   Name of the highlight group to use
/// @param line       Line to highlight
/// @param col_start  Start of range of columns to highlight
/// @param col_end    End of range of columns to highlight,
///                   or -1 to highlight to end of line
/// @param[out] err   Error details, if any
/// @return The src_id that was used
Integer nvim_buf_add_highlight(Buffer buffer,
                               Integer src_id,
                               String hl_group,
                               Integer line,
                               Integer col_start,
                               Integer col_end,
                               error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return 0;
    }

    if(line < 0 || line >= MAXLNUM)
    {
        api_set_error(err, kErrorTypeValidation, "Line number outside range");
        return 0;
    }

    if(col_start < 0 || col_start > MAXCOL)
    {
        api_set_error(err, kErrorTypeValidation, "Column value outside range");
        return 0;
    }

    if(col_end < 0 || col_end > MAXCOL)
    {
        col_end = MAXCOL;
    }

    int hlg_id = syn_name2id((uchar_kt *)(hl_group.data ? hl_group.data : ""));

    src_id = bufhl_add_hl(buf,
                          (int)src_id, hlg_id,
                          (linenum_kt)line+1,
                          (columnum_kt)col_start+1,
                          (columnum_kt)col_end);
    return src_id;
}

/// Clears highlights from a given source group and a range of lines
///
/// To clear a source group in the entire buffer, pass in 1 and -1 to
/// line_start and line_end respectively.
///
/// @param buffer     Buffer handle
/// @param src_id     Highlight source group to clear, or -1 to clear all.
/// @param line_start Start of range of lines to clear
/// @param line_end   End of range of lines to clear (exclusive) or -1 to clear
///                   to end of file.
/// @param[out] err   Error details, if any
void nvim_buf_clear_highlight(Buffer buffer,
                              Integer src_id,
                              Integer line_start,
                              Integer line_end,
                              error_st *err)
FUNC_API_SINCE(1)
{
    filebuf_st *buf = find_buffer_by_handle(buffer, err);

    if(!buf)
    {
        return;
    }

    if(line_start < 0 || line_start >= MAXLNUM)
    {
        api_set_error(err, kErrorTypeValidation, "Line number outside range");
        return;
    }

    if(line_end < 0 || line_end > MAXLNUM)
    {
        line_end = MAXLNUM;
    }

    bufhl_clear_line_range(buf, (int)src_id, (int)line_start+1, (int)line_end);
}

// Check if deleting lines made the cursor position invalid.
// Changed the lines from "lo" to "hi" and added "extra" lines (negative if
// deleted).
static void fix_cursor(linenum_kt lo, linenum_kt hi, linenum_kt extra)
{
    if(curwin->w_cursor.lnum >= lo)
    {
        // Adjust the cursor position if it's in/after the changed lines.
        if(curwin->w_cursor.lnum >= hi)
        {
            curwin->w_cursor.lnum += extra;
            check_cursor_col();
        }
        else if(extra < 0)
        {
            curwin->w_cursor.lnum = lo;
            check_cursor();
        }
        else
        {
            check_cursor_col();
        }

        changed_cline_bef_curs();
    }

    invalidate_botline();
}

// Normalizes 0-based indexes to buffer line numbers
static int64_t normalize_index(filebuf_st *buf, int64_t index, bool *oob)
{
    int64_t line_count = buf->b_ml.ml_line_count;
    // Fix if < 0
    index = index < 0 ? line_count + index +1 : index;

    // Check for oob
    if(index > line_count)
    {
        *oob = true;
        index = line_count;
    }
    else if(index < 0)
    {
        *oob = true;
        index = 0;
    }

    // Convert the index to a vim line number
    index++;
    return index;
}

static int64_t convert_index(int64_t index)
{
    return index < 0 ? index - 1 : index;
}
