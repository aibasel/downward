#include "bounds.h"

using namespace std;

namespace plugins {
Bounds::Bounds(const string &min, const string &max)
    : min(min), max(max) {
}

bool Bounds::has_bound() const {
    return !min.empty() || !max.empty();
}

Bounds Bounds::unlimited() {
    return Bounds("", "");
}

ostream &operator<<(ostream &out, const Bounds &bounds) {
    if (bounds.has_bound())
        out << "[" << bounds.min << ", " << bounds.max << "]";
    return out;
}
}
