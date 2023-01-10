#ifndef PRUNING_STUBBORN_SETS_H
#define PRUNING_STUBBORN_SETS_H

#include "../pruning_method.h"
#include "../task_proxy.h"

namespace stubborn_sets {
inline FactPair find_unsatisfied_condition(
    const std::vector<FactPair> &conditions, const State &state);

class StubbornSets : public PruningMethod {
    void compute_sorted_operators(const TaskProxy &task_proxy);
    void compute_achievers(const TaskProxy &task_proxy);
    virtual void prune(const State &state,
                       std::vector<OperatorID> &op_ids) override;
protected:
    /*
      We copy some parts of the task here, so we can avoid the more expensive
      access through the task interface during the search.
    */
    int num_operators;
    std::vector<std::vector<FactPair>> sorted_op_preconditions;
    std::vector<std::vector<FactPair>> sorted_op_effects;
    std::vector<FactPair> sorted_goals;

    /* achievers[var][value] contains all operator indices of
       operators that achieve the fact (var, value). */
    std::vector<std::vector<std::vector<int>>> achievers;

    /* stubborn[op_no] is true iff the operator with operator index
       op_no is contained in the stubborn set */
    std::vector<bool> stubborn;

    /*
      Return the first unsatified precondition,
      or FactPair::no_fact if there is none.

      Note that we use a sorted list of preconditions here intentionally.
      The ICAPS paper "Efficient Stubborn Sets: Generalized Algorithms and
      Selection Strategies" (Wehrle and Helmert, 2014) contains a limited study
      of this (see section "Strategies for Choosing Unsatisfied Conditions" and
      especially subsection "Static Variable Orderings"). One of the outcomes
      was the sorted version ("static orders/FD" in Table 1 of the paper)
      is dramatically better than choosing preconditions and goals randomly
      every time ("dynamic orders/random" in Table 1).

      The code also intentionally uses the "causal graph order" of variables
      rather than an arbitrary variable order. (However, so far, there is no
      experimental evidence that this is a particularly good order.)
    */
    FactPair find_unsatisfied_precondition(int op_no, const State &state) const {
        return find_unsatisfied_condition(sorted_op_preconditions[op_no], state);
    }

    virtual void compute_stubborn_set(const State &state) = 0;
public:
    explicit StubbornSets(const plugins::Options &opts);
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
};

// Return the first unsatified condition, or FactPair::no_fact if there is none.
inline FactPair find_unsatisfied_condition(
    const std::vector<FactPair> &conditions, const State &state) {
    for (const FactPair &condition : conditions) {
        if (state[condition.var].get_value() != condition.value)
            return condition;
    }
    return FactPair::no_fact;
}
}

#endif
