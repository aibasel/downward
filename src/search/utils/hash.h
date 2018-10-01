#ifndef UTILS_HASH_H
#define UTILS_HASH_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace utils {
/*
  We provide a family of hash functions that are supposedly higher
  quality than what is guaranteed by the standard library. Changing a
  single bit in the input should typically change around half of the
  bits in the final hash value. The hash functions we previously used
  turned out to cluster when we tried hash tables with open addressing
  for state registries.

  The low-level hash functions are based on lookup3.c by Bob Jenkins,
  May 2006, public domain. See http://www.burtleburtle.net/bob/c/lookup3.c.

  To hash an object x, it is represented as a sequence of 32-bit
  pieces (called the "code" for x, written code(x) in the following)
  that are "fed" to the main hashing function (implemented in class
  HashState) one by one. This allows a compositional approach to
  hashing. For example, the code for a pair p is the concatenation of
  code(x.first) and code(x.second).

  A simpler compositional approach to hashing would first hash the
  components of an object and then combine the hash values, and this
  is what a previous version of our code did. The approach with an
  explicit HashState object is stronger because the internal hash
  state is larger (96 bits) than the final hash value and hence pairs
  <x, y> and <x', y> where x and x' have the same hash value don't
  necessarily collide. Another advantage of our approach is that we
  can use the same overall hashing approach to generate hash values of
  different types (e.g. 32-bit vs. 64-bit unsigned integers).

  To extend the hashing mechanism to further classes, provide a
  template specialization for the "feed" function. This must satisfy
  the following requirements:

  A) If x and y are objects of the same type, they should have code(x)
     = code(y) iff x = y. That is, the code sequence should uniquely
     describe each logically distinct object.

     This requirement avoids unnecessary hash collisions. Of course,
     there will still be "necessary" hash collisions because different
     code sequences can collide in the low-level hash function.

  B) To play nicely with composition, we additionally require that feed
     implements a prefix code, i.e., for objects x != y of the same
     type, code(x) must not be a prefix of code(y).

     This requirement makes it much easier to define non-colliding
     code sequences for composite objects such as pairs via
     concatenation: if <a, b> != <a', b'>, then code(a) != code(a')
     and code(b) != code(b') is *not* sufficient for concat(code(a),
     code(b)) != concat(code(a'), code(b')). However, if we require a
     prefix code, it *is* sufficient and the resulting code will again
     be a prefix code.

  Note that objects "of the same type" is meant as "logical type"
  rather than C++ type.

  For example, for objects such as vectors where we expect
  different-length vectors to be combined in the same containers (=
  have the same logical type), we include the length of the vector as
  the first element in the code to ensure the prefix code property.

  In contrast, for integer arrays encoding states, we *do not* include
  the length as a prefix because states of different sizes are
  considered to be different logical types and should not be mixed in
  the same container, even though they are represented by the same C++
  type.
*/

/*
  Circular rotation (http://stackoverflow.com/a/31488147/224132).
*/
inline uint32_t rotate(uint32_t value, uint32_t offset) {
    return (value << offset) | (value >> (32 - offset));
}

/*
  Store the state of the hashing process.

  This class can either be used by specializing the template function
  utils::feed() (recommended, see below), or by working with it directly.
*/
class HashState {
    std::uint32_t a, b, c;
    int pending_values;

    /*
      Mix the three 32-bit values bijectively.

      Any information in (a, b, c) before mix() is still in (a, b, c) after
      mix().
    */
    void mix() {
        a -= c;
        a ^= rotate(c, 4);
        c += b;
        b -= a;
        b ^= rotate(a, 6);
        a += c;
        c -= b;
        c ^= rotate(b, 8);
        b += a;
        a -= c;
        a ^= rotate(c, 16);
        c += b;
        b -= a;
        b ^= rotate(a, 19);
        a += c;
        c -= b;
        c ^= rotate(b, 4);
        b += a;
    }

    /*
      Final mixing of the three 32-bit values (a, b, c) into c.

      Triples of (a, b, c) differing in only a few bits will usually produce
      values of c that look totally different.
    */
    void final_mix() {
        c ^= b;
        c -= rotate(b, 14);
        a ^= c;
        a -= rotate(c, 11);
        b ^= a;
        b -= rotate(a, 25);
        c ^= b;
        c -= rotate(b, 16);
        a ^= c;
        a -= rotate(c, 4);
        b ^= a;
        b -= rotate(a, 14);
        c ^= b;
        c -= rotate(b, 24);
    }

public:
    HashState()
        : a(0xdeadbeef),
          b(a),
          c(a),
          pending_values(0) {
    }

    void feed(std::uint32_t value) {
        assert(pending_values != -1);
        if (pending_values == 3) {
            mix();
            pending_values = 0;
        }
        if (pending_values == 0) {
            a += value;
            ++pending_values;
        } else if (pending_values == 1) {
            b += value;
            ++pending_values;
        } else if (pending_values == 2) {
            c += value;
            ++pending_values;
        }
    }

    /*
      After calling this method, it is illegal to use the HashState object
      further, i.e., make further calls to feed, get_hash32 or get_hash64. We
      set pending_values = -1 to catch such illegal usage in debug mode.
    */
    std::uint32_t get_hash32() {
        assert(pending_values != -1);
        if (pending_values) {
            /*
               pending_values == 0 can only hold if we never called
               feed(), i.e., if we are hashing an empty sequence.
               In this case we don't call final_mix for compatibility
               with the original hash function by Jenkins.
            */
            final_mix();
        }
        pending_values = -1;
        return c;
    }

    /*
      See comment for get_hash32.
    */
    std::uint64_t get_hash64() {
        assert(pending_values != -1);
        if (pending_values) {
            // See comment for get_hash32.
            final_mix();
        }
        pending_values = -1;
        return (static_cast<std::uint64_t>(b) << 32) | c;
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

      Using uint64_t is wasteful on 32-bit platforms but feeding a size_t breaks
      the build on MacOS (see msg7812).
    */
    feed(hash_state, static_cast<uint64_t>(vec.size()));
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
  Aliases for hash sets and hash maps in user code.

  Use these aliases for hashing types T that don't have a standard std::hash<T>
  specialization.

  To hash types that are not supported out of the box, implement utils::feed.
*/
template<typename T1, typename T2>
using HashMap = std::unordered_map<T1, T2, Hash<T1>>;

template<typename T>
using HashSet = std::unordered_set<T, Hash<T>>;
}

#endif
