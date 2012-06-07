#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include <ext/slist>
#include <vector>
#include <set>

#include "../heuristic.h"

using namespace std;
using namespace __gnu_cxx;

namespace cegar_heuristic {

// TODO: Use 32-bit masks for variables. This means we can not handle tasks
// with domain sizes > 32.

/*
class AbstractTransitionSystem {
    // abs_states[(1, -1, 0)] => Which abstract state has var1=1, var2=?, var3=0?
    hash_map<vector<int>, AbstractState> abs_states;

    AbstractState get_abstract_state(const State &state) {
        // Create a vector of values (state->vars) and set all values to -1 if
        // we haven't refined the variable yet.
        // Lookup this vector in abs_states and return it.
    }
};

*/

int get_eff(Operator op, int var);
int get_pre(Operator op, int var);

class AbstractState {
    // Possible values of each variable in this state.
    // values[1] == {2} -> var1 is concretely set here.
    // values[1] == {2, 3} -> var1 has two possible values.
    vector<set<int> > values;

public:
    AbstractState regress(Operator op);
};


class CegarHeuristic : public Heuristic {
    int min_operator_cost;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    CegarHeuristic(const Options &options);
    ~CegarHeuristic();
};

}

#endif
