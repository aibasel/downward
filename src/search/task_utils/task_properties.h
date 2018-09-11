#ifndef TASK_UTILS_TASK_PROPERTIES_H
#define TASK_UTILS_TASK_PROPERTIES_H

#include "../global_state.h"
#include "../task_proxy.h"

#include "../algorithms/int_packer.h"

namespace task_properties {
inline bool is_applicable(OperatorProxy op, const State &state) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (state[precondition.get_variable()] != precondition)
            return false;
    }
    return true;
}

inline bool is_goal_state(TaskProxy task, const State &state) {
    for (FactProxy goal : task.get_goals()) {
        if (state[goal.get_variable()] != goal)
            return false;
    }
    return true;
}

/*
  TODO: get rid of this method and use the overload above instead.
  To make this efficient however, the search should work with unpacked States
  instead of packed GlobalStates internally.
*/
inline bool is_goal_state(const TaskProxy &task_proxy, const GlobalState &state) {
    for (FactProxy goal : task_proxy.get_goals()) {
        FactPair goal_fact = goal.get_pair();
        if (state[goal_fact.var] != goal_fact.value) {
            return false;
        }
    }
    return true;
}


/*
  Return true iff all operators have cost 1.

  Runtime: O(n), where n is the number of operators.
*/
extern bool is_unit_cost(TaskProxy task);

// Runtime: O(1)
extern bool has_axioms(TaskProxy task);

/*
  Report an error and exit with ExitCode::UNSUPPORTED if the task has axioms.
  Runtime: O(1)
*/
extern void verify_no_axioms(TaskProxy task);

// Runtime: O(n), where n is the number of operators.
extern bool has_conditional_effects(TaskProxy task);

/*
  Report an error and exit with ExitCode::UNSUPPORTED if the task has
  conditional effects.
  Runtime: O(n), where n is the number of operators.
*/
extern void verify_no_conditional_effects(TaskProxy task);

extern std::vector<int> get_operator_costs(const TaskProxy &task_proxy);
extern double get_average_operator_cost(TaskProxy task_proxy);
extern int get_min_operator_cost(TaskProxy task_proxy);

template<class FactProxyCollection>
std::vector<FactPair> get_fact_pairs(const FactProxyCollection &facts) {
    std::vector<FactPair> fact_pairs;
    fact_pairs.reserve(facts.size());
    for (FactProxy fact : facts) {
        fact_pairs.push_back(fact.get_pair());
    }
    return fact_pairs;
}

/* Create or retrieve a state packer from cache. If state packers are
   created with this function, we build at most one per AbstractTask. */
// TODO: remove duplication with causal graph and axiom evaluator.
const int_packer::IntPacker &get_state_packer(const AbstractTask *task);
}

#endif
