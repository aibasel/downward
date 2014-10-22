#ifndef UTILITIES_H
#define UTILITIES_H

#include <cstdlib>
#include <ostream>
#include <utility>
#include <vector>
#include <tr1/functional>

#define LINUX 0
#define OSX 1
#define CYGWIN 2
#define WINDOWS 3

#if defined(__CYGWIN32__)
#define OPERATING_SYSTEM CYGWIN
#elif defined(__WINNT__)
#define OPERATING_SYSTEM WINDOWS
#elif defined(__APPLE__)
#define OPERATING_SYSTEM OSX
#else
#define OPERATING_SYSTEM LINUX
#endif

#define ABORT(msg) \
    ( \
        (std::cerr << "Critical error in file " << __FILE__ \
                   << ", line " << __LINE__ << ": " << msg << std::endl), \
        (abort()), \
        (void)0 \
    )

enum ExitCode {
    EXIT_PLAN_FOUND = 0,
    EXIT_CRITICAL_ERROR = 1,
    EXIT_INPUT_ERROR = 2,
    EXIT_UNSUPPORTED = 3,
    // Task is provably unsolvable with current bound. Currently unused (see issue377).
    EXIT_UNSOLVABLE = 4,
    // Search ended without finding a solution.
    EXIT_UNSOLVED_INCOMPLETE = 5,
    EXIT_OUT_OF_MEMORY = 6,
    // Currently unused.
    EXIT_TIMEOUT = 7
};

extern void exit_with(ExitCode returncode) __attribute__((noreturn));

extern void register_event_handlers();

extern int get_peak_memory_in_kb(bool use_buffered_input = true);
extern void print_peak_memory(bool use_buffered_input = true);

/* Test if the product of two numbers is bounded by a third number.
   Safe against overflow. The caller must guarantee
   0 <= factor1, factor2 <= limit; failing this is an error. */
extern bool is_product_within_limit(int factor1, int factor2, int limit);

template<class T>
extern bool is_sorted_unique(const std::vector<T> &values) {
    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i - 1] >= values[i])
            return false;
    }
    return true;
}

namespace std {
template<class T>
ostream &operator<<(ostream &stream, const vector<T> &vec) {
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

template<class T>
bool in_bounds(int index, const T &container) {
    return index >= 0 && static_cast<size_t>(index) < container.size();
}

template<class T>
bool in_bounds(size_t index, const T &container) {
    return index < container.size();
}

template<typename T>
void unused_parameter(const T &) {
}

#endif
