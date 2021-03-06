/// @file nvim/memfile_defs.h

#ifndef NVIM_MEMFILE_DEFS_H
#define NVIM_MEMFILE_DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "nvim/types.h"
#include "nvim/pos.h"

/// A block number.
///
/// Blocks numbered from 0 upwards have been assigned a place in the
/// actual file. The block number is equal to the page number in the
/// file. The blocks with negative numbers are currently in memory only.
typedef int64_t blknum_kt;

/// A hash item used for memory file.
///
/// Items' keys are block numbers.
/// Items in the same bucket are organized into a doubly-linked list.
///
/// Therefore, items can be arbitrary data structures beginning with
/// pointers for the list and and a block number key.
typedef struct mf_hashitem_s mf_hashitem_st;
struct mf_hashitem_s
{
    mf_hashitem_st *mhi_next;
    mf_hashitem_st *mhi_prev;
    blknum_kt mhi_key;
};

/// Initial size for a hashtable of memory file.
#define MHT_INIT_SIZE   64

/// A chained hashtable with block numbers as keys and arbitrary data
/// structures as items.
///
/// This is an intrusive data structure: we require that items begin with
/// mf_hashitem_st which contains the key and linked list pointers.
/// List of items in each bucket is doubly-linked.
typedef struct mf_hashtab_s
{
    /// mask used to mod hash value to array index
    /// (nr of items in array is 'mht_mask + 1')
    size_t mht_mask;

    /// number of items inserted
    size_t mht_count;

    /// points to the array of buckets (can be
    /// mht_small_buckets or a newly allocated array
    /// when mht_small_buckets becomes too small)
    mf_hashitem_st **mht_buckets;

    /// the initial buckets
    mf_hashitem_st *mht_small_buckets[MHT_INIT_SIZE];
} mf_hashtab_st;

/// Block header flags, used for blk_hdr_st::bh_flags
typedef enum blkhdr_flg_e
{
    kBlkHdrClean  = 0,
    kBlkHdrDirty  = 1,
    kBlkHdrLocked = 2,
} blkhdr_flg_et;

/// A block header.
///
/// There is a block header for each previously used block in the memfile.
///
/// The block may be linked in the used list OR in the free list.
/// The used blocks are also kept in hash lists.
///
/// The used list is a doubly linked list, most recently used block first.
/// The blocks in the used list have a block of memory allocated.
/// mf_used_count is the number of pages in the used list.
/// The hash lists are used to quickly find a block in the used list.
/// The free list is a single linked list, not sorted.
/// The blocks in the free list have no block of memory allocated and
/// the contents of the block in the file (if any) is irrelevant.
typedef struct blk_hdr_s blk_hdr_st;
struct blk_hdr_s
{
    /// block number, part of bh_hashitem
    #define bh_bnum  bh_hashitem.mhi_key
    /// header for hash table and key
    mf_hashitem_st   bh_hashitem;

    blk_hdr_st *bh_next;    ///< next block header in free or used list
    blk_hdr_st *bh_prev;    ///< previous block header in used list
    void *bh_data;          ///< pointer to memory (for used block)
    unsigned bh_page_count; ///< number of pages in this block
    blkhdr_flg_et bh_flags; ///< kBlkHdrClean, kBlkHdrDirty, kBlkHdrLocked
};

/// A block number translation list item.
///
/// When a block with a negative number is flushed to the file, it gets a
/// positive number. Because the reference to the block is still the negative
/// number, we remember the translation to the new positive number in the
/// double linked trans lists. The structure is the same as the hash lists.
typedef struct mf_blknum_trans_item_s
{
    /// old, negative, number
    #define nt_old_bnum nt_hashitem.mhi_key
    /// header for hash table and key
    mf_hashitem_st      nt_hashitem;

    /// new, positive, number
    blknum_kt nt_new_bnum;
} mf_blknum_trans_item_st;

/// A memory file.
typedef struct memfile_s
{
    uchar_kt *mf_fname;          ///< name of the file
    uchar_kt *mf_ffname;         ///< idem, full path
    int mf_fd;                   ///< file descriptor
    blk_hdr_st *mf_free_first;   ///< first block header in free list
    blk_hdr_st *mf_used_first;   ///< mru block header in used list
    blk_hdr_st *mf_used_last;    ///< lru block header in used list
    unsigned mf_used_count;      ///< number of pages in used list
    unsigned mf_used_count_max;  ///< maximum number of pages in memory
    mf_hashtab_st mf_hash;       ///< hash lists
    mf_hashtab_st mf_trans;      ///< trans lists
    blknum_kt mf_blocknr_max;    ///< highest positive block number + 1
    blknum_kt mf_blocknr_min;    ///< lowest negative block number - 1
    blknum_kt mf_neg_count;      ///< number of negative blocks numbers
    blknum_kt mf_infile_count;   ///< number of pages in the file
    unsigned mf_page_size;       ///< number of bytes in a page
    bool mf_dirty;               ///< TRUE if there are dirty blocks
} memfile_st;

#endif // NVIM_MEMFILE_DEFS_H
