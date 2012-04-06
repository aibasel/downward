#ifndef UTILITIES_H
#define UTILITIES_H

#include <ostream>
#include <utility>
#include <vector>

extern void register_event_handlers();

extern int get_peak_memory_in_kb();
extern void print_peak_memory();

namespace std {
template<class T>
ostream & operator<<(ostream &stream, const vector<T> &vec) {
    stream << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0)
            stream << ", ";
        stream << vec[i];
    }
    stream << "]";
    return stream;
}
}

template<class Sequence>
size_t hash_number_sequence(Sequence data, size_t length) {
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
        return size_t(1337 * key.first + key.second);
    }
};

#endif
