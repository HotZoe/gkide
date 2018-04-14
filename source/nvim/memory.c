/// @file nvim/memory.c
///
/// Various routines dealing with allocation and deallocation of memory.

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "nvim/ui.h"
#include "nvim/nvim.h"
#include "nvim/eval.h"
#include "nvim/error.h"
#include "nvim/misc1.h"
#include "nvim/memory.h"
#include "nvim/memfile.h"
#include "nvim/message.h"

#include "generated/config/confignvim.h"

#ifdef FOUND_WORKING_JEMALLOC
    // Force je_ prefix on jemalloc functions.
    #define JEMALLOC_NO_DEMANGLE
    #include <jemalloc/jemalloc.h>
#endif

#ifdef UNIT_TESTING
    #define free(ptr)           mem_free(ptr)
    #define malloc(size)        mem_malloc(size)
    #define calloc(count, size) mem_calloc(count, size)
    #define realloc(ptr, size)  mem_realloc(ptr, size)

    #ifdef FOUND_WORKING_JEMALLOC
        mem_free_ft    mem_free    = &je_free;
        mem_malloc_ft  mem_malloc  = &je_malloc;
        mem_calloc_ft  mem_calloc  = &je_calloc;
        mem_realloc_ft mem_realloc = &je_realloc;
    #else
        mem_free_ft    mem_free    = &free;
        mem_malloc_ft  mem_malloc  = &malloc;
        mem_calloc_ft  mem_calloc  = &calloc;
        mem_realloc_ft mem_realloc = &realloc;
    #endif
#else
    #ifdef FOUND_WORKING_JEMALLOC
        #define free(ptr)           je_free(ptr)
        #define malloc(size)        je_malloc(size)
        #define calloc(count, size) je_calloc(count, size)
        #define realloc(ptr, size)  je_realloc(ptr, size)
    #endif
#endif

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "memory.c.generated.h"
#endif

#ifdef EXITFREE
bool entered_free_all_mem = false;
#endif

/// Try to free memory for garbage collection.
/// Used when trying to recover from out of memory errors.
///
/// @see xmalloc()
void try_to_free_memory(void)
{
    static bool trying_to_free = false;

    // avoid recursive calls
    if(trying_to_free)
    {
        return;
    }

    trying_to_free = true;

    // free any scrollback text
    clear_sb_text();

    // Try to save all buffers and release as many blocks as possible
    mf_release_all();

    trying_to_free = false;
}

/// malloc() wrapper with garbage collection
/// - a malloc() wrapper that tries to free some memory before trying again
/// - shows an out-of-memory error message to the user before returning NULL
///
/// @param size
/// @return pointer to allocated space. NULL if out of memory
///
/// @see try_to_free_memory()
static void *try_malloc(size_t size)
FUNC_ATTR_MALLOC
FUNC_ATTR_ALLOC_SIZE(1)
{
    size_t allocated_size = size ? size : 1;
    void *ret = malloc(allocated_size);

    if(!ret)
    {
        try_to_free_memory();
        ret = malloc(allocated_size);
    }

    if(!ret)
    {
        msg_out_of_memory(size);
    }

    return ret;
}

/// malloc() wrapper that never returns NULL
///
/// xmalloc() succeeds or gracefully aborts when out of memory.
/// Before aborting try to free some memory and call malloc again.
///
/// @param size
/// @return pointer to allocated space. Never NULL
///
/// @see try_to_free_memory()
void *xmalloc(size_t size)
FUNC_ATTR_MALLOC
FUNC_ATTR_ALLOC_SIZE(1)
FUNC_ATTR_NONNULL_RET
{
    void *ret = try_malloc(size);

    if(!ret)
    {
        mch_errmsg(e_outofmem);
        mch_errmsg("\n");
        preserve_exit(kNEStatusHostMemoryNotEnough);
    }

    return ret;
}

/// free() wrapper, which delegates to the background memory manager
void xfree(void *ptr)
{
    free(ptr);
}

/// calloc() wrapper, the memory is set to zero.
///
/// @param count
/// @param size
///
/// @return pointer to allocated space. Never NULL
///
/// @see xmalloc()
void *xcalloc(size_t count, size_t size)
FUNC_ATTR_MALLOC
FUNC_ATTR_ALLOC_SIZE_PROD(1, 2)
FUNC_ATTR_NONNULL_RET
{
    size_t allocated_count = count && size ? count : 1;
    size_t allocated_size = count && size ? size : 1;
    void *ret = calloc(allocated_count, allocated_size);

    if(!ret)
    {
        try_to_free_memory();
        ret = calloc(allocated_count, allocated_size);

        if(!ret)
        {
            mch_errmsg(e_outofmem);
            mch_errmsg("\n");
            preserve_exit(kNEStatusHostMemoryNotEnough);
        }
    }

    return ret;
}

