/// @file nvim/mark.c
///
/// This file contains routines to maintain and manipulate marks:
/// - functions for setting marks and jumping to them
/// - If a named file mark's lnum is non-zero, it is valid.
/// - If a named file mark's fnum is non-zero, it is for an existing buffer,
///   otherwise it is from .shada and namedfm[n].fname is the file name.
/// - There are marks 'A - 'Z (set by user) and '0 to '9 (set when writing shada).

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "nvim/vim.h"
#include "nvim/ascii.h"
#include "nvim/mark.h"
#include "nvim/buffer.h"
#include "nvim/charset.h"
#include "nvim/diff.h"
#include "nvim/eval.h"
#include "nvim/ex_cmds.h"
#include "nvim/fileio.h"
#include "nvim/fold.h"
#include "nvim/mbyte.h"
#include "nvim/memline.h"
#include "nvim/memory.h"
#include "nvim/message.h"
#include "nvim/normal.h"
#include "nvim/option.h"
#include "nvim/path.h"
#include "nvim/quickfix.h"
#include "nvim/search.h"
#include "nvim/strings.h"
#include "nvim/ui.h"
#include "nvim/os/os.h"
#include "nvim/os/time.h"
#include "nvim/os/input.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "mark.c.generated.h"
#endif

/// Global marks (marks with file number or name)
static xfilemark_st namedfm[NGLOBALMARKS];

/// Set named mark "c" at current cursor position.
/// Returns OK on success, FAIL if bad name given.
int setmark(int c)
{
    return setmark_pos(c, &curwin->w_cursor, curbuf->b_fnum);
}

/// Free filemark_st item
void free_fmark(filemark_st fm)
{
    tv_dict_unref(fm.additional_data);
}

/// Free xfilemark_st item
void free_xfmark(xfilemark_st fm)
{
    xfree(fm.fname);
    free_fmark(fm.fmark);
}

/// Free and clear filemark_st item
void clear_fmark(filemark_st *fm)
FUNC_ATTR_NONNULL_ALL
{
    free_fmark(*fm);
    memset(fm, 0, sizeof(*fm));
}

/// Set named mark "c" to position "pos".
/// When "c" is upper case use file "fnum".
/// Returns OK on success, FAIL if bad name given.
int setmark_pos(int c, apos_st *pos, int fnum)
{
    int i;

    // Check for a special key (may cause islower() to crash).
    if(c < 0)
    {
        return FAIL;
    }

    if(c == '\'' || c == '`')
    {
        if(pos == &curwin->w_cursor)
        {
            setpcmark();

            // keep it even when the cursor doesn't move
            curwin->w_prev_pcmark = curwin->w_pcmark;
        }
        else
        {
            curwin->w_pcmark = *pos;
        }

        return OK;
    }

    if(c == '"')
    {
        RESET_FMARK(&curbuf->b_last_cursor, *pos, curbuf->b_fnum);
        return OK;
    }

    // Allow setting '[ and '] for an autocommand
    // that simulates reading a file.
    if(c == '[')
    {
        curbuf->b_op_start = *pos;
        return OK;
    }

    if(c == ']')
    {
        curbuf->b_op_end = *pos;
        return OK;
    }

    if(c == '<' || c == '>')
    {
        if(c == '<')
        {
            curbuf->b_visual.vi_start = *pos;
        }
        else
        {
            curbuf->b_visual.vi_end = *pos;
        }

        if(curbuf->b_visual.vi_mode == NUL)
        {
            // Visual_mode has not yet been set, use a sane default.
            curbuf->b_visual.vi_mode = 'v';
        }

        return OK;
    }

    fbuf_st *buf = buflist_findnr(fnum);

    // Can't set a mark in a non-existant buffer.
    if(buf == NULL)
    {
        return FAIL;
    }

    if(ASCII_ISLOWER(c))
    {
        i = c - 'a';
        RESET_FMARK(buf->b_namedm + i, *pos, fnum);
        return OK;
    }

    if(ASCII_ISUPPER(c) || ascii_isdigit(c))
    {
        if(ascii_isdigit(c))
        {
            i = c - '0' + NMARKS;
        }
        else
        {
            i = c - 'A';
        }

        RESET_XFMARK(namedfm + i, *pos, fnum, NULL);
        return OK;
    }

    return FAIL;
}

/// Set the previous context mark to the current
/// position and add it to the jump list.
void setpcmark(void)
{
    xfilemark_st *fm;

    // for :global the mark is set only once
    if(global_busy || listcmd_busy || cmdmod.keepjumps)
    {
        return;
    }

    curwin->w_prev_pcmark = curwin->w_pcmark;
    curwin->w_pcmark = curwin->w_cursor;

    // If jumplist is full: remove oldest entry
    if(++curwin->w_jumplistlen > JUMPLISTSIZE)
    {
        curwin->w_jumplistlen = JUMPLISTSIZE;
        free_xfmark(curwin->w_jumplist[0]);

        memmove(&curwin->w_jumplist[0],
                &curwin->w_jumplist[1],
                (JUMPLISTSIZE - 1) * sizeof(curwin->w_jumplist[0]));
    }

    curwin->w_jumplistidx = curwin->w_jumplistlen;
    fm = &curwin->w_jumplist[curwin->w_jumplistlen - 1];

    SET_XFMARK(fm, curwin->w_pcmark, curbuf->b_fnum, NULL);
}

/// To change context, call setpcmark(), then move the current position to
/// where ever, then call checkpcmark(). This ensures that the previous
/// context will only be changed if the cursor moved to a different line.
/// If pcmark was deleted (with "dG") the previous mark is restored.
void checkpcmark(void)
{
    if(curwin->w_prev_pcmark.lnum != 0
       && (equalpos(curwin->w_pcmark, curwin->w_cursor)
           || curwin->w_pcmark.lnum == 0))
    {
        curwin->w_pcmark = curwin->w_prev_pcmark;
        curwin->w_prev_pcmark.lnum = 0; // Show it has been checked
    }
}

