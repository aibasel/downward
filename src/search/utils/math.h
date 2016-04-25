#ifndef UTILS_MATH_H
#define UTILS_MATH_H

#include <cstddef> // for size_t

namespace utils {
/* Test if the product of two numbers is bounded by a third number.
   Safe against overflow. The caller must guarantee
   0 <= factor1, factor2 <= limit; failing this is an error. */
extern bool is_product_within_limit_int(int factor1, int factor2, int limit);
extern bool is_product_within_limit_size_t(std::size_t factor1, std::size_t factor2, std::size_t limit);
}

#endif
