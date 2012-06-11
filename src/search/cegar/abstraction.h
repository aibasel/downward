#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include <ext/slist>
#include <vector>
#include <deque>
#include <set>
#include <iostream>
#include <sstream>

#include "../heuristic.h"
#include "../operator.h"
#include "./abstract_state.h"

#include "gtest/gtest_prod.h"

namespace cegar_heuristic {

class Abstraction {
private:
    // abs_states[(1, -1, 0)] => Which abstract state has var1=1, var2=?, var3=0?
    //std::map<std::vector<int>, AbstractState> abs_states;
    std::vector<AbstractState*> abs_states;

    AbstractState init;
    deque<AbstractState*> solution_states;
    deque<Operator*> solution_ops;

    void extract_solution(AbstractState &goal);

    // Refinement hierarchy.
    AbstractState single;
    void collect_child_states(AbstractState *parent);

public:
    Abstraction();
    void collect_states();

    // Create a vector of values (state->vars) and set all values to -1 if
    // we haven't refined the variable yet.
    // Lookup this vector in abs_states and return it.
    AbstractState get_abstract_state(const State &state) const;

    FRIEND_TEST(CegarTest, find_solution);
    bool find_solution();
};

}

#endif
