#ifndef OPTIONS_BOUNDS_H
#define OPTIONS_BOUNDS_H

#include <iostream>
#include <string>

namespace options {
struct Bounds {
    std::string min;
    std::string max;

public:
    Bounds(const std::string &min, const std::string &max)
        : min(min), max(max) {
    }
    ~Bounds() = default;

    bool has_bound() const {
        return !min.empty() || !max.empty();
    }

    static Bounds unlimited() {
        return Bounds("", "");
    }

    friend std::ostream &operator<<(std::ostream &out, const Bounds &bounds);
};
}

#endif