/// realloc() wrapper
///
/// @param size
///
/// @return pointer to reallocated space. Never NULL
///
/// @see xmalloc()
void *xrealloc(void *ptr, size_t size)
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_ALLOC_SIZE(2)
FUNC_ATTR_NONNULL_RET
{
    size_t allocated_size = size ? size : 1;
    void *ret = realloc(ptr, allocated_size);

    if(!ret)
    {
        try_to_free_memory();
        ret = realloc(ptr, allocated_size);

        if(!ret)
        {
            mch_errmsg(e_outofmem);
            mch_errmsg("\n");
            preserve_exit(kNEStatusHostMemoryNotEnough);
        }
    }

    return ret;
}

/// xmalloc() wrapper that allocates
/// size + 1 bytes and zeroes the last byte
///
/// @param size
///
/// @return pointer to allocated space. Never NULL
///
/// @see xmalloc()
void *xmallocz(size_t size)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_RET
FUNC_ATTR_WARN_UNUSED_RESULT
{
    size_t total_size = size + 1;

    if(total_size < size)
    {
        mch_errmsg(_("Data too large to fit into virtual memory space\n"));
        preserve_exit(kNEStatusFileTooBigToOpen);
    }

    void *ret = xmalloc(total_size);
    ((char *)ret)[size] = 0;

    return ret;
}

/// Allocates (len + 1) bytes of memory, duplicates @b len bytes of
/// @b data to the allocated memory, zero terminates the allocated memory,
/// and returns a pointer to the allocated memory. If the allocation fails,
/// the program dies.
///
/// @param data Pointer to the data that will be copied
/// @param len number of bytes that will be copied
///
/// @see xmalloc()
void *xmemdupz(const void *data, size_t len)
FUNC_ATTR_MALLOC
FUNC_ATTR_NONNULL_RET
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_NONNULL_ALL
{
    return memcpy(xmallocz(len), data, len);
}

/// A version of memchr() that returns a pointer
/// one past the end if it doesn't find @b c.
///
/// @param addr The address of the memory object.
/// @param c    The char to look for.
/// @param size The size of the memory object.
///
/// @returns
/// a pointer to the first instance of @b c,
/// or one past the end if not found.
void *xmemscan(const void *addr, char c, size_t size)
FUNC_ATTR_NONNULL_RET
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_PURE
{
    char *p = memchr(addr, c, size);

    return p ? p : (char *)addr + size;
}

/// Replaces every instance of @b c with @b x.
///
/// @param data  An object in memory. May contain NULs.
/// @param c     The unwanted byte.
/// @param x     The replacement.
/// @param len   The length of data.
void xmemchrsub(void *data, char c, char x, size_t len)
FUNC_ATTR_NONNULL_ALL
{
    char *p = data, *end = (char *)data + len;

    while((p = memchr(p, c, (size_t)(end - p))))
    {
        *p++ = x;
    }
}

/// Counts the number of occurrences of byte @b c in @b data[len].
///
/// @param data  Pointer to the data to search.
/// @param c     The byte to search for.
/// @param len   The length of @b data.
///
/// @returns the number of occurrences of @b c in @b data[len].
size_t xmemcnt(const void *data, char c, size_t len)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_PURE
{
    size_t cnt = 0;
    const char *ptr = data;
    const char *end = ptr + len;

    while((ptr = memchr(ptr, c, (size_t)(end - ptr))) != NULL)
    {
        cnt++;
        ptr++; // Skip the instance of c.
    }

    return cnt;
}

/// A version of memchr that starts the search at @b src + @b len.
///
/// Based on glibc's memrchr.
///
/// @param src  The source memory object.
/// @param c    The byte to search for.
/// @param len  The length of the memory object.
///
/// @returns a pointer to the found byte in src[len], or NULL.
void *xmemrchr(const void *src, uint8_t c, size_t len)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_PURE
{
    while(len--)
    {
        if(((uint8_t *)src)[len] == c)
        {
            return (uint8_t *) src + len;
        }
    }

    return NULL;
}

