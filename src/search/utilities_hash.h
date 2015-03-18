#ifndef HASH_UTILS
#define HASH_UTILS

#include <functional>
#include <utility>


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

struct hash_int_pair {
    size_t operator()(const std::pair<int, int> &key) const {
        return size_t(key.first * 1337 + key.second);
    }
};

namespace std{

template<typename T>
struct hash<std::pair<T, T>>{
    size_t operator()(const std::pair<T, T> &key) const {
        return std::hash<T>(key.first) * 1337 + std::hash<T>(key.second);
    }
};

//TODO: this is basically a copy of the std::hash<T*> just for pairs so we should find a possbility to reuse it.
template<typename T>
struct hash<std::pair<T*, T*>>{
    size_t operator()(const std::pair<T*, T*> &key) const {
        return  reinterpret_cast<size_t>(key.first) * 1337 + reinterpret_cast<size_t>(key.second);
    }
};


}

#endif // HASH_UTILS

