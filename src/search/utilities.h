#ifndef UTILITIES_H
#define UTILITIES_H

#include <cassert>
#include <ostream>
#include <utility>
#include <vector>
#include <tr1/functional>

enum ExitCode {
    EXIT_PLAN_FOUND = 0,
    // Error in an underlying library.
    EXIT_CRITICAL_ERROR = 1,
    // Wrong commandline syntax or erroneous sas file.
    EXIT_INPUT_ERROR = 2,
    // Unsupported feature requested.
    EXIT_UNSUPPORTED = 3,
    // Task is provably unsolvable.
    EXIT_UNSOLVABLE = 4,
    // Search ended without finding a solution.
    EXIT_UNSOLVED_INCOMPLETE = 5,
    EXIT_OUT_OF_MEMORY = 6,
    // Currently unused.
    EXIT_TIMEOUT = 7
};

extern void exit_with(ExitCode returncode);

extern void register_event_handlers();

extern int get_peak_memory_in_kb();
extern void print_peak_memory();
extern void assert_sorted_unique(const std::vector<int> &values);

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
        return size_t(key.first * 1337 + key.second);
    }
};

struct hash_pointer_pair {
    size_t operator()(const std::pair<void *, void *> &key) const {
        return size_t(size_t(key.first) * 1337 + size_t(key.second));
    }
};

class hash_pointer {
public:
    size_t operator()(const void *p) const {
        //return size_t(reinterpret_cast<int>(p));
        std::tr1::hash<const void *> my_hash_class;
        return my_hash_class(p);
    }
};

#endif
