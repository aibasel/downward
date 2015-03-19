#ifndef HASH_UTILS
#define HASH_UTILS

#include <functional>
#include <utility>
#include <vector>

// combination of hash values is based on boost
//TODO: move helper functions to a namespace?
inline void hash_combine_impl(size_t &seed, size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T>
inline void hash_combine(size_t &seed, const T &value) {
    const std::hash<T> hasher;
    return hash_combine_impl(seed, hasher(value));
}

template<typename Sequence>
size_t hash_number_sequence(const Sequence &data, size_t length) {
    size_t seed = 0;
    for (size_t i = 0; i < length; ++i) {
        hash_combine(seed, data[i]);
    }
    return seed;
}

template<typename Sequence>
inline size_t hash_number_sequence(const Sequence &data) {
    return hash_number_sequence(data, data.size());
}

namespace std {
template<typename T, typename A>
struct hash<std::vector<T, A> > {
    size_t operator()(const std::vector<T, A> &vec) const {
        return ::hash_number_sequence(vec, vec.size());
    }
};

template<typename TA, typename TB>
struct  hash < std::pair < TA, TB > > {
    size_t operator()(const std::pair<TA, TB> &key) const {
        size_t seed = 0;
        const std::hash<TA> hash_a;
        const std::hash<TB> hash_b;
        hash_combine_impl(seed, hash_a(key.first));
        hash_combine_impl(seed, hash_b(key.second));
        return seed;
    }
};

template<typename T, typename A>
struct hash<pair<vector<T *, A>, T *> > {
public:
    size_t operator()(const pair<vector<T *, A>, T *> &key) const {
        size_t seed = 0;
        hash_combine(seed, key.second);
        hash_combine_impl(seed, hash_number_sequence(key.first));
        return seed;
    }
};
}

#endif // HASH_UTILS
