#include "bounds.h"

#include <iostream>

using namespace std;

namespace options {
ostream &operator<<(ostream &out, const Bounds &bounds) {
    if (!bounds.min.empty() || !bounds.max.empty())
        out << "[" << bounds.min << ", " << bounds.max << "]";
    return out;
}
}
