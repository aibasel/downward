#ifndef PDBS_PATTERN_DATABASE_FACTORY_H
#define PDBS_PATTERN_DATABASE_FACTORY_H

#include "types.h"

#include "../task_proxy.h"

#include <vector>

namespace pdbs {
class AbstractOperator;
class MatchTree;
class PerfectHashFunction;

class Projection {
    const TaskProxy &task_proxy;
    const Pattern &pattern;

    // Computed on demand
    mutable std::vector<int> variable_to_index;
    mutable std::vector<FactPair> abstract_goals;

    void compute_variable_to_index() const;
    void compute_abstract_goals() const;
public:
    Projection(const TaskProxy &task_proxy, const Pattern &pattern);

    const TaskProxy &get_task_proxy() const {
        return task_proxy;
    }

    const Pattern &get_pattern() const {
        return pattern;
    }

    int get_pattern_index(int var_id) const {
        if (variable_to_index.empty()) {
            compute_variable_to_index();
        }
        return variable_to_index[var_id];
    }

    const std::vector<FactPair> &get_abstract_goals() const {
        if (abstract_goals.empty()) {
            compute_abstract_goals();
        }
        return abstract_goals;
    }
};

extern PerfectHashFunction compute_hash_function(
    const TaskProxy &task_proxy, const Pattern &pattern);

extern MatchTree build_match_tree(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const std::vector<AbstractOperator> &abstract_operators);

/*
  For a given abstract state (given as index), the according values
  for each variable in the state are computed and compared with the
  given pairs of goal variables and values. Returns true iff the
  state is a goal state.
*/
extern bool is_goal_state(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    size_t state_index);

extern std::vector<int> compute_distances(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const std::vector<AbstractOperator> &regression_operators,
    const MatchTree &match_tree);

/*
    Computes all abstract operators, builds the match tree (successor
    generator) and then does a Dijkstra regression search to compute
    all final h-values (stored in distances). operator_costs can
    specify individual operator costs for each operator for action
    cost partitioning. If left empty, default operator costs are used.
*/
/*
  Important: It is assumed that the given pattern is sorted, contains no
  duplicates and is small enough so that the number of abstract states is
  below numeric_limits<int>::max().
  Optional parameters:
   dump:           If set to true, prints the construction time.
   operator_costs: Can specify individual operator costs for each
   operator. This is useful for action cost partitioning. If left
   empty, default operator costs are used.
*/
extern std::shared_ptr<PatternDatabase> generate_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    bool dump = false,
    const std::vector<int> &operator_costs = std::vector<int>());
}

#endif
