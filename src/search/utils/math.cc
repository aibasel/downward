#include "math.h"

#include <cassert>

namespace utils {
bool is_product_within_limit_int(int factor1, int factor2, int limit) {
    assert(factor1 >= 0);
    assert(factor2 >= 0);
    assert(limit >= 0);
    return factor2 == 0 || factor1 <= limit / factor2;
}

bool is_product_within_limit_size_t(size_t factor1, size_t factor2, size_t limit) {
    return factor2 == 0 || factor1 <= limit / factor2;
}
}
