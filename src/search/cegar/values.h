#ifndef CEGAR_VALUES_H
#define CEGAR_VALUES_H

#include "utils.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <vector>

namespace cegar_heuristic {
class Values {
private:
    // Possible values of each variable in this state.
    // Values are represented from right to left (least- to most-significant).
    // 11 10 001 -> var0 = {0}, var1 = {1}, var2 = {0,1}
    Bitset values;

    // Total number of facts and size of the values bitset.
    static int facts;
    // The first bit for var is at borders[var].
    static vector<int> borders;
    // masks[var][pos] == true iff pos belongs to var.
    static vector<Bitset> masks;

    void initialize_static_members();
    int pos(int var, int value) const {return borders[var] + value; }
public:
    Values();
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
    string str() const;
    FRIEND_TEST(CegarTest, values);
};
}

#endif
