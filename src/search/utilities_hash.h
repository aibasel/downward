#ifndef UTILITIES_HASH
#define UTILITIES_HASH

#include <functional>
#include <utility>
#include <vector>

// combination of hash values is based on Boost
//TODO: move helper functions to a namespace?
template<typename T>
inline void hash_combine(size_t &hash, const T &value) {
    const std::hash<T> hasher;
    hash ^= hasher(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
}

template<typename Sequence>
size_t hash_sequence(const Sequence &data, size_t length) {
    size_t hash = 0;
    for (size_t i = 0; i < length; ++i) {
        hash_combine(hash, data[i]);
    }
    return hash;
}

namespace std {
template<typename T>
struct hash<std::vector<T> > {
    size_t operator()(const std::vector<T> &vec) const {
        return ::hash_sequence(vec, vec.size());
    }
};

template<typename TA, typename TB>
struct  hash<std::pair<TA, TB> > {
    size_t operator()(const std::pair<TA, TB> &pair) const {
        size_t hash = 0;
        hash_combine(hash, pair.first);
        hash_combine(hash, pair.second);
        return hash;
    }
};

template<typename TA, typename TB>
struct hash<std::pair<vector<TA>, TB> > {
public:
    size_t operator()(const pair<vector<TA>, TB> &pair) const {
        size_t hash = 0;
        hash_combine(hash, pair.second);
        hash_combine(hash, pair.first);
        return hash;
    }
};
}

#endif
