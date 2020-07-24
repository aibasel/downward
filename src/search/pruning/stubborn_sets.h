#ifndef PRUNING_STUBBORN_SETS_H
#define PRUNING_STUBBORN_SETS_H

#include "../abstract_task.h"
#include "../pruning_method.h"

#include "../utils/timer.h"

namespace options {
class OptionParser;
}

namespace stubborn_sets {
inline FactPair find_unsatisfied_condition(
    const std::vector<FactPair> &conditions, const State &state);

class StubbornSets : public PruningMethod {
    const double min_required_pruning_ratio;
    const int num_expansions_before_checking_pruning_ratio;
    int num_pruning_calls;
    bool is_pruning_disabled;

    utils::Timer timer;
    long num_unpruned_successors_generated;
    long num_pruned_successors_generated;

    /*
      stubborn_queue contains the operator indices of operators that
      have been marked as stubborn but have not yet been processed
      (i.e. more operators might need to be added to stubborn because
      of the operators in the queue).
    */
    std::vector<int> stubborn_queue;

    void compute_sorted_operators(const TaskProxy &task_proxy);
    void compute_achievers(const TaskProxy &task_proxy);

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

    bool can_disable(int op1_no, int op2_no) const;
    bool can_conflict(int op1_no, int op2_no) const;

    /*
      Return the first unsatified goal pair,
      or FactPair::no_fact if there is none.

      Note that we use a sorted list of goals here intentionally.
      (See comment on find_unsatisfied_precondition.)
    */
    FactPair find_unsatisfied_goal(const State &state) const {
        return find_unsatisfied_condition(sorted_goals, state);
    }

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

    // Return true iff the operator was enqueued.
    // TODO: rename to enqueue_stubborn_operator?
    bool mark_as_stubborn(int op_no);
    virtual void initialize_stubborn_set(const State &state) = 0;
    virtual void handle_stubborn_operator(const State &state, int op_no) = 0;
public:
    explicit StubbornSets(const options::Options &opts);

    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;

    /* TODO: move prune_operators, and also the statistics, to the
       base class to have only one method virtual, and to make the
       interface more obvious */
    virtual void prune_operators(const State &state,
                                 std::vector<OperatorID> &op_ids) override;
    virtual void print_statistics() const override;
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

void add_pruning_options(options::OptionParser &parser);
}

#endif
