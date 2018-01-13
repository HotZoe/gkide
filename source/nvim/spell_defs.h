/// @file nvim/spell_defs.h

#ifndef NVIM_SPELL_DEFS_H
#define NVIM_SPELL_DEFS_H

#include <stdbool.h>
#include <stdint.h>

#include "nvim/buffer_defs.h"
#include "nvim/garray.h"
#include "nvim/regexp_defs.h"
#include "nvim/types.h"

// Some places assume a word length fits
// in a byte, thus it can't be above 255.
#define MAXWLEN   254  ///< Assume max. word len is this many bytes.

/// Type used for indexes in the word tree need to be at least 4 bytes.
/// If int is 8 bytes we could use something smaller, but what?
typedef int idx_T;

#define SPL_FNAME_TMPL   "%s.%s.spl"
#define SPL_FNAME_ADD    ".add."
#define SPL_FNAME_ASCII  ".ascii."

// Flags used for a word.
// Only the lowest byte can be used, the region byte comes above it.
#define WF_REGION   0x01   ///< region byte follows
#define WF_ONECAP   0x02   ///< word with one capital (or all capitals)
#define WF_ALLCAP   0x04   ///< word must be all capitals
#define WF_RARE     0x08   ///< rare word
#define WF_BANNED   0x10   ///< bad word
#define WF_AFX      0x20   ///< affix ID follows
#define WF_FIXCAP   0x40   ///< keep-case word, allcap not allowed
#define WF_KEEPCAP  0x80   ///< keep-case word

// for <flags2>, shifted up one byte to be used in wn_flags
#define WF_HAS_AFF    0x0100   ///< word includes affix
#define WF_NEEDCOMP   0x0200   ///< word only valid in compound
#define WF_NOSUGGEST  0x0400   ///< word not to be suggested
#define WF_COMPROOT   0x0800   ///< already compounded word, COMPOUNDROOT
#define WF_NOCOMPBEF  0x1000   ///< no compounding before this word
#define WF_NOCOMPAFT  0x2000   ///< no compounding after this word

// flags for <pflags>
#define WFP_RARE         0x01  ///< rare prefix
#define WFP_NC           0x02  ///< prefix is not combining
#define WFP_UP           0x04  ///< to-upper prefix
#define WFP_COMPPERMIT   0x08  ///< prefix with COMPOUNDPERMITFLAG
#define WFP_COMPFORBID   0x10  ///< prefix with COMPOUNDFORBIDFLAG

// Flags for postponed prefixes in "sl_pidxs".
// Must be above affixID (one byte) and prefcondnr (two bytes).
#define WF_RAREPFX   (WFP_RARE << 24)  ///< rare postponed prefix
#define WF_PFX_NC    (WFP_NC << 24)    ///< non-combining postponed prefix
#define WF_PFX_UP    (WFP_UP << 24)    ///< to-upper postponed prefix

#define WF_PFX_COMPPERMIT  (WFP_COMPPERMIT << 24)  ///< postponed prefix with
#define WF_PFX_COMPFORBID  (WFP_COMPFORBID << 24)  ///< postponed prefix with

// flags for <compoptions>
#define COMP_CHECKDUP       1  ///< CHECKCOMPOUNDDUP
#define COMP_CHECKREP       2  ///< CHECKCOMPOUNDREP
#define COMP_CHECKCASE      4  ///< CHECKCOMPOUNDCASE
#define COMP_CHECKTRIPLE    8  ///< CHECKCOMPOUNDTRIPLE

/// Info from "REP", "REPSAL" and "SAL" entries in ".aff"
/// file used in si_rep, si_repsal, sl_rep, and si_sal.
/// Not for sl_sal! One replacement: from "ft_from" to "ft_to".
typedef struct fromto_s
{
    uchar_kt *ft_from;
    uchar_kt *ft_to;
} fromto_st;

/// Info from "SAL" entries in ".aff" file used in sl_sal.
/// The info is split for quick processing by spell_soundfold().
/// Note that "sm_oneof" and "sm_rules" point into sm_lead.
typedef struct salitem_s
{
    uchar_kt *sm_lead;  ///< leading letters
    int sm_leadlen;     ///< length of "sm_lead"
    uchar_kt *sm_oneof; ///< letters from () or NULL
    uchar_kt *sm_rules; ///< rules like ^, $, priority
    uchar_kt *sm_to;    ///< replacement.
    int *sm_lead_w;     ///< wide character copy of "sm_lead"
    int *sm_oneof_w;    ///< wide character copy of "sm_oneof"
    int *sm_to_w;       ///< wide character copy of "sm_to"
} salitem_T;

typedef int salfirst_T;

// Values for SP_*ERROR are negative,
// positive values are used by read_cnt_string().
#define SP_TRUNCERROR   -1   ///< spell file truncated error
#define SP_FORMERROR    -2   ///< format error in spell file
#define SP_OTHERERROR   -3   ///< other error while reading spell file

