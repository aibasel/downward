#ifndef CEGAR_DOMAINS_H
#define CEGAR_DOMAINS_H

#include <boost/dynamic_bitset.hpp>

#include <vector>

namespace CEGAR {
using Bitset = boost::dynamic_bitset<>;

class Domains {
    /*
      We represent the possible values of all variables in a single
      bitset. If we assume that all of the task's facts are joined in a
      single list ordered by variable, this means that bits[i] is set
      iff the task's fact i is a possible fact in this Domains object.

      Example:
        Original domains: {0,1,2}, {0,1}, {0,1}
        Possible values:  {0},     {1},   {0,1}
        Bitset:           100      01     11    => 1000111

      TODO: We might want to think about a simpler representation.
      Almost all operations inspect the values of only a single
      variable, so it might even speed things up if we use, for
      example, a vector<Bitset> or a vector<vector<bool>>. This might
      also allow us to get rid of some of the static members.
    */
    Bitset bits;

    static std::vector<int> orig_domain_sizes;
    // Total number of facts and size of the bitset.
    static int num_facts;
    // The first bit for var is at borders[var].
    static std::vector<int> borders;
    // masks[var][pos] is set iff pos belongs to var.
    static std::vector<Bitset> masks;
    static std::vector<Bitset> inverse_masks;
    // Temporary bitset for speeding up calculations.
    static Bitset temp_bits;

    static void initialize_static_members(std::vector<int> &&domain_sizes);

    int pos(int var, int value) const {return borders[var] + value; }

public:
    explicit Domains(std::vector<int> &&domain_sizes);
    ~Domains() = default;

    void add(int var, int value);
    void set_single_value(int var, int value);
    void remove(int var, int value);
    void add_all(int var);
    void remove_all(int var);

    bool test(int var, int value) const {
        return bits.test(pos(var, value));
    }

    size_t count(int var) const;
    bool intersects(const Domains &other, int var) const;
    bool is_superset_of(const Domains &other) const;

    friend std::ostream &operator<<(std::ostream &os, const Domains &bits);
};

std::ostream &operator<<(std::ostream &os, const Domains &domains);
}

#endif
