#ifndef OPTIONS_BOUNDS_H
#define OPTIONS_BOUNDS_H

#include <ostream>
#include <string>

namespace options {
struct Bounds {
    std::string min;
    std::string max;

public:
    Bounds(const std::string &min, const std::string &max);
    ~Bounds() = default;

    bool has_bound() const;
    static Bounds unlimited();

    friend std::ostream &operator<<(std::ostream &out, const Bounds &bounds);
};
}

#endif