/// move "count" positions in the jump list (count may be negative)
apos_st *movemark(int count)
{
    apos_st *pos;
    xfilemark_st *jmp;
    cleanup_jumplist();

    if(curwin->w_jumplistlen == 0) // nothing to jump to
    {
        return (apos_st *)NULL;
    }

    for(;;)
    {
        if(curwin->w_jumplistidx + count < 0
           || curwin->w_jumplistidx + count >= curwin->w_jumplistlen)
        {
            return (apos_st *)NULL;
        }

        // if first CTRL-O or CTRL-I command after a jump, add cursor position
        // to list. Careful: If there are duplicates (CTRL-O immediately after
        // starting Vim on a file), another entry may have been removed.
        if(curwin->w_jumplistidx == curwin->w_jumplistlen)
        {
            setpcmark();
            --curwin->w_jumplistidx; // skip the new entry

            if(curwin->w_jumplistidx + count < 0)
            {
                return (apos_st *)NULL;
            }
        }

        curwin->w_jumplistidx += count;
        jmp = curwin->w_jumplist + curwin->w_jumplistidx;

        if(jmp->fmark.fnum == 0)
        {
            fname2fnum(jmp);
        }

        if(jmp->fmark.fnum != curbuf->b_fnum)
        {
            // jump to other file
            if(buflist_findnr(jmp->fmark.fnum) == NULL) // Skip this one ..
            {
                count += count < 0 ? -1 : 1;
                continue;
            }

            if(buflist_getfile(jmp->fmark.fnum,
                               jmp->fmark.mark.lnum, 0, FALSE) == FAIL)
            {
                return (apos_st *)NULL;
            }

            // Set lnum again, autocommands my have changed it
            curwin->w_cursor = jmp->fmark.mark;
            pos = (apos_st *)-1;
        }
        else
        {
            pos = &(jmp->fmark.mark);
        }

        return pos;
    }
}

/// Move "count" positions in the changelist (count may be negative).
apos_st *movechangelist(int count)
{
    int n;

    if(curbuf->b_changelistlen == 0) // nothing to jump to
    {
        return (apos_st *)NULL;
    }

    n = curwin->w_changelistidx;

    if(n + count < 0)
    {
        if(n == 0)
        {
            return (apos_st *)NULL;
        }

        n = 0;
    }
    else if(n + count >= curbuf->b_changelistlen)
    {
        if(n == curbuf->b_changelistlen - 1)
        {
            return (apos_st *)NULL;
        }

        n = curbuf->b_changelistlen - 1;
    }
    else
    {
        n += count;
    }

    curwin->w_changelistidx = n;
    return &(curbuf->b_changelist[n].mark);
}

/// Find mark "c" in buffer pointed to by "buf".
/// If "changefile" is TRUE it's allowed to edit
/// another file for '0, 'A, etc. If "fnum" is not
/// NULL store the fnum there for '0, 'A etc., don't edit another file.
///
/// @return
/// - pointer to apos_st if found.  lnum is 0 when mark not set, -1 when mark is
///   in another file which can't be gotten. (caller needs to check lnum!)
/// - NULL if there is no mark called 'c'.
/// - -1 if mark is in other file and jumped there (only if changefile is TRUE)
apos_st *getmark_buf(fbuf_st *buf, int c, int changefile)
{
    return getmark_buf_fnum(buf, c, changefile, NULL);
}

apos_st *getmark(int c, int changefile)
{
    return getmark_buf_fnum(curbuf, c, changefile, NULL);
}

apos_st *getmark_buf_fnum(fbuf_st *buf, int c, int changefile, int *fnum)
{
    apos_st *posp;
    apos_st *startp, *endp;
    static apos_st pos_copy;
    posp = NULL;

    // Check for special key, can't be a mark
    // name and might cause islower() to crash.
    if(c < 0)
    {
        return posp;
    }

    if(c > '~') // check for islower()/isupper()
    {
    }
    else if(c == '\'' || c == '`') // previous context mark
    {
        pos_copy = curwin->w_pcmark; // need to make a copy because
        posp = &pos_copy; // w_pcmark may be changed soon
    }
    else if(c == '"') // to pos when leaving buffer
    {
        posp = &(buf->b_last_cursor.mark);
    }
    else if(c == '^') // to where Insert mode stopped
    {
        posp = &(buf->b_last_insert.mark);
    }
    else if(c == '.') // to where last change was made
    {
        posp = &(buf->b_last_change.mark);
    }
    else if(c == '[') // to start of previous operator
    {
        posp = &(buf->b_op_start);
    }
    else if(c == ']') // to end of previous operator
    {
        posp = &(buf->b_op_end);
    }
    else if(c == '{' || c == '}') // to previous/next paragraph
    {
        apos_st pos;
        oparg_T oa;
        int slcb = listcmd_busy;
        pos = curwin->w_cursor;
        listcmd_busy = TRUE; // avoid that '' is changed

        if(findpar(&oa.inclusive, c == '}' ? FORWARD : BACKWARD, 1L, NUL, FALSE))
        {
            pos_copy = curwin->w_cursor;
            posp = &pos_copy;
        }

        curwin->w_cursor = pos;
        listcmd_busy = slcb;
    }
    else if(c == '(' || c == ')') // to previous/next sentence
    {
        apos_st pos;
        int slcb = listcmd_busy;
        pos = curwin->w_cursor;
        listcmd_busy = TRUE; // avoid that '' is changed

        if(findsent(c == ')' ? FORWARD : BACKWARD, 1L))
        {
            pos_copy = curwin->w_cursor;
            posp = &pos_copy;
        }

        curwin->w_cursor = pos;
        listcmd_busy = slcb;
    }
    else if(c == '<' || c == '>') // start/end of visual area
    {
        startp = &buf->b_visual.vi_start;
        endp = &buf->b_visual.vi_end;

        if((c == '<') == lt(*startp, *endp))
        {
            posp = startp;
        }
        else
        {
            posp = endp;
        }

        // For Visual line mode, set mark at begin or end of line
        if(buf->b_visual.vi_mode == 'V')
        {
            pos_copy = *posp;
            posp = &pos_copy;

            if(c == '<')
            {
                pos_copy.col = 0;
            }
            else
            {
                pos_copy.col = MAXCOL;
            }

            pos_copy.coladd = 0;
        }
    }
    else if(ASCII_ISLOWER(c)) // normal named mark
    {
        posp = &(buf->b_namedm[c - 'a'].mark);
    }
    else if(ASCII_ISUPPER(c) || ascii_isdigit(c)) // named file mark
    {
        if(ascii_isdigit(c))
        {
            c = c - '0' + NMARKS;
        }
        else
        {
            c -= 'A';
        }

        posp = &(namedfm[c].fmark.mark);

        if(namedfm[c].fmark.fnum == 0)
        {
            fname2fnum(&namedfm[c]);
        }

        if(fnum != NULL)
        {
            *fnum = namedfm[c].fmark.fnum;
        }
        else if(namedfm[c].fmark.fnum != buf->b_fnum)
        {
            // mark is in another file
            posp = &pos_copy;

            if(namedfm[c].fmark.mark.lnum != 0
               && changefile && namedfm[c].fmark.fnum)
            {
                if(buflist_getfile(namedfm[c].fmark.fnum,
                                   (linenum_kt)1, GETF_SETMARK, FALSE) == OK)
                {
                    // Set the lnum now, autocommands could have changed it
                    curwin->w_cursor = namedfm[c].fmark.mark;
                    return (apos_st *)-1;
                }

                pos_copy.lnum = -1; // can't get file
            }
            else
            {
                pos_copy.lnum = 0;
            }
            // mark exists, but is not valid in current buffer
        }
    }

    return posp;
}

