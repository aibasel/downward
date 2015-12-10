#ifndef UTILS_UTILITIES_H
#define UTILS_UTILITIES_H

#include "system.h"

#include <iostream>
#include <memory>
#include <utility>

#define ABORT(msg) \
    ( \
        (std::cerr << "Critical error in file " << __FILE__ \
                   << ", line " << __LINE__ << ": " << std::endl \
                   << (msg) << std::endl), \
        (abort()), \
        (void)0 \
    )


namespace Utils {
/* Test if the product of two numbers is bounded by a third number.
   Safe against overflow. The caller must guarantee
   0 <= factor1, factor2 <= limit; failing this is an error. */
extern bool is_product_within_limit(int factor1, int factor2, int limit);

/*
  make_unique_ptr is a poor man's version of make_unique. Once we
  require C++14, we should change all occurrences of make_unique_ptr
  to make_unique.
*/

template<typename T, typename ... Args>
std::unique_ptr<T> make_unique_ptr(Args && ... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args) ...));
}
}

#endif
