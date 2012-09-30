#ifndef CEGAR_VALUES_H
#define CEGAR_VALUES_H

#include "utils.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <vector>

namespace cegar_heuristic {

class Values {
private:
    // Forbid copy constructor and copy assignment operator.
    //Values(const Values &);
    //Values &operator=(const Values &);

    // Possible values of each variable in this state.
    // values[1] == {2} -> var1 is concretely set here.
    // values[1] == {2, 3} -> var1 has two possible values.
    std::vector<Domain> values;

public:
    Values();
    void add(int var, int value);
    void set(int var, int value);
    void remove(int var, int value);
    void add_all(int var);
    void remove_all(int var);
    bool test(int var, int value) const;
    int count(int var) const;
    //int get_var(int pos) const;
    bool intersects(int var, const Values &other) const;
    string str() const;
    //Domain& operator[](int var) {return values[var]; }
    //const Domain& operator[](int var) const {return values[var]; }
    bool abstracts(const Values &other) const;
    void get_unmet_conditions(const Values &other, Conditions *conditions) const;
    void release_memory() {std::vector<Domain>().swap(values); }
};
}

#endif