/// Search for the next named mark in the current file.
///
/// @param startpos    where to start
/// @param dir         direction for search
/// @param begin_line
///
/// @return
/// Returns pointer to apos_st of the next mark or NULL if no mark is found.
apos_st *getnextmark(apos_st *startpos, int dir, int begin_line)
{
    int i;
    apos_st *result = NULL;
    apos_st pos;
    pos = *startpos;

    // When searching backward and leaving the cursor on the first non-blank,
    // position must be in a previous line.
    // When searching forward and leaving the cursor on the first non-blank,
    // position must be in a next line.
    if(dir == BACKWARD && begin_line)
    {
        pos.col = 0;
    }
    else if(dir == FORWARD && begin_line)
    {
        pos.col = MAXCOL;
    }

    for(i = 0; i < NMARKS; i++)
    {
        if(curbuf->b_namedm[i].mark.lnum > 0)
        {
            if(dir == FORWARD)
            {
                if((result == NULL || lt(curbuf->b_namedm[i].mark, *result))
                   && lt(pos, curbuf->b_namedm[i].mark))
                {
                    result = &curbuf->b_namedm[i].mark;
                }
            }
            else
            {
                if((result == NULL || lt(*result, curbuf->b_namedm[i].mark))
                   && lt(curbuf->b_namedm[i].mark, pos))
                {
                    result = &curbuf->b_namedm[i].mark;
                }
            }
        }
    }

    return result;
}

/// For an xtended filemark: set the fnum from the fname.
/// This is used for marks obtained from the .shada file. It's postponed
/// until the mark is used to avoid a long startup delay.
static void fname2fnum(xfilemark_st *fm)
{
    uchar_kt *p;

    if(fm->fname != NULL)
    {
        // First expand "~/" in the file name to the home directory.
        // Don't expand the whole name, it may contain other '~' chars.
    #ifdef BACKSLASH_IN_FILENAME
        if(fm->fname[0] == '~' && (fm->fname[1] == '/' || fm->fname[1] == '\\'))
    #else
        if(fm->fname[0] == '~' && (fm->fname[1] == '/'))
    #endif
        {
            int len;
            expand_env((uchar_kt *)"~/", NameBuff, MAXPATHL);
            len = (int)STRLEN(NameBuff);
            STRLCPY(NameBuff + len, fm->fname + 2, MAXPATHL - len);
        }
        else
        {
            STRLCPY(NameBuff, fm->fname, MAXPATHL);
        }

        // Try to shorten the file name.
        os_dirname(IObuff, IOSIZE);
        p = path_shorten_fname(NameBuff, IObuff);

        // buflist_new() will call fmarks_check_names()
        (void)buflist_new(NameBuff, p, (linenum_kt)1, 0);
    }
}

/// Check all file marks for a name that matches the file name in buf.
/// May replace the name with an fnum.
/// Used for marks that come from the .shada file.
void fmarks_check_names(fbuf_st *buf)
{
    int i;
    uchar_kt *name = buf->b_ffname;

    if(buf->b_ffname == NULL)
    {
        return;
    }

    for(i = 0; i < NGLOBALMARKS; ++i)
    {
        fmarks_check_one(&namedfm[i], name, buf);
    }

    FOR_ALL_WINDOWS_IN_TAB(wp, curtab)
    {
        for(i = 0; i < wp->w_jumplistlen; ++i)
        {
            fmarks_check_one(&wp->w_jumplist[i], name, buf);
        }
    }
}

static void fmarks_check_one(xfilemark_st *fm, uchar_kt *name, fbuf_st *buf)
{
    if(fm->fmark.fnum == 0
       && fm->fname != NULL
       && fnamecmp(name, fm->fname) == 0)
    {
        fm->fmark.fnum = buf->b_fnum;
        xfree(fm->fname);
        fm->fname = NULL;
    }
}

/// Check a if a position from a mark is valid.
/// Give and error message and return FAIL if not.
int check_mark(apos_st *pos)
{
    if(pos == NULL)
    {
        EMSG(_(e_umark));
        return FAIL;
    }

    if(pos->lnum <= 0)
    {
        // lnum is negative if mark is in another file can can't
        // get that file, error message already give then.
        if(pos->lnum == 0)
        {
            EMSG(_(e_marknotset));
        }

        return FAIL;
    }

    if(pos->lnum > curbuf->b_ml.ml_line_count)
    {
        EMSG(_(e_markinval));
        return FAIL;
    }

    return OK;
}

/// Clear all marks and change list in the given buffer
///
/// Used mainly when trashing the entire buffer during ":e" type commands.
///
/// @param[out]  buf  Buffer to clear marks in.
void clrallmarks(fbuf_st *const buf)
FUNC_ATTR_NONNULL_ALL
{
    for(size_t i = 0; i < NMARKS; i++)
    {
        clear_fmark(&buf->b_namedm[i]);
    }

    clear_fmark(&buf->b_last_cursor);
    buf->b_last_cursor.mark.lnum = 1;
    clear_fmark(&buf->b_last_insert);
    clear_fmark(&buf->b_last_change);

    buf->b_op_start.lnum = 0; // start/end op mark cleared
    buf->b_op_end.lnum = 0;

    for(int i = 0; i < buf->b_changelistlen; i++)
    {
        clear_fmark(&buf->b_changelist[i]);
    }

    buf->b_changelistlen = 0;
}

/// Get name of file from a filemark.
/// When it's in the current buffer, return the text at the mark.
/// Returns an allocated string.
uchar_kt *fm_getname(filemark_st *fmark, int lead_len)
{
    if(fmark->fnum == curbuf->b_fnum) // current buffer
    {
        return mark_line(&(fmark->mark), lead_len);
    }

    return buflist_nr2name(fmark->fnum, FALSE, TRUE);
}

