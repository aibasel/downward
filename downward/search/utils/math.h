#ifndef UTILS_MATH_H
#define UTILS_MATH_H

namespace utils {
/* Test if the product of two numbers is bounded by a third number.
   Safe against overflow. The caller must guarantee
   0 <= factor1, factor2 <= limit; failing this is an error. */
extern bool is_product_within_limit(int factor1, int factor2, int limit);

/* Test if the product of two numbers falls between the given inclusive lower
   and upper bounds. Safe against overflow. The caller must guarantee
   lower_limit < 0 and upper_limit >= 0; failing this is an error. */
extern bool is_product_within_limits(
    int factor1, int factor2, int lower_limit, int upper_limit);
}

#endif
