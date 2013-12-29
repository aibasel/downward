#ifndef CEGAR_VALUES_H
#define CEGAR_VALUES_H

#include "utils.h"

#include <string>
#include <vector>

namespace cegar_heuristic {
class AbstractState;

class Values {
friend class AbstractState;

private:
    // Possible values of each variable in this state.
    // Values are represented from right to left (least- to most-significant).
    // 11 10 001 -> var0 = {0}, var1 = {1}, var2 = {0,1}
    Bitset values;

    // Total number of facts and size of the values bitset.
    static int facts;
    // The first bit for var is at borders[var].
    static std::vector<int> borders;
    // masks[var][pos] == true iff pos belongs to var.
    static std::vector<Bitset> masks;

    Values();

    int pos(int var, int value) const {return borders[var] + value; }

    void add(int var, int value);
    void set(int var, int value);
    void remove(int var, int value);
    void add_all(int var);
    void remove_all(int var);
    bool test(int var, int value) const;
    int count(int var) const;
    bool domains_intersect(const Values &other, int var);
    bool abstracts(const Values &other) const;
    void get_possible_splits(const Values &flaw, const State conc_state, Splits *splits) const;

    std::string str() const;

public:
    static void initialize_static_members();
};
}

#endif