/// Return the line at mark "mp".  Truncate to fit in window.
/// The returned string has been allocated.
static uchar_kt *mark_line(apos_st *mp, int lead_len)
{
    uchar_kt *s, *p;
    int len;

    if(mp->lnum == 0 || mp->lnum > curbuf->b_ml.ml_line_count)
    {
        return vim_strsave((uchar_kt *)"-invalid-");
    }

    assert(Columns >= 0 && (size_t)Columns <= SIZE_MAX);

    s = vim_strnsave(skipwhite(ml_get(mp->lnum)), (size_t)Columns);
    // Truncate the line to fit it in the window
    len = 0;

    for(p = s; *p != NUL; mb_ptr_adv(p))
    {
        len += ptr2cells(p);

        if(len >= Columns - lead_len)
        {
            break;
        }
    }

    *p = NUL;
    return s;
}

/// print the marks
void do_marks(exargs_st *eap)
{
    int i;
    uchar_kt *name;
    uchar_kt *arg = eap->arg;

    if(arg != NULL && *arg == NUL)
    {
        arg = NULL;
    }

    show_one_mark('\'', arg, &curwin->w_pcmark, NULL, true);

    for(i = 0; i < NMARKS; ++i)
    {
        show_one_mark(i + 'a', arg, &curbuf->b_namedm[i].mark, NULL, true);
    }

    for(i = 0; i < NGLOBALMARKS; ++i)
    {
        if(namedfm[i].fmark.fnum != 0)
        {
            name = fm_getname(&namedfm[i].fmark, 15);
        }
        else
        {
            name = namedfm[i].fname;
        }

        if(name != NULL)
        {
            show_one_mark(i >= NMARKS ? i - NMARKS + '0' : i + 'A',
                          arg,
                          &namedfm[i].fmark.mark,
                          name,
                          namedfm[i].fmark.fnum == curbuf->b_fnum);

            if(namedfm[i].fmark.fnum != 0)
            {
                xfree(name);
            }
        }
    }

    show_one_mark('"', arg, &curbuf->b_last_cursor.mark, NULL, true);
    show_one_mark('[', arg, &curbuf->b_op_start, NULL, true);
    show_one_mark(']', arg, &curbuf->b_op_end, NULL, true);
    show_one_mark('^', arg, &curbuf->b_last_insert.mark, NULL, true);
    show_one_mark('.', arg, &curbuf->b_last_change.mark, NULL, true);
    show_one_mark('<', arg, &curbuf->b_visual.vi_start, NULL, true);
    show_one_mark('>', arg, &curbuf->b_visual.vi_end, NULL, true);
    show_one_mark(-1, arg, NULL, NULL, false);
}

/// @param c
/// @param arg
/// @param p
/// @param name
/// @param current  in current file
static void show_one_mark(int c,
                          uchar_kt *arg,
                          apos_st *p,
                          uchar_kt *name,
                          int current)
{
    static int did_title = FALSE;
    int mustfree = FALSE;

    if(c == -1) // finish up
    {
        if(did_title)
        {
            did_title = FALSE;
        }
        else
        {
            if(arg == NULL)
            {
                MSG(_("No marks set"));
            }
            else
            {
                EMSG2(_("E283: No marks matching \"%s\""), arg);
            }
        }
    }
    // don't output anything if 'q' typed at --more-- prompt
    else if(!got_int
            && (arg == NULL || vim_strchr(arg, c) != NULL)
            && p->lnum != 0)
    {
        if(!did_title)
        {
            // Highlight title
            MSG_PUTS_TITLE(_("\nmark line  col file/text"));
            did_title = TRUE;
        }

        msg_putchar('\n');

        if(!got_int)
        {
            sprintf((char *)IObuff, " %c %6ld %4d ", c, p->lnum, p->col);
            msg_outtrans(IObuff);

            if(name == NULL && current)
            {
                name = mark_line(p, 15);
                mustfree = TRUE;
            }

            if(name != NULL)
            {
                msg_outtrans_attr(name, current ? hl_attr(HLF_D) : 0);

                if(mustfree)
                {
                    xfree(name);
                }
            }
        }

        ui_flush(); // show one line at a time
    }
}

/// ":delmarks[!] [marks]"
void ex_delmarks(exargs_st *eap)
{
    uchar_kt *p;
    int from, to;
    int i;
    int lower;
    int digit;
    int n;

    if(*eap->arg == NUL && eap->forceit)
    {
        // clear all marks
        clrallmarks(curbuf);
    }
    else if(eap->forceit)
    {
        EMSG(_(e_invarg));
    }
    else if(*eap->arg == NUL)
    {
        EMSG(_(e_argreq));
    }
    else
    {
        // clear specified marks only
        for(p = eap->arg; *p != NUL; ++p)
        {
            lower = ASCII_ISLOWER(*p);
            digit = ascii_isdigit(*p);

            if(lower || digit || ASCII_ISUPPER(*p))
            {
                if(p[1] == '-')
                {
                    // clear range of marks
                    from = *p;
                    to = p[2];

                    if(!(lower ? ASCII_ISLOWER(p[2])
                         : (digit
                            ? ascii_isdigit(p[2])
                            : ASCII_ISUPPER(p[2])))
                       || to < from)
                    {
                        EMSG2(_(e_invarg2), p);
                        return;
                    }

                    p += 2;
                }
                else
                {
                    // clear one lower case mark
                    from = to = *p;
                }

                for(i = from; i <= to; ++i)
                {
                    if(lower)
                    {
                        curbuf->b_namedm[i - 'a'].mark.lnum = 0;
                    }
                    else
                    {
                        if(digit)
                        {
                            n = i - '0' + NMARKS;
                        }
                        else
                        {
                            n = i - 'A';
                        }

                        namedfm[n].fmark.mark.lnum = 0;
                        xfree(namedfm[n].fname);
                        namedfm[n].fname = NULL;
                    }
                }
            }
            else
                switch(*p)
                {
                    case '"':
                        CLEAR_FMARK(&curbuf->b_last_cursor);
                        break;

                    case '^':
                        CLEAR_FMARK(&curbuf->b_last_insert);
                        break;

                    case '.':
                        CLEAR_FMARK(&curbuf->b_last_change);
                        break;

                    case '[':
                        curbuf->b_op_start.lnum    = 0;
                        break;

                    case ']':
                        curbuf->b_op_end.lnum      = 0;
                        break;

                    case '<':
                        curbuf->b_visual.vi_start.lnum = 0;
                        break;

                    case '>':
                        curbuf->b_visual.vi_end.lnum   = 0;
                        break;

                    case ' ':
                        break;

                    default:
                        EMSG2(_(e_invarg2), p);
                        return;
                }
        }
    }
}

