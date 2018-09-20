#ifndef FAST_HASH_H
#define FAST_HASH_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fast_hash {
static_assert(sizeof(unsigned int) == 4, "unsigned int has unexpected size");

/*
  Internal class storing the state of the hashing process. It should only be
  instantiated by functions in this file.
*/
class HashState {
    std::uint32_t hash;

public:
    HashState()
        : hash(0xdeadbeef) {
    }

    void feed(std::uint32_t value) {
        hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    std::uint32_t get_hash32() {
        return hash;
    }

    std::uint64_t get_hash64() {
        return (static_cast<std::uint64_t>(hash) << 32) | hash;
    }
};


/*
  These functions add a new object to an existing HashState object.

  To add hashing support for a user type X, provide an override
  for utils::feed(HashState &hash_state, const X &value).
*/
static_assert(
    sizeof(int) == sizeof(std::uint32_t),
    "int and uint32_t have different sizes");
inline void feed(HashState &hash_state, int value) {
    hash_state.feed(static_cast<std::uint32_t>(value));
}

static_assert(
    sizeof(unsigned int) == sizeof(std::uint32_t),
    "unsigned int and uint32_t have different sizes");
inline void feed(HashState &hash_state, unsigned int value) {
    hash_state.feed(static_cast<std::uint32_t>(value));
}

inline void feed(HashState &hash_state, std::uint64_t value) {
    hash_state.feed(static_cast<std::uint32_t>(value));
    value >>= 32;
    hash_state.feed(static_cast<std::uint32_t>(value));
}

template<typename T>
void feed(HashState &hash_state, const T *p) {
    // This is wasteful in 32-bit mode, but we plan to discontinue 32-bit compiles anyway.
    feed(hash_state, reinterpret_cast<std::uint64_t>(p));
}

template<typename T1, typename T2>
void feed(HashState &hash_state, const std::pair<T1, T2> &p) {
    feed(hash_state, p.first);
    feed(hash_state, p.second);
}

template<typename T>
void feed(HashState &hash_state, const std::vector<T> &vec) {
    /*
      Feed vector size to ensure that no two different vectors of the same type
      have the same code prefix.
    */
    feed(hash_state, vec.size());
    for (const T &item : vec) {
        feed(hash_state, item);
    }
}


/*
  Public hash functions.

  get_hash() is used internally by the HashMap and HashSet classes below. In
  more exotic use cases, such as implementing a custom hash table, you can also
  use `get_hash32()`, `get_hash64()` and `get_hash()` directly.
*/
template<typename T>
std::uint32_t get_hash32(const T &value) {
    HashState hash_state;
    feed(hash_state, value);
    return hash_state.get_hash32();
}

template<typename T>
std::uint64_t get_hash64(const T &value) {
    HashState hash_state;
    feed(hash_state, value);
    return hash_state.get_hash64();
}

template<typename T>
std::size_t get_hash(const T &value) {
    return static_cast<std::size_t>(get_hash64(value));
}


// This struct should only be used by HashMap and HashSet below.
template<typename T>
struct Hash {
    std::size_t operator()(const T &val) const {
        return get_hash(val);
    }
};

/*
  Aliases for hash sets and hash maps in user code. All user code should use
  utils::UnorderedSet and utils::UnorderedMap instead of std::unordered_set and
  std::unordered_map.

  To hash types that are not supported out of the box, implement utils::feed.
*/
template<typename T1, typename T2>
using HashMap = std::unordered_map<T1, T2, Hash<T1>>;

template<typename T>
using HashSet = std::unordered_set<T, Hash<T>>;


/* Transitional aliases and functions */
template<typename T1, typename T2>
using UnorderedMap = std::unordered_map<T1, T2>;

template<typename T>
using UnorderedSet = std::unordered_set<T>;
}

#endif
