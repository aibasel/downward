#ifndef CEGAR_DOMAINS_H
#define CEGAR_DOMAINS_H

#include "utils.h"

#include <boost/dynamic_bitset.hpp>

#include <string>
#include <vector>

class State;
class TaskProxy;

namespace cegar {
typedef boost::dynamic_bitset<> Bitset;

class Domains {
    // Possible values of each variable in this state.
    // Values are represented from right to left (least- to most-significant).
    // 11 10 001 -> var0 = {0}, var1 = {1}, var2 = {0,1}
    Bitset domains;

    static std::vector<int> original_domain_sizes;
    // Total number of facts and size of the values bitset.
    static int num_facts;
    // The first bit for var is at borders[var].
    static std::vector<int> borders;
    // masks[var][pos] == true iff pos belongs to var.
    static std::vector<Bitset> masks;
    static std::vector<Bitset> inverse_masks;
    // Temporary bitset for speeding up calculations.
    static Bitset temp_values;

    static void initialize_static_members(TaskProxy task_proxy);

    int pos(int var, int value) const {return borders[var] + value; }

public:
    explicit Domains(TaskProxy task_proxy);
    ~Domains() = default;

    void add(int var, int value);
    void set(int var, int value);
    void remove(int var, int value);
    void add_all(int var);
    void remove_all(int var);
    bool test(int var, int value) const;
    size_t count(int var) const;
    bool intersects(const Domains &other, int var) const;
    // Return true if all abstract domains are supersets of the
    // other's respective domains.
    bool abstracts(const Domains &other) const;
    void get_possible_flaws(const Domains &flaw, const State &conc_state, Flaws *flaws) const;

    std::string str() const;
};
}

#endif