/// print the jumplist
void ex_jumps(exargs_st *FUNC_ARGS_UNUSED_REALY(exarg_ptr))
{
    int i;
    uchar_kt *name;
    cleanup_jumplist();

    // Highlight title
    MSG_PUTS_TITLE(_("\n jump line  col file/text"));

    for(i = 0; i < curwin->w_jumplistlen && !got_int; ++i)
    {
        if(curwin->w_jumplist[i].fmark.mark.lnum != 0)
        {
            if(curwin->w_jumplist[i].fmark.fnum == 0)
            {
                fname2fnum(&curwin->w_jumplist[i]);
            }

            name = fm_getname(&curwin->w_jumplist[i].fmark, 16);

            if(name == NULL) // file name not available
            {
                continue;
            }

            msg_putchar('\n');

            if(got_int)
            {
                xfree(name);
                break;
            }

            sprintf((char *)IObuff,
                    "%c %2d %5ld %4d ",
                    i == curwin->w_jumplistidx ? '>' : ' ',
                    i > curwin->w_jumplistidx
                    ? i - curwin->w_jumplistidx
                    : curwin->w_jumplistidx - i,
                    curwin->w_jumplist[i].fmark.mark.lnum,
                    curwin->w_jumplist[i].fmark.mark.col);

            msg_outtrans(IObuff);

            msg_outtrans_attr(name,
                              curwin->w_jumplist[i].fmark.fnum == curbuf->b_fnum
                              ? hl_attr(HLF_D) : 0);
            xfree(name);
            os_breakcheck();
        }

        ui_flush();
    }

    if(curwin->w_jumplistidx == curwin->w_jumplistlen)
    {
        MSG_PUTS("\n>");
    }
}

void ex_clearjumps(exargs_st *FUNC_ARGS_UNUSED_REALY(exarg_ptr))
{
    free_jumplist(curwin);
    curwin->w_jumplistlen = 0;
    curwin->w_jumplistidx = 0;
}

/// print the changelist
void ex_changes(exargs_st *FUNC_ARGS_UNUSED_REALY(exarg_ptr))
{
    int i;
    uchar_kt *name;

    // Highlight title
    MSG_PUTS_TITLE(_("\nchange line  col text"));

    for(i = 0; i < curbuf->b_changelistlen && !got_int; ++i)
    {
        if(curbuf->b_changelist[i].mark.lnum != 0)
        {
            msg_putchar('\n');

            if(got_int)
            {
                break;
            }

            sprintf((char *)IObuff,
                    "%c %3d %5ld %4d ",
                    i == curwin->w_changelistidx ? '>' : ' ',
                    i > curwin->w_changelistidx
                    ? i - curwin->w_changelistidx
                    : curwin->w_changelistidx - i,
                    (long)curbuf->b_changelist[i].mark.lnum,
                    curbuf->b_changelist[i].mark.col);

            msg_outtrans(IObuff);
            name = mark_line(&curbuf->b_changelist[i].mark, 17);
            msg_outtrans_attr(name, hl_attr(HLF_D));
            xfree(name);
            os_breakcheck();
        }

        ui_flush();
    }

    if(curwin->w_changelistidx == curbuf->b_changelistlen)
    {
        MSG_PUTS("\n>");
    }
}

#define one_adjust(add)                      \
    {                                        \
        lp = add;                            \
        if(*lp >= line1 && *lp <= line2)     \
        {                                    \
            if(amount == MAXLNUM)            \
            {                                \
                *lp = 0;                     \
            }                                \
            else                             \
            {                                \
                *lp += amount;               \
            }                                \
        }                                    \
        else if(amount_after && *lp > line2) \
        {                                    \
            *lp += amount_after;             \
        }                                    \
    }

/// don't delete the line, just put at first deleted line
#define one_adjust_nodel(add)                \
    {                                        \
        lp = add;                            \
        if(*lp >= line1 && *lp <= line2)     \
        {                                    \
            if(amount == MAXLNUM)            \
            {                                \
                *lp = line1;                 \
            }                                \
            else                             \
            {                                \
                *lp += amount;               \
            }                                \
        }                                    \
        else if(amount_after && *lp > line2) \
        {                                    \
            *lp += amount_after;             \
        }                                    \
    }

/// Adjust marks between line1 and line2 (inclusive) to move 'amount' lines.
///
/// Must be called before changed_*(), appended_lines() or deleted_lines().
/// May be called before or after changing the text.
///
/// When deleting lines line1 to line2, use an 'amount' of MAXLNUM:
/// The marks within this range are made invalid.
///
/// If 'amount_after' is non-zero adjust marks after line2.
///
/// - Example: Delete lines 34 and 35: mark_adjust(34, 35, MAXLNUM, -2);
/// - Example: Insert two lines below 55: mark_adjust(56, MAXLNUM, 2, 0);
///            or: mark_adjust(56, 55, MAXLNUM, 2);
void mark_adjust(linenum_kt line1,
                 linenum_kt line2,
                 long amount,
                 long amount_after)
{
    mark_adjust_internal(line1, line2, amount, amount_after, true);
}

// mark_adjust_nofold() does the same as mark_adjust() but without adjusting
// folds in any way. Folds must be adjusted manually by the caller.
// This is only useful when folds need to be moved in a way different to
// calling foldMarkAdjust() with arguments line1, line2, amount, amount_after,
// for an example of why this may be necessary, see do_move().
void mark_adjust_nofold(linenum_kt line1,
                        linenum_kt line2,
                        long amount,
                        long amount_after)
{
    mark_adjust_internal(line1, line2, amount, amount_after, false);
}