/// Structure used to store words and other info for one language,
/// loaded from a .spl file. The main access is through the tree in
/// "sl_fbyts/sl_fidxs", storing the case-folded words.
/// "sl_kbyts/sl_kidxs" is for keep-case words.
///
/// The "byts" array stores the possible bytes in each tree node,
/// preceded by the number of possible bytes, sorted on byte value:
///      <len> <byte1> <byte2> ...
///
/// The "idxs" array stores the index of the child node corresponding to
/// the byte in "byts". Exception: when the byte is zero, the word may end
/// here and "idxs" holds the flags, region mask and affixID for the word.
/// There may be several zeros in sequence for alternative
/// flag/region/affixID combinations.
typedef struct slang_S slang_T;

struct slang_S
{
    slang_T *sl_next;  ///< next language
    uchar_kt *sl_name;   ///< language name "en", "en.rare", "nl", etc.
    uchar_kt *sl_fname;  ///< name of .spl file
    bool sl_add;       ///< true if it's a .add file.

    uchar_kt *sl_fbyts;  ///< case-folded word bytes
    idx_T *sl_fidxs;   ///< case-folded word indexes
    uchar_kt *sl_kbyts;  ///< keep-case word bytes
    idx_T *sl_kidxs;   ///< keep-case word indexes
    uchar_kt *sl_pbyts;  ///< prefix tree word bytes
    idx_T *sl_pidxs;   ///< prefix tree word indexes

    uchar_kt *sl_info;         ///< infotext string or NULL
    uchar_kt sl_regions[17];   ///< table with up to 8 region names plus NUL
    uchar_kt *sl_midword;      ///< MIDWORD string or NULL
    hashtable_st sl_wordcount;  ///< hashtable with word count, wordcount_T

    int sl_compmax;          ///< COMPOUNDWORDMAX (default: MAXWLEN)
    int sl_compminlen;       ///< COMPOUNDMIN (default: 0)
    int sl_compsylmax;       ///< COMPOUNDSYLMAX (default: MAXWLEN)
    int sl_compoptions;      ///< COMP_* flags
    garray_st sl_comppat;     ///< CHECKCOMPOUNDPATTERN items
    regprog_st *sl_compprog;  ///< COMPOUNDRULE turned into a regexp progrm
                             ///< (NULL when no compounding)
    uchar_kt *sl_comprules;      ///< all COMPOUNDRULE concatenated (or NULL)
    uchar_kt *sl_compstartflags; ///< flags for first compound word
    uchar_kt *sl_compallflags;   ///< all flags for compound words
    bool sl_nobreak;           ///< When true: no spaces between words
    uchar_kt *sl_syllable;       ///< SYLLABLE repeatable chars or NULL
    garray_st sl_syl_items;     ///< syllable items

    int sl_prefixcnt;          ///< number of items in "sl_prefprog"
    regprog_st **sl_prefprog;   ///< table with regprogs for prefixes

    /// list of fromto_st entries from REP lines
    garray_st sl_rep;
    /// indexes where byte first appears, -1 if there is none
    int16_t sl_rep_first[256];
    /// list of salitem_T entries from SAL lines
    garray_st sl_sal;
    /// indexes where byte first appears, -1 if there is none
    salfirst_T sl_sal_first[256];

    bool sl_followup;     ///< SAL followup
    bool sl_collapse;     ///< SAL collapse_result
    bool sl_rem_accents;  ///< SAL remove_accents

    /// SOFOFROM and SOFOTO instead of SAL items:
    /// - @b sl_sal_first maps chars, when has_mbyte
    /// - @b sl_sal is a list of wide char lists.
    bool sl_sofo;

    garray_st sl_repsal; ///< list of fromto_st entries from REPSAL lines
    int16_t sl_repsal_first[256]; ///< sl_rep_first for REPSAL lines
    bool sl_nosplitsugs; ///< don't suggest splitting a word
    bool sl_nocompoundsugs; ///< don't suggest compounding

    // Info from the .sug file. Loaded on demand.
    time_t sl_sugtime;  ///< timestamp for .sug file
    uchar_kt *sl_sbyts;   ///< soundfolded word bytes
    idx_T *sl_sidxs;    ///< soundfolded word indexes
    filebuf_st *sl_sugbuf;   ///< buffer with word number table
    bool sl_sugloaded;  ///< true when .sug file was loaded or failed to load

    bool sl_has_map;         ///< true, if there is a MAP line
    hashtable_st sl_map_hash;   ///< MAP for multi-byte chars
    int sl_map_array[256];   ///< MAP for first 256 chars
    hashtable_st sl_sounddone;  ///< table with soundfolded words that have
                             ///< handled, see add_sound_suggest()
};

/// Structure used in "b_langp", filled from 'spelllang'.
typedef struct langp_S
{
    slang_T *lp_slang;    ///< info for this language
    slang_T *lp_sallang;  ///< language used for sound folding or NULL
    slang_T *lp_replang;  ///< language used for REP items or NULL
    int lp_region;        ///< bitmask for region or REGION_ALL
} langp_T;

#define LANGP_ENTRY(ga, i)   (((langp_T *)(ga).ga_data) + (i))

