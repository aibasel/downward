#ifndef CEGAR_DOMAINS_H
#define CEGAR_DOMAINS_H

#include "../algorithms/dynamic_bitset.h"

#include <ostream>
#include <vector>

namespace cegar {
using Bitset = dynamic_bitset::DynamicBitset<unsigned short>;

/*
  Represent a Cartesian set: for each variable store a subset of values.

  The underlying data structure is a vector of bitsets.
*/
class Domains {
    std::vector<Bitset> domain_subsets;

public:
    explicit Domains(const std::vector<int> &domain_sizes);

    void add(int var, int value);
    void set_single_value(int var, int value);
    void remove(int var, int value);
    void add_all(int var);
    void remove_all(int var);

    bool test(int var, int value) const {
        return domain_subsets[var][value];
    }

    int count(int var) const;
    bool intersects(const Domains &other, int var) const;
    bool is_superset_of(const Domains &other) const;

    friend std::ostream &operator<<(
        std::ostream &os, const Domains &domains);
};
}

#endif