static void mark_adjust_internal(linenum_kt line1,
                                 linenum_kt line2,
                                 long amount,
                                 long amount_after,
                                 bool adjust_folds)
{
    int i;
    int fnum = curbuf->b_fnum;
    linenum_kt *lp;
    static apos_st initpos = INIT_POS_T(1, 0, 0);

    if(line2 < line1 && amount_after == 0L) // nothing to do
    {
        return;
    }

    if(!cmdmod.lockmarks)
    {
        // named marks, lower case and upper case
        for(i = 0; i < NMARKS; i++)
        {
            one_adjust(&(curbuf->b_namedm[i].mark.lnum));

            if(namedfm[i].fmark.fnum == fnum)
            {
                one_adjust_nodel(&(namedfm[i].fmark.mark.lnum));
            }
        }

        for(i = NMARKS; i < NGLOBALMARKS; i++)
        {
            if(namedfm[i].fmark.fnum == fnum)
            {
                one_adjust_nodel(&(namedfm[i].fmark.mark.lnum));
            }
        }

        // last Insert position
        one_adjust(&(curbuf->b_last_insert.mark.lnum));

        // last change position
        one_adjust(&(curbuf->b_last_change.mark.lnum));

        // last cursor position, if it was set
        if(!equalpos(curbuf->b_last_cursor.mark, initpos))
        {
            one_adjust(&(curbuf->b_last_cursor.mark.lnum));
        }

        // list of change positions
        for(i = 0; i < curbuf->b_changelistlen; ++i)
        {
            one_adjust_nodel(&(curbuf->b_changelist[i].mark.lnum));
        }

        // Visual area
        one_adjust_nodel(&(curbuf->b_visual.vi_start.lnum));
        one_adjust_nodel(&(curbuf->b_visual.vi_end.lnum));

        // quickfix marks
        qf_mark_adjust(NULL, line1, line2, amount, amount_after);

        // location lists
        FOR_ALL_TAB_WINDOWS(tab, win)
        {
            qf_mark_adjust(win, line1, line2, amount, amount_after);
        }

        sign_mark_adjust(line1, line2, amount, amount_after);
        bufhl_mark_adjust(curbuf, line1, line2, amount, amount_after);
    }

    // previous context mark
    one_adjust(&(curwin->w_pcmark.lnum));

    // previous pcmark
    one_adjust(&(curwin->w_prev_pcmark.lnum));

    // saved cursor for formatting
    if(saved_cursor.lnum != 0)
    {
        one_adjust_nodel(&(saved_cursor.lnum));
    }

    // Adjust items in all windows related to the current buffer.
    FOR_ALL_TAB_WINDOWS(tab, win)
    {
        if(!cmdmod.lockmarks)
        {
            // Marks in the jumplist. When deleting lines, this may create
            // duplicate marks in the jumplist, they will be removed later.
            for(i = 0; i < win->w_jumplistlen; ++i)
            {
                if(win->w_jumplist[i].fmark.fnum == fnum)
                {
                    one_adjust_nodel(&(win->w_jumplist[i].fmark.mark.lnum));
                }
            }
        }

        if(win->w_buffer == curbuf)
        {
            if(!cmdmod.lockmarks)
            {
                // marks in the tag stack
                for(i = 0; i < win->w_tagstacklen; i++)
                {
                    if(win->w_tagstack[i].fmark.fnum == fnum)
                    {
                        one_adjust_nodel(&(win->w_tagstack[i].fmark.mark.lnum));
                    }
                }
            }

            // the displayed Visual area
            if(win->w_old_cursor_lnum != 0)
            {
                one_adjust_nodel(&(win->w_old_cursor_lnum));
                one_adjust_nodel(&(win->w_old_visual_lnum));
            }

            // topline and cursor position for windows with
            // the same buffer other than the current window
            if(win != curwin)
            {
                if(win->w_topline >= line1 && win->w_topline <= line2)
                {
                    if(amount == MAXLNUM) // topline is deleted
                    {
                        if(line1 <= 1)
                        {
                            win->w_topline = 1;
                        }
                        else
                        {
                            win->w_topline = line1 - 1;
                        }
                    }
                    else // keep topline on the same line
                    {
                        win->w_topline += amount;
                    }

                    win->w_topfill = 0;
                }
                else if(amount_after && win->w_topline > line2)
                {
                    win->w_topline += amount_after;
                    win->w_topfill = 0;
                }

                if(win->w_cursor.lnum >= line1 && win->w_cursor.lnum <= line2)
                {
                    if(amount == MAXLNUM) // line with cursor is deleted
                    {
                        if(line1 <= 1)
                        {
                            win->w_cursor.lnum = 1;
                        }
                        else
                        {
                            win->w_cursor.lnum = line1 - 1;
                        }

                        win->w_cursor.col = 0;
                    }
                    else // keep cursor on the same line
                    {
                        win->w_cursor.lnum += amount;
                    }
                }
                else if(amount_after && win->w_cursor.lnum > line2)
                {
                    win->w_cursor.lnum += amount_after;
                }
            }

            if(adjust_folds)
            {
                foldMarkAdjust(win, line1, line2, amount, amount_after);
            }
        }
    }

    // adjust diffs
    diff_mark_adjust(line1, line2, amount, amount_after);
}

/// This code is used often, needs to be fast.
#define col_adjust(pp)                                              \
    {                                                               \
        posp = pp;                                                  \
        if(posp->lnum == lnum && posp->col >= mincol)               \
        {                                                           \
            posp->lnum += lnum_amount;                              \
            assert(col_amount > INT_MIN && col_amount <= INT_MAX);  \
            if(col_amount < 0 && posp->col <= (columnum_kt)-col_amount) \
            {                                                       \
                posp->col = 0;                                      \
            }                                                       \
            else                                                    \
            {                                                       \
                posp->col += (columnum_kt)col_amount;                   \
            }                                                       \
        }                                                           \
    }

