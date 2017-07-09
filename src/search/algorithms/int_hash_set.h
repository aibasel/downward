#ifndef ALGORITHMS_INT_HASH_SET_H
#define ALGORITHMS_INT_HASH_SET_H

#include "../utils/collections.h"
#include "../utils/language.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace int_hash_set {
/*
  Hash set using a single vector for storing non-negative integer keys.

  During key insertion we use ideas from hopscotch hashing to ensure
  that each key is at most "max_distance" buckets away from its ideal
  bucket. This ensures constant lookup times.
*/
template<typename Hasher, typename Equal>
class IntHashSet {
    // Allow using -1 for empty buckets.
    using KeyType = int;
    using HashType = unsigned int;

    // Max distance from the ideal bucket to the actual bucket for each key.
    static const int max_distance = 32;

    struct Bucket {
        static const KeyType empty_bucket_key = -1;
        KeyType key;
        HashType hash;

        Bucket()
            : key(empty_bucket_key),
              hash(0) {
        }

        Bucket(KeyType key, HashType hash)
            : key(key),
              hash(hash) {
        }

        bool empty() const {
            return key == empty_bucket_key;
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

    void enlarge() {
        reserve(get_capacity() * 2);
    }

    int wrap_unsigned(HashType hash) const {
        /* Note: If we restricted the number of buckets to powers of 2,
           we could use "i % 2^n = i & (2^n – 1)" to speed this up. */
        assert(!buckets.empty());
        return hash % buckets.size();
    }

    int wrap_signed(int n) const {
        assert(n >= 0);
        return wrap_unsigned(n);
    }

    /*
      Return distance from bucket 1 to bucket 2, only moving right and wrapping
      from the last to the first bucket.
    */
    int get_distance(int index1, int index2) const {
        assert(utils::in_bounds(index1, buckets));
        assert(utils::in_bounds(index2, buckets));
        if (index2 >= index1) {
            return index2 - index1;
        } else {
            return get_capacity() + index2 - index1;
        }
    }

    int find_next_free_bucket_index(int index) const {
        assert(num_entries < get_capacity());
        assert(utils::in_bounds(index, buckets));
        while (buckets[index].full()) {
            index = wrap_signed(index + 1);
        }
        return index;
    }

    KeyType find_equal_key(KeyType key, HashType hash) const {
        assert(hasher(key) == hash);
        int ideal_index = wrap_unsigned(hash);
        for (int i = 0; i < max_distance; ++i) {
            int index = wrap_signed(ideal_index + i);
            const Bucket &bucket = buckets[index];
            if (bucket.full() && bucket.hash == hash && equal(bucket.key, key)) {
                return bucket.key;
            }
        }
        return Bucket::empty_bucket_key;
    }

    /*
      Ensure that each key is at most "max_distance" buckets away from
      its ideal bucket by moving the closest free bucket towards the
      ideal bucket. If this can't be achieved, we resize the vector,
      reinsert the old keys and try inserting the new key again.

      For the return type, see the public insert() method.
    */
    std::pair<KeyType, bool> insert(KeyType key, HashType hash) {
        assert(hasher(key) == hash);

        KeyType equal_key = find_equal_key(key, hash);
        if (equal_key != Bucket::empty_bucket_key) {
            return {
                       equal_key, false
            };
        }

        assert(num_entries <= get_capacity());
        if (num_entries == get_capacity()) {
            enlarge();
        }
        assert(num_entries < get_capacity());

        int ideal_index = wrap_unsigned(hash);

        // Find next free bucket.
        int free_index = find_next_free_bucket_index(ideal_index);

        // Move the free bucket towards the ideal bucket.
        while (get_distance(ideal_index, free_index) >= max_distance) {
            bool swapped = false;
            int capacity = get_capacity();
            int max_offset = std::min(max_distance, capacity) - 1;
            for (int offset = max_offset; offset >= 1; --offset) {
                // Add capacity to ensure the index remains non-negative.
                assert(offset < capacity);
                int candidate_index = wrap_signed(free_index + capacity - offset);
                HashType candidate_hash = buckets[candidate_index].hash;
                int candidate_ideal_index = wrap_unsigned(candidate_hash);
                if (get_distance(candidate_ideal_index, free_index) < max_distance) {
                    // Candidate can be swapped.
                    std::swap(buckets[candidate_index], buckets[free_index]);
                    free_index = candidate_index;
                    swapped = true;
                    break;
                }
            }
            if (!swapped) {
                /* Free bucket could not be moved close enough.
                   -> Enlarge and try inserting again. */
                enlarge();
                return insert(key, hash);
            }
        }
        assert(utils::in_bounds(free_index, buckets));
        assert(buckets[free_index].empty());
        buckets[free_index] = Bucket(key, hash);
        ++num_entries;
        return {
                   key, true
        };
    }

public:
    IntHashSet(const Hasher &hasher, const Equal &equal)
        : hasher(hasher),
          equal(equal),
          buckets(1),
          num_entries(0),
          num_resizes(0) {
    }

    int get_num_entries() const {
        return num_entries;
    }

    int get_capacity() const {
        return buckets.size();
    }

    /*
      Return a pair whose first item is the given key, or an equivalent
      key already contained in the hash set. The second item in the
      pair is a bool indicating whether a new key was inserted into the
      hash set.
    */
    std::pair<KeyType, bool> insert(KeyType key) {
        return insert(key, hasher(key));
    }

    void reserve(int new_capacity) {
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

    void dump() const {
        int capacity = get_capacity();
        std::cout << "[";
        for (int i = 0; i < capacity; ++i) {
            const Bucket &bucket = buckets[i];
            if (bucket.full()) {
                std::cout << bucket.key;
            } else {
                std::cout << "_";
            }
            if (i < capacity - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }

    void print_statistics() const {
        assert(!buckets.empty());
        int capacity = get_capacity();
        assert(capacity != 0);
        std::cout << "Int hash set load factor: " << num_entries << "/"
                  << capacity << " = "
                  << static_cast<double>(num_entries) / capacity
                  << std::endl;
        std::cout << "Int hash set resizes: " << num_resizes << std::endl;
    }
};
}

#endif
