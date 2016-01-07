#ifndef CEGAR_DOMAINS_H
#define CEGAR_DOMAINS_H

#include <boost/dynamic_bitset.hpp>

#include <vector>

namespace CEGAR {
using Bitset = boost::dynamic_bitset<unsigned long>;

/*
  For each variable store a subset of values.

  The underlying data structure is a vector of bitsets.
*/
class Domains {
    std::vector<Bitset> bits;

public:
    explicit Domains(const std::vector<int> &domain_sizes);
    ~Domains() = default;

    void add(int var, int value);
    void set_single_value(int var, int value);
    void remove(int var, int value);
    void add_all(int var);
    void remove_all(int var);

    bool test(int var, int value) const {
        return bits[var][value];
    }

    int count(int var) const;
    bool intersects(const Domains &other, int var) const;
    bool is_superset_of(const Domains &other) const;

    friend std::ostream &operator<<(std::ostream &os, const Domains &bits);
};

std::ostream &operator<<(std::ostream &os, const Domains &domains);
}

#endif