/// Adjust marks in line "lnum" at column "mincol" and further:
/// add "lnum_amount" to the line number and add "col_amount" to
/// the column position.
void mark_col_adjust(linenum_kt lnum,
                     columnum_kt mincol,
                     long lnum_amount,
                     long col_amount)
{
    int i;
    int fnum = curbuf->b_fnum;
    apos_st *posp;

    if((col_amount == 0L && lnum_amount == 0L) || cmdmod.lockmarks)
    {
        return; // nothing to do
    }

    // named marks, lower case and upper case
    for(i = 0; i < NMARKS; i++)
    {
        col_adjust(&(curbuf->b_namedm[i].mark));

        if(namedfm[i].fmark.fnum == fnum)
        {
            col_adjust(&(namedfm[i].fmark.mark));
        }
    }

    for(i = NMARKS; i < NGLOBALMARKS; i++)
    {
        if(namedfm[i].fmark.fnum == fnum)
        {
            col_adjust(&(namedfm[i].fmark.mark));
        }
    }

    // last Insert position
    col_adjust(&(curbuf->b_last_insert.mark));

    // last change position
    col_adjust(&(curbuf->b_last_change.mark));

    // list of change positions
    for(i = 0; i < curbuf->b_changelistlen; ++i)
    {
        col_adjust(&(curbuf->b_changelist[i].mark));
    }

    // Visual area
    col_adjust(&(curbuf->b_visual.vi_start));
    col_adjust(&(curbuf->b_visual.vi_end));

    // previous context mark
    col_adjust(&(curwin->w_pcmark));

    // previous pcmark
    col_adjust(&(curwin->w_prev_pcmark));

    // saved cursor for formatting
    col_adjust(&saved_cursor);

    // Adjust items in all windows related to the current buffer.
    FOR_ALL_WINDOWS_IN_TAB(win, curtab)
    {
        // marks in the jumplist
        for(i = 0; i < win->w_jumplistlen; ++i)
        {
            if(win->w_jumplist[i].fmark.fnum == fnum)
            {
                col_adjust(&(win->w_jumplist[i].fmark.mark));
            }
        }

        if(win->w_buffer == curbuf)
        {
            // marks in the tag stack
            for(i = 0; i < win->w_tagstacklen; i++)
            {
                if(win->w_tagstack[i].fmark.fnum == fnum)
                {
                    col_adjust(&(win->w_tagstack[i].fmark.mark));
                }
            }

            // cursor position for other windows with the same buffer
            if(win != curwin)
            {
                col_adjust(&win->w_cursor);
            }
        }
    }
}

/// When deleting lines, this may create duplicate marks in the
/// jumplist. They will be removed here for the current window.
void cleanup_jumplist(void)
{
    int i;
    int from, to;
    to = 0;

    for(from = 0; from < curwin->w_jumplistlen; ++from)
    {
        if(curwin->w_jumplistidx == from)
        {
            curwin->w_jumplistidx = to;
        }

        for(i = from + 1; i < curwin->w_jumplistlen; ++i)
        {
            if(curwin->w_jumplist[i].fmark.fnum
               == curwin->w_jumplist[from].fmark.fnum
               && curwin->w_jumplist[from].fmark.fnum != 0
               && curwin->w_jumplist[i].fmark.mark.lnum
               == curwin->w_jumplist[from].fmark.mark.lnum)
            {
                break;
            }
        }

        if(i >= curwin->w_jumplistlen) // no duplicate
        {
            if(to != from)
            {
                // Not using curwin->w_jumplist[to++] = curwin->w_jumplist[from]
                // because this way valgrind complains about overlapping source
                // and destination in memcpy() call. (clang-3.6.0, debug build
                // with -DEXITFREE).
                curwin->w_jumplist[to] = curwin->w_jumplist[from];
            }

            to++;
        }
        else
        {
            xfree(curwin->w_jumplist[from].fname);
        }
    }

    if(curwin->w_jumplistidx == curwin->w_jumplistlen)
    {
        curwin->w_jumplistidx = to;
    }

    curwin->w_jumplistlen = to;
}

/// Copy the jumplist from window "from" to window "to".
void copy_jumplist(win_st *from, win_st *to)
{
    int i;

    for(i = 0; i < from->w_jumplistlen; ++i)
    {
        to->w_jumplist[i] = from->w_jumplist[i];

        if(from->w_jumplist[i].fname != NULL)
        {
            to->w_jumplist[i].fname = vim_strsave(from->w_jumplist[i].fname);
        }
    }

    to->w_jumplistlen = from->w_jumplistlen;
    to->w_jumplistidx = from->w_jumplistidx;
}

/// Iterate over jumplist items
///
/// @warning
/// No jumplist-editing functions must be
/// run while iteration is in progress.
///
/// @param[in]  iter  Iterator. Pass NULL to start iteration.
/// @param[in]  win   Window for which jump list is processed.
/// @param[out] fm    Item definition.
///
/// @return
/// Pointer that needs to be passed to next
/// 'mark_jumplist_iter' call or NULL if iteration is over.
const void *mark_jumplist_iter(const void *const iter,
                               const win_st *const win,
                               xfilemark_st *const fm)
FUNC_ATTR_NONNULL_ARG(2, 3)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    if(iter == NULL && win->w_jumplistlen == 0)
    {
        *fm = (xfilemark_st) { { { 0, 0, 0 }, 0, 0, NULL}, NULL };
        return NULL;
    }

    const xfilemark_st *const iter_mark = (iter == NULL
                                       ? &(win->w_jumplist[0])
                                       : (const xfilemark_st *const) iter);

    *fm = *iter_mark;

    if(iter_mark == &(win->w_jumplist[win->w_jumplistlen - 1]))
    {
        return NULL;
    }
    else
    {
        return iter_mark + 1;
    }
}

/// Iterate over global marks
///
/// @warning
/// No mark-editing functions must be run while
/// iteration is in progress.
///
/// @param[in]   iter  Iterator. Pass NULL to start iteration.
/// @param[out]  name  Mark name.
/// @param[out]  fm    Mark definition.
///
/// @return
/// Pointer that needs to be passed to next
/// 'mark_global_iter' call or NULL if iteration is over.
const void *mark_global_iter(const void *const iter,
                             char *const name,
                             xfilemark_st *const fm)
FUNC_ATTR_NONNULL_ARG(2, 3)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    *name = NUL;
    const xfilemark_st *iter_mark = (iter == NULL
                                 ? &(namedfm[0])
                                 : (const xfilemark_st *const) iter);

    while((size_t)(iter_mark - &(namedfm[0])) < ARRAY_SIZE(namedfm)
          && !iter_mark->fmark.mark.lnum)
    {
        iter_mark++;
    }

    if((size_t)(iter_mark - &(namedfm[0])) == ARRAY_SIZE(namedfm)
       || !iter_mark->fmark.mark.lnum)
    {
        return NULL;
    }

    size_t iter_off = (size_t)(iter_mark - &(namedfm[0]));

    *name = (char)(iter_off < NMARKS
                   ? 'A' + (char) iter_off
                   : '0' + (char)(iter_off - NMARKS));

    *fm = *iter_mark;

    while((size_t)(++iter_mark - &(namedfm[0])) < ARRAY_SIZE(namedfm))
    {
        if(iter_mark->fmark.mark.lnum)
        {
            return (const void *) iter_mark;
        }
    }

    return NULL;
}

/// Get next mark and its name
///
/// @param[in] buf
/// Buffer for which next mark is taken.
///
/// @param[in,out] mark_name
/// Pointer to the current mark name. Next mark name
/// will be saved at this address as well.
///
/// Current mark name must either be NUL, '"', '^',
/// '.' or 'a' .. 'z'. If it is neither of these
/// behaviour is undefined.
///
/// @return Pointer to the next mark or NULL.
static inline const filemark_st *next_buffer_mark(const fbuf_st *const buf,
                                              char *const mark_name)
