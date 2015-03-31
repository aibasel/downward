#ifndef UTILITIES_HASH_H
#define UTILITIES_HASH_H

#include <functional>
#include <utility>
#include <vector>

/*
  Hash a new value and combine it with an existing hash.

  This function should only be called from within this module.
*/
template<typename T>
inline void hash_combine(size_t &hash, const T &value) {
    std::hash<T> hasher;
    /*
      The combination of hash values is based on issue 6.18 in
      http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1756.pdf.
      Boost combines hash values in the same way.
    */
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
struct hash<std::pair<TA, TB> > {
    size_t operator()(const std::pair<TA, TB> &pair) const {
        size_t hash = 0;
        hash_combine(hash, pair.first);
        hash_combine(hash, pair.second);
        return hash;
    }
};
}

#endif
