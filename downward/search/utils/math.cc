#include "math.h"

#include <cassert>
#include <cstdlib>
#include <limits>

using namespace std;

namespace utils {
bool is_product_within_limit(int factor1, int factor2, int limit) {
    assert(factor1 >= 0);
    assert(factor2 >= 0);
    assert(limit >= 0);
    return factor2 == 0 || factor1 <= limit / factor2;
}

static bool is_product_within_limit_unsigned(
    unsigned int factor1, unsigned int factor2, unsigned int limit) {
    return factor2 == 0 || factor1 <= limit / factor2;
}

static unsigned int safe_abs(int x) {
    // Don't call abs() if the call would overflow.
    if (x == numeric_limits<int>::min()) {
        return static_cast<unsigned int>(-(x + 1)) + 1u;
    }
    return abs(x);
}

bool is_product_within_limits(
    int factor1, int factor2, int lower_limit, int upper_limit) {
    assert(lower_limit < 0);
    assert(upper_limit >= 0);

    if (factor1 >= 0 && factor2 >= 0) {
        return is_product_within_limit(factor1, factor2, upper_limit);
    } else if (factor1 < 0 && factor2 < 0) {
        return is_product_within_limit_unsigned(
            safe_abs(factor1), safe_abs(factor2), upper_limit);
    } else {
        return is_product_within_limit_unsigned(
            safe_abs(factor1), safe_abs(factor2), safe_abs(lower_limit));
    }
}
}
