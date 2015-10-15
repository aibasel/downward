#include "utilities.h"

#include "system.h"

#include <cassert>
#include <cstdlib>

using namespace std;

void exit_with(ExitCode exitcode) {
    report_exit_code_reentrant(exitcode);
    exit(exitcode);
}

bool is_product_within_limit(int factor1, int factor2, int limit) {
    assert(factor1 >= 0);
    assert(factor2 >= 0);
    assert(limit >= 0);
    return factor2 == 0 || factor1 <= limit / factor2;
}
