#ifndef CEGAR_VALUES_H
#define CEGAR_VALUES_H

#include "utils.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <vector>

namespace cegar_heuristic {

class Values {
private:
    // Forbid copy constructor and copy assignment operator.
    Values(const Values &);
    Values &operator=(const Values &);

    // Possible values of each variable in this state.
    // values[1] == {2} -> var1 is concretely set here.
    // values[1] == {2, 3} -> var1 has two possible values.
    std::vector<Domain> values;

public:
    void set_value(int var, int value, bool val = false);
    void set_var(int var);
    void reset_var(int var);
    bool test(int var, int value) const;
    int count(int var) const;
    void clear(int var);

    int get_var(int pos) const;
    bool intersects(int var, const Values &other) const;
};
}

#endif