FUNC_ATTR_NONNULL_ALL
FUNC_ATTR_WARN_UNUSED_RESULT
{
    switch(*mark_name)
    {
        case NUL:
        {
            *mark_name = '"';
            return &(buf->b_last_cursor);
        }

        case '"':
        {
            *mark_name = '^';
            return &(buf->b_last_insert);
        }

        case '^':
        {
            *mark_name = '.';
            return &(buf->b_last_change);
        }

        case '.':
        {
            *mark_name = 'a';
            return &(buf->b_namedm[0]);
        }

        case 'z':
        {
            return NULL;
        }

        default:
        {
            (*mark_name)++;
            return &(buf->b_namedm[*mark_name - 'a']);
        }
    }
}

/// Iterate over buffer marks
///
/// @warning
/// No mark-editing functions must be run while iteration is in progress.
///
/// @param[in]   iter  Iterator. Pass NULL to start iteration.
/// @param[in]   buf   Buffer.
/// @param[out]  name  Mark name.
/// @param[out]  fm    Mark definition.
///
/// @return
/// Pointer that needs to be passed to next
/// 'mark_buffer_iter' call or NULL if iteration is over.
const void *mark_buffer_iter(const void *const iter,
                             const fbuf_st *const buf,
                             char *const name,
                             filemark_st *const fm)
FUNC_ATTR_NONNULL_ARG(2, 3, 4)
FUNC_ATTR_WARN_UNUSED_RESULT
{
    *name = NUL;

    char mark_name = (char)(iter == NULL
                            ? NUL
                            : (iter == &(buf->b_last_cursor)
                               ? '"'
                               : (iter == &(buf->b_last_insert)
                                  ? '^'
                                  : (iter == &(buf->b_last_change)
                                     ? '.'
                                     : 'a' + (char)((const filemark_st *)iter
                                                    - &(buf->b_namedm[0]))))));

    const filemark_st *iter_mark = next_buffer_mark(buf, &mark_name);

    while(iter_mark != NULL && iter_mark->mark.lnum == 0)
    {
        iter_mark = next_buffer_mark(buf, &mark_name);
    }

    if(iter_mark == NULL)
    {
        return NULL;
    }

    size_t iter_off = (size_t)(iter_mark - &(buf->b_namedm[0]));

    if(mark_name)
    {
        *name = mark_name;
    }
    else
    {
        *name = (char)('a' + (char) iter_off);
    }

    *fm = *iter_mark;
    return (const void *) iter_mark;
}

/// Set global mark
///
/// @param[in] name
/// Mark name.
///
/// @param[in] fm
/// Mark to be set.
///
/// @param[in] update
/// If true then only set global mark if
/// it was created later then existing one.
///
/// @return true on success, false on failure.
bool mark_set_global(const char name, const xfilemark_st fm, const bool update)
{
    const int idx = mark_global_index(name);

    if(idx == -1)
    {
        return false;
    }

    xfilemark_st *const fm_tgt = &(namedfm[idx]);

    if(update && fm.fmark.timestamp <= fm_tgt->fmark.timestamp)
    {
        return false;
    }

    if(fm_tgt->fmark.mark.lnum != 0)
    {
        free_xfmark(*fm_tgt);
    }

    *fm_tgt = fm;
    return true;
}

/// Set local mark
///
/// @param[in] name
/// Mark name.
///
/// @param[in] buf
/// Pointer to the buffer to set mark in.
///
/// @param[in] fm
/// Mark to be set.
///
/// @param[in] update
/// If true then only set global mark if it
/// was created later then existing one.
///
/// @return true on success, false on failure.
bool mark_set_local(const char name,
                    fbuf_st *const buf,
                    const filemark_st fm,
                    const bool update)
FUNC_ATTR_NONNULL_ALL
{
    filemark_st *fm_tgt = NULL;

    if(ASCII_ISLOWER(name))
    {
        fm_tgt = &(buf->b_namedm[name - 'a']);
    }
    else if(name == '"')
    {
        fm_tgt = &(buf->b_last_cursor);
    }
    else if(name == '^')
    {
        fm_tgt = &(buf->b_last_insert);
    }
    else if(name == '.')
    {
        fm_tgt = &(buf->b_last_change);
    }
    else
    {
        return false;
    }

    if(update && fm.timestamp <= fm_tgt->timestamp)
    {
        return false;
    }

    if(fm_tgt->mark.lnum != 0)
    {
        free_fmark(*fm_tgt);
    }

    *fm_tgt = fm;
    return true;
}

/// Free items in the jumplist of window "wp".
void free_jumplist(win_st *wp)
{
    int i;

    for(i = 0; i < wp->w_jumplistlen; ++i)
    {
        free_xfmark(wp->w_jumplist[i]);
    }

    wp->w_jumplistlen = 0;
}

void set_last_cursor(win_st *win)
{
    if(win->w_buffer != NULL)
    {
        RESET_FMARK(&win->w_buffer->b_last_cursor, win->w_cursor, 0);
    }
}

#if defined(EXITFREE)
void free_all_marks(void)
{
    int i;

    for(i = 0; i < NGLOBALMARKS; i++)
    {
        if(namedfm[i].fmark.mark.lnum != 0)
        {
            free_xfmark(namedfm[i]);
        }
    }

    memset(&namedfm[0], 0, sizeof(namedfm));
}
#endif

/// Adjust position to point to the first byte of a multi-byte character
///
/// If it points to a tail byte it is move backwards to the head byte.
///
/// @param[in]  buf  Buffer to adjust position in.
/// @param[out]  lp  Position to adjust.
void mark_mb_adjustpos(fbuf_st *buf, apos_st *lp)
FUNC_ATTR_NONNULL_ALL
{
    if(lp->col > 0 || lp->coladd > 1)
    {
        const uchar_kt *const p = ml_get_buf(buf, lp->lnum, false);
        lp->col -= (*mb_head_off)(p, p + lp->col);

        // Reset "coladd" when the cursor would be
        // on the right half of a double-wide character.
        if(lp->coladd == 1
           && p[lp->col] != TAB
           && vim_isprintc((*mb_ptr2char)(p + lp->col))
           && ptr2cells(p + lp->col) > 1)
        {
            lp->coladd = 0;
        }
    }
}
