#ifndef HASH_UTILS
#define HASH_UTILS

#include <functional>
#include <utility>
#include <vector>


template<class Sequence>
size_t hash_number_sequence(const Sequence &data, size_t length) {
    // hash function adapted from Python's hash function for tuples.
    size_t hash_value = 0x345678;
    size_t mult = 1000003;
    for (int i = length - 1; i >= 0; --i) {
        hash_value = (hash_value ^ data[i]) * mult;
        mult += 82520 + i + i;
    }
    hash_value += 97531;
    return hash_value;
}

namespace std {
template<typename T, typename A>
struct hash<const std::vector<T, A> > {
    size_t operator()(const std::vector<T, A> &vec) const {
        return ::hash_number_sequence(vec, vec.size());
    }
};

// based on boost
template<typename TA, typename TB>
struct  hash < std::pair < TA, TB >> {
    inline void hash_combine_impl(size_t &seed, size_t value) const {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    size_t operator()(const std::pair<TA, TB> &key) const {
        size_t seed = 0;
        const std::hash<TA> hash_a;
        const std::hash<TB> hash_b;
        hash_combine_impl(seed, hash_a(key.first));
        hash_combine_impl(seed, hash_b(key.second));
        return seed;
    }
};
}

#endif // HASH_UTILS
