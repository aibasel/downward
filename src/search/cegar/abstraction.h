#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include <ext/slist>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>

#include "../heuristic.h"
#include "../operator.h"
#include "./abstract_state.h"

#include "gtest/gtest_prod.h"

namespace cegar_heuristic {

class Abstraction {
    // abs_states[(1, -1, 0)] => Which abstract state has var1=1, var2=?, var3=0?
    //std::map<std::vector<int>, AbstractState> abs_states;

    // Create a vector of values (state->vars) and set all values to -1 if
    // we haven't refined the variable yet.
    // Lookup this vector in abs_states and return it.
    AbstractState get_abstract_state(const State &state) const;
};

}

#endif
