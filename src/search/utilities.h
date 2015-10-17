#ifndef UTILITIES_H
#define UTILITIES_H

#include "system.h"

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#define ABORT(msg) \
    ( \
        (std::cerr << "Critical error in file " << __FILE__ \
                   << ", line " << __LINE__ << ": " << std::endl \
                   << (msg) << std::endl), \
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
    EXIT_OUT_OF_MEMORY = 6
};

NO_RETURN extern void exit_with(ExitCode returncode);

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

template<class T>
void release_vector_memory(std::vector<T> &vec) {
    std::vector<T>().swap(vec);
}

/*
  make_unique_ptr is a poor man's version of make_unique. Once we
  require C++14, we should change all occurrences of make_unique_ptr
  to make_unique.
*/

template<typename T, typename ... Args>
std::unique_ptr<T> make_unique_ptr(Args && ... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args) ...));
}

#endif
