#ifndef ALGORITHMS_INT_HASH_SET_H
#define ALGORITHMS_INT_HASH_SET_H

#include "../utils/collections.h"
#include "../utils/language.h"
#include "../utils/logging.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

namespace int_hash_set {
/*
  Hash set for storing non-negative integer keys.

  Compared to unordered_set<int> in the standard library, this
  implementation is much more memory-efficient. It requires 8 bytes
  per bucket, so roughly 12-16 bytes per entry with typical load
  factors.

  Usage:

  IntHashSet<MyHasher, MyEqualityTester> s;
  pair<int, bool> result1 = s.insert(3);
  assert(result1 == make_pair(3, true));
  pair<int, bool> result2 = s.insert(3);
  assert(result2 == make_pair(3, false));

  Limitations:

  We use 32-bit (signed and unsigned) integers instead of larger data
  types for keys and hashes to save memory.

  Consequently, the range of valid keys is [0, 2^31 - 1]. This range
  could be extended to [0, 2^32 - 2] without using more memory by
  using unsigned integers for the keys and a different designated
  value for empty buckets (currently we use -1 for this).

  The maximum capacity (i.e., number of buckets) is 2^30 because we
  use a signed integer to store it, we grow the hash set by doubling
  its capacity, and the next larger power of 2 (2^31) is too big for
  an int. The maximum capacity could be increased by storing the
  number of buckets in a larger data type.

  Note on hash functions:

  Because the hash table uses open addressing, it is important to use
  hash functions that distribute the values roughly uniformly. Hash
  implementations intended for use with C++ standard containers often
  do not satisfy this requirement, for example hashing ints or
  pointers to themselves, which is not problematic for hash
  implementations based on chaining but can be catastrophic for this
  implementation.

  Implementation:

  All data and hashes are stored in a single vector, using open
  addressing. We use ideas from hopscotch hashing
  (https://en.wikipedia.org/wiki/Hopscotch_hashing) to ensure that
  each key is at most "max_distance" buckets away from its ideal
  bucket. This ensures constant lookup times since we always need to
  check at most "max_distance" buckets. Since all buckets we need to
  check for a given key are aligned in memory, the lookup has good
  cache locality.

*/

using KeyType = int;
using HashType = unsigned int;

static_assert(sizeof(KeyType) == 4, "KeyType does not use 4 bytes");
static_assert(sizeof(HashType) == 4, "HashType does not use 4 bytes");

template<typename Hasher, typename Equal>
class IntHashSet {
    // Max distance from the ideal bucket to the actual bucket for each key.
    static const int MAX_DISTANCE = 32;
    static const unsigned int MAX_BUCKETS = std::numeric_limits<unsigned int>::max();

    struct Bucket {
        KeyType key;
        HashType hash;

        static const KeyType empty_bucket_key = -1;

        Bucket()
            : key(empty_bucket_key),
              hash(0) {
        }

        Bucket(KeyType key, HashType hash)
            : key(key),
              hash(hash) {
        }

        bool full() const {
            return key != empty_bucket_key;
        }
    };

    Hasher hasher;
    Equal equal;
    std::vector<Bucket> buckets;
    int num_entries;
    int num_resizes;

    int capacity() const {
        return buckets.size();
    }

    void rehash(int new_capacity) {
        assert(new_capacity >= 1);
        int num_entries_before = num_entries;
        std::vector<Bucket> old_buckets = std::move(buckets);
        assert(buckets.empty());
        num_entries = 0;
        buckets.resize(new_capacity);
        for (const Bucket &bucket : old_buckets) {
            if (bucket.full()) {
                insert(bucket.key, bucket.hash);
            }
        }
        utils::unused_variable(num_entries_before);
        assert(num_entries == num_entries_before);
        ++num_resizes;
    }