/// string at start of Vim .sug file
#define VIMSUGMAGIC    "VIMsug"

#define VIMSUGMAGICL   6   ///<
#define VIMSUGVERSION  1   ///<

/// word valid in all regions
#define REGION_ALL  0xff

/// The tables used for recognizing word characters according to spelling.
/// These are only used for the first 256 characters of 'encoding'.
typedef struct
{
    bool st_isw[256];      ///< flags: is word char
    bool st_isu[256];      ///< flags: is uppercase char
    uchar_kt st_fold[256];   ///< chars: folded case
    uchar_kt st_upper[256];  ///< chars: upper case
} spelltab_T;

/// For finding suggestions:
/// At each node in the tree these states are tried:
typedef enum
{
    /// At start of node check for NUL bytes (goodword ends);
    /// if badword ends there is a match, otherwise try splitting word.
    STATE_START = 0,

    STATE_NOPREFIX,    ///< try without prefix
    STATE_SPLITUNDO,   ///< Undo splitting.
    STATE_ENDNUL,      ///< Past NUL bytes at start of the node.
    STATE_PLAIN,       ///< Use each byte of the node.
    STATE_DEL,         ///< Delete a byte from the bad word.
    STATE_INS_PREP,    ///< Prepare for inserting bytes.
    STATE_INS,         ///< Insert a byte in the bad word.
    STATE_SWAP,        ///< Swap two bytes.
    STATE_UNSWAP,      ///< Undo swap two characters.
    STATE_SWAP3,       ///< Swap two characters over three.
    STATE_UNSWAP3,     ///< Undo Swap two characters over three.
    STATE_UNROT3L,     ///< Undo rotate three characters left
    STATE_UNROT3R,     ///< Undo rotate three characters right
    STATE_REP_INI,     ///< Prepare for using REP items.
    STATE_REP,         ///< Use matching REP items from the .aff file.
    STATE_REP_UNDO,    ///< Undo a REP item replacement.
    STATE_FINAL        ///< End of this node.
} state_T;

/// Struct to keep the state at each level in suggest_try_change().
typedef struct trystate_S
{
    state_T ts_state;        ///< state at this level, STATE_
    int ts_score;            ///< score
    idx_T ts_arridx;         ///< index in tree array, start of node
    short ts_curi;           ///< index in list of child nodes
    uchar_kt ts_fidx;          ///< index in fword[], case-folded bad word
    uchar_kt ts_fidxtry;       ///< ts_fidx at which bytes may be changed
    uchar_kt ts_twordlen;      ///< valid length of tword[]
    uchar_kt ts_prefixdepth;   ///< stack depth for end of prefix or
                             ///< PFD_PREFIXTREE or PFD_NOPREFIX
    uchar_kt ts_flags;         ///< TSF_ flags
    uchar_kt ts_tcharlen;      ///< number of bytes in tword character
    uchar_kt ts_tcharidx;      ///< current byte index in tword character
    uchar_kt ts_isdiff;        ///< DIFF_ values
    uchar_kt ts_fcharstart;    ///< index in fword where badword char started
    uchar_kt ts_prewordlen;    ///< length of word in "preword[]"
    uchar_kt ts_splitoff;      ///< index in "tword" after last split
    uchar_kt ts_splitfidx;     ///< "ts_fidx" at word split
    uchar_kt ts_complen;       ///< nr of compound words used
    uchar_kt ts_compsplit;     ///< index for "compflags" where word was spit
    uchar_kt ts_save_badflags; ///< su_badflags saved here
    uchar_kt ts_delidx;        ///< index in fword for char that was deleted,
                             ///< valid when "ts_flags" has TSF_DIDDEL
} trystate_T;

// Use our own character-case definitions, because the current locale may
// differ from what the .spl file uses.
// These must not be called with negative number!
#include <wchar.h>  // for towupper() and towlower()

/// Multi-byte implementation. For Unicode we can call utf_*(), but don't do
/// that for ASCII, because we don't want to use 'casemap' here. Otherwise use
/// the "w" library function for characters above 255.
#define SPELL_TOFOLD(c)         \
    (enc_utf8 && (c) >= 128     \
     ? utf_fold(c) : (c) < 256  \
     ? (int)spelltab.st_fold[c] : (int)towlower(c))

#define SPELL_TOUPPER(c)         \
    (enc_utf8 && (c) >= 128      \
     ? mb_toupper(c) : (c) < 256 \
     ? (int)spelltab.st_upper[c] : (int)towupper(c))

#define SPELL_ISUPPER(c)         \
    (enc_utf8 && (c) >= 128      \
     ? mb_isupper(c) : (c) < 256 \
     ? spelltab.st_isu[c] : iswupper(c))

/// First language that is loaded, start of
/// the linked list of loaded languages.
extern slang_T *first_lang;

/// file used for "zG" and "zW"
extern uchar_kt *int_wordlist;

extern spelltab_T spelltab;
extern int did_set_spelltab;
extern char *e_format;

#endif // NVIM_SPELL_DEFS_H
