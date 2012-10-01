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

    static int facts;
    static vector<int> borders;
    static vector<Bitset> masks;

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
    bool all_vars_intersect(const Values &other, const std::vector<bool> &checked) const;
    string str() const;
    //Domain& operator[](int var) {return values[var]; }
    //const Domain& operator[](int var) const {return values[var]; }
    bool abstracts(const Values &other) const;
    void get_unmet_conditions(const Values &other, Conditions *conditions) const;
    // TODO: Release memory.
    void release_memory() { }
};
}

#endif
