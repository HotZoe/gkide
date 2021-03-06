/// @file nvim/hashtab.h

#ifndef NVIM_HASHTAB_H
#define NVIM_HASHTAB_H

#include <stddef.h>

#include "nvim/types.h"

/// Magic number used for hash item(hashitem_st)
/// @b hi_key, indicating a deleted item
///
/// Only the address is used.
extern char hash_removed;

/// Type for hash number (hash calculation result).
typedef size_t hash_kt;

/// The address of hash_removed is used as a magic number
/// for hi_key to indicate a removed item.
#define HI_KEY_REMOVED     ((uchar_kt *) &hash_removed)

#define HASHITEM_EMPTY(hi) ((hi)->hi_key == NULL \
                            || (hi)->hi_key == (uchar_kt *) &hash_removed)

/// A hastable item.
///
/// Each item has a NUL terminated string key.
/// A key can appear only once in the table.
///
/// A hash number is computed from the key for quick lookup.
/// When the hashes of two different keys point to the same
/// entry an algorithm is used to iterate over other entries
/// in the table until the right one is found. To make the
/// iteration work removed keys are different from entries
/// where a key was never present.
///
/// Note that this does not contain a pointer to the key and
/// another pointer to the value. Instead, it is assumed that
/// the key is contained within the value, so that you can get
/// a pointer to the value subtracting an offset from the pointer
/// to the key. This reduces the size of this item by 1/3.
typedef struct hashitem_s
{
    /// Cached hash number for hi_key.
    hash_kt hi_hash;

    /// Item key.
    ///
    /// Possible values mean the following:
    /// - NULL            : Item was never used.
    /// - HI_KEY_REMOVED  : Item was removed.
    /// - Any other value : Item is currently being used.
    uchar_kt *hi_key;
} hashitem_st;

/// Initial size for a hashtable.
///
/// Our items are relatively small and
/// growing is expensive, thus start with 16.
///
/// @note Must be a power of 2.
#define HT_INIT_SIZE    16

/// An array-based hashtable.
///
/// Keys are NUL terminated strings.
/// They cannot be repeated within a table.
/// Values are of any type.
///
/// The hashtable grows to accommodate more entries when needed.
typedef struct hashtable_s
{
    hash_kt ht_mask;        ///< mask used for hash value
                            ///< nr of items in array is: @b ht_mask + 1
    size_t ht_used;         ///< number of items used
    size_t ht_filled;       ///< number of items used or removed
    int ht_locked;          ///< counter for hash_lock()

    /// points to the array
    /// allocated when it's not @b ht_smallarray
    hashitem_st *ht_array;

    /// for initial array
    hashitem_st ht_smallarray[HT_INIT_SIZE];
} hashtable_st;

/// Iterate over a hashtab
///
/// @param[in] ht    Hashtab to iterate over.
/// @param     hi    Name of the variable with current hashtab entry.
/// @param     code  Cycle body.
#define HASHTAB_ITER(ht, hi, code)                                  \
    do                                                              \
    {                                                               \
        hashtable_st *const hi##ht_ = (ht);                         \
        size_t hi##todo_ = hi##ht_->ht_used;                        \
        for(hashitem_st *hi = hi##ht_->ht_array; hi##todo_; hi++)   \
        {                                                           \
            if(!HASHITEM_EMPTY(hi))                                 \
            {                                                       \
                {                                                   \
                    code                                            \
                }                                                   \
                hi##todo_--;                                        \
            }                                                       \
        }                                                           \
    } while(0)

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "hashtab.h.generated.h"
#endif

#endif // NVIM_HASHTAB_H