    void enlarge() {
        unsigned int num_buckets = buckets.size();
        // Verify that the number of buckets is a power of 2.
        assert((num_buckets & (num_buckets - 1)) == 0);
        if (num_buckets > MAX_BUCKETS / 2) {
            std::cerr << "IntHashSet surpassed maximum capacity. This means"
                " you either use IntHashSet for high-memory"
                " applications for which it was not designed, or there"
                " is an unexpectedly high number of hash collisions"
                " that should be investigated. Aborting."
                      << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        rehash(num_buckets * 2);
    }

    int get_bucket(HashType hash) const {
        assert(!buckets.empty());
        unsigned int num_buckets = buckets.size();
        // Verify that the number of buckets is a power of 2.
        assert((num_buckets & (num_buckets - 1)) == 0);
        /* We want to return hash % num_buckets. The following line does this
           because we know that num_buckets is a power of 2. */
        return hash & (num_buckets - 1);
    }

    /*
      Return distance from index1 to index2, only moving right and wrapping
      from the last to the first bucket.
    */
    int get_distance(int index1, int index2) const {
        assert(utils::in_bounds(index1, buckets));
        assert(utils::in_bounds(index2, buckets));
        if (index2 >= index1) {
            return index2 - index1;
        } else {
            return capacity() + index2 - index1;
        }
    }

    int find_next_free_bucket_index(int index) const {
        assert(num_entries < capacity());
        assert(utils::in_bounds(index, buckets));
        while (buckets[index].full()) {
            index = get_bucket(index + 1);
        }
        return index;
    }

    KeyType find_equal_key(KeyType key, HashType hash) const {
        assert(hasher(key) == hash);
        int ideal_index = get_bucket(hash);
        for (int i = 0; i < MAX_DISTANCE; ++i) {
            int index = get_bucket(ideal_index + i);
            const Bucket &bucket = buckets[index];
            if (bucket.full() && bucket.hash == hash && equal(bucket.key, key)) {
                return bucket.key;
            }
        }
        return Bucket::empty_bucket_key;
    }

    /*
      Private method that inserts a key and its corresponding hash into the
      hash set.

      The method ensures that each key is at most "max_distance" buckets away
      from its ideal bucket by moving the closest free bucket towards the ideal
      bucket. If this can't be achieved, we resize the vector, reinsert the old
      keys and try inserting the new key again.

      For the return type, see the public insert() method.

      Note that the private insert() may call enlarge() and therefore rehash(),
      which itself calls the private insert() again.
    */
    std::pair<KeyType, bool> insert(KeyType key, HashType hash) {
        assert(hasher(key) == hash);

        /* If the hash set already contains the key, return the key and a
           Boolean indicating that no new key has been inserted. */
        KeyType equal_key = find_equal_key(key, hash);
        if (equal_key != Bucket::empty_bucket_key) {
            return std::make_pair(equal_key, false);
        }

        assert(num_entries <= capacity());
        if (num_entries == capacity()) {
            enlarge();
        }
        assert(num_entries < capacity());

        // Compute ideal bucket.
        int ideal_index = get_bucket(hash);

        // Find first free bucket left of the ideal bucket.
        int free_index = find_next_free_bucket_index(ideal_index);

        /*
          While the free bucket is too far from the ideal bucket, move the free
          bucket towards the ideal bucket by swapping a suitable third bucket
          with the free bucket. A full and an empty bucket can be swapped if
          the swap doesn't move the full bucket too far from its ideal
          position.
        */
        while (get_distance(ideal_index, free_index) >= MAX_DISTANCE) {
            bool swapped = false;
            int num_buckets = capacity();
            int max_offset = std::min(MAX_DISTANCE, num_buckets) - 1;
            for (int offset = max_offset; offset >= 1; --offset) {
                assert(offset < num_buckets);
                int candidate_index = free_index + num_buckets - offset;
                assert(candidate_index >= 0);
                candidate_index = get_bucket(candidate_index);
                HashType candidate_hash = buckets[candidate_index].hash;
                int candidate_ideal_index = get_bucket(candidate_hash);
                if (get_distance(candidate_ideal_index, free_index) < MAX_DISTANCE) {
                    // Candidate can be swapped.
                    std::swap(buckets[candidate_index], buckets[free_index]);
                    free_index = candidate_index;
                    swapped = true;
                    break;
                }
            }
            if (!swapped) {
                /* Free bucket could not be moved close enough to ideal bucket.
                   -> Enlarge and try inserting again. */
                enlarge();
                return insert(key, hash);
            }
        }
        assert(utils::in_bounds(free_index, buckets));
        assert(!buckets[free_index].full());
        buckets[free_index] = Bucket(key, hash);
        ++num_entries;
        return std::make_pair(key, true);
    }

public:
    IntHashSet(const Hasher &hasher, const Equal &equal)
        : hasher(hasher),
          equal(equal),
          buckets(1),
          num_entries(0),
          num_resizes(0) {
    }

    int size() const {
        return num_entries;
    }

    /*
      Insert a key into the hash set.

      Return a pair whose first item is the given key, or an equivalent key
      already contained in the hash set. The second item in the pair is a bool
      indicating whether a new key was inserted into the hash set.
    */
    std::pair<KeyType, bool> insert(KeyType key) {
        assert(key >= 0);
        return insert(key, hasher(key));
    }

    void dump() const {
        int num_buckets = capacity();
        utils::g_log << "[";
        for (int i = 0; i < num_buckets; ++i) {
            const Bucket &bucket = buckets[i];
            if (bucket.full()) {
                utils::g_log << bucket.key;
            } else {
                utils::g_log << "_";
            }
            if (i < num_buckets - 1) {
                utils::g_log << ", ";
            }
        }
        utils::g_log << "]" << std::endl;
    }

    void print_statistics() const {
        assert(!buckets.empty());
        int num_buckets = capacity();
        assert(num_buckets != 0);
        utils::g_log << "Int hash set load factor: " << num_entries << "/"
                     << num_buckets << " = "
                     << static_cast<double>(num_entries) / num_buckets
                     << std::endl;
        utils::g_log << "Int hash set resizes: " << num_resizes << std::endl;
    }
};

template<typename Hasher, typename Equal>
const int IntHashSet<Hasher, Equal>::MAX_DISTANCE;

template<typename Hasher, typename Equal>
const unsigned int IntHashSet<Hasher, Equal>::MAX_BUCKETS;
}

#endif
