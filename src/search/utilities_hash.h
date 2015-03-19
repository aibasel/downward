#ifndef HASH_UTILS
#define HASH_UTILS

#include <functional>
#include <utility>
#include <vector>

// combination of hash values is based on boost
//TODO: move helper functions to a namespace?
inline void hash_combine_impl(size_t &hash, size_t value) {
    hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
}

template<typename T>
inline void hash_combine(size_t &seed, const T &value) {
    const std::hash<T> hasher;
    return hash_combine_impl(seed, hasher(value));
}

template<typename Sequence>
size_t hash_sequence(const Sequence &data, size_t length) {
    size_t hash = 0;
    for (size_t i = 0; i < length; ++i) {
        hash_combine(hash, data[i]);
    }
    return hash;
}

template<typename Sequence>
inline size_t hash_sequence(const Sequence &data) {
    return ::hash_sequence(data, data.size());
}

namespace std {
template<typename T, typename Alloc>
struct hash<std::vector<T, Alloc> > {
    size_t operator()(const std::vector<T, Alloc> &vec) const {
        return ::hash_sequence(vec, vec.size());
    }
};

template<typename TA, typename TB>
struct  hash < std::pair < TA, TB > > {
    size_t operator()(const std::pair<TA, TB> &pair) const {
        size_t hash = 0;
        hash_combine(hash, pair.first);
        hash_combine(hash, pair.second);
        return hash;
    }
};

template<typename T, typename Alloc>
struct hash<pair<vector<T *, Alloc>, T *> > {
public:
    size_t operator()(const pair<vector<T *, Alloc>, T *> &pair) const {
        size_t hash = 0;
        hash_combine(hash, pair.second);
        hash_combine_impl(hash, hash_sequence(pair.first));
        return hash;
    }
};
}

#endif