/// Duplicates a chunk of memory using xmalloc
///
/// @param data pointer to the chunk
/// @param len size of the chunk
///
/// @return a pointer
///
/// @see xmalloc()
void *xmemdup(const void *data, size_t len)
FUNC_ATTR_MALLOC
FUNC_ATTR_ALLOC_SIZE(2)
FUNC_ATTR_NONNULL_RET
FUNC_ATTR_WARN_UNUSED_RESULT
FUNC_ATTR_NONNULL_ALL
{
    return memcpy(xmalloc(len), data, len);
}

#if defined(EXITFREE)
#include "nvim/file_search.h"
#include "nvim/buffer.h"
#include "nvim/charset.h"
#include "nvim/diff.h"
#include "nvim/edit.h"
#include "nvim/ex_cmds.h"
#include "nvim/ex_docmd.h"
#include "nvim/ex_getln.h"
#include "nvim/fileio.h"
#include "nvim/fold.h"
#include "nvim/getchar.h"
#include "nvim/mark.h"
#include "nvim/mbyte.h"
#include "nvim/memline.h"
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

/// Free everything that we allocated.
/// Can be used to detect memory leaks, e.g., with ccmalloc.
///
/// @note
/// This is tricky! Things are freed that functions depend on. Don't be
/// surprised if Vim crashes... Some things can't be freed, esp. things
/// local to a library function.
void free_all_mem(void)
{
    filebuf_st *buf, *nextbuf;

    // When we cause a crash here it is caught and Vim tries
    // to exit cleanly. Don't try freeing everything again.
    if(entered_free_all_mem)
    {
        return;
    }

    entered_free_all_mem = true;

    // Don't want to trigger autocommands from here on.
    block_autocmds();

    // Close all tabs and windows.
    // Reset 'equalalways' to avoid redraws.
    p_ea = false;

    if(first_tabpage->tp_next != NULL)
    {
        do_cmdline_cmd("tabonly!");
    }

    if(firstwin != lastwin)
    {
        do_cmdline_cmd("only!");
    }

    // Free all spell info.
    spell_free_all();

    // Clear user commands (before deleting buffers).
    ex_comclear(NULL);

    // Clear menus.
    do_cmdline_cmd("aunmenu *");
    do_cmdline_cmd("menutranslate clear");

    // Clear mappings, abbreviations, breakpoints.
    do_cmdline_cmd("lmapclear");
    do_cmdline_cmd("xmapclear");
    do_cmdline_cmd("mapclear");
    do_cmdline_cmd("mapclear!");
    do_cmdline_cmd("abclear");
    do_cmdline_cmd("breakdel *");
    do_cmdline_cmd("profdel *");
    do_cmdline_cmd("set keymap=");

    free_titles();
    free_findfile();

    // Obviously named calls.
    free_all_autocmds();
    free_all_options();
    free_all_marks();

    alist_clear(&g_arglist);

    free_homedir();
    free_users();
    free_search_patterns();
    free_old_sub();
    free_last_insert();
    free_prev_shellcmd();
    free_regexp_stuff();
    free_tag_stuff();
    free_cd_dir();
    free_signs();

    set_expr_line(NULL);
    diff_clear(curtab);
    clear_sb_text(); // free any scrollback text

    xfree(last_cmdline); // Free some global vars.
    xfree(new_last_cmdline);
    set_keep_msg(NULL, 0);

    // Clear cmdline history.
    p_hi = 0;
    init_history();
    qf_free_all(NULL);

    // Free all location lists
    FOR_ALL_TAB_WINDOWS(tab, win)
    {
        qf_free_all(win);
    }

    // Close all script inputs.
    close_all_scripts();

    // Destroy all windows.
    // Must come before freeing buffers.
    win_free_all();
    free_cmdline_buf();

    // Clear registers.
    clear_registers();
    ResetRedobuff();
    ResetRedobuff();

    // highlight info
    free_highlight();
    reset_last_sourcing();
    free_tabpage(first_tabpage);
    first_tabpage = NULL;

    // message history
    for(;;)
    {
        if(delete_first_msg() == FAIL)
        {
            break;
        }
    }

    eval_clear();

    // Free all buffers. Reset 'autochdir' to avoid accessing
    // things that were freed already. Must be after eval_clear
    // to avoid it trying to access b:changedtick after freeing it.
    p_acd = false;

    for(buf = firstbuf; buf != NULL;)
    {
        bufref_st bufref;
        set_bufref(&bufref, buf);
        nextbuf = buf->b_next;
        close_buffer(NULL, buf, DOBUF_WIPE, false);

        // Didn't work, try next one.
        buf = bufref_valid(&bufref) ? nextbuf : firstbuf;
    }

    // screenlines (can't display anything now!)
    free_screenlines();
    clear_hl_tables();
}
#endif
