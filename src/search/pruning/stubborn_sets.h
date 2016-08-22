#ifndef PRUNING_STUBBORN_SETS_H
#define PRUNING_STUBBORN_SETS_H

#include "../abstract_task.h"
#include "../pruning_method.h"

namespace stubborn_sets {
inline FactPair find_unsatisfied_condition(
    const std::vector<FactPair> &conditions, const State &state);

class StubbornSets : public PruningMethod {
    long num_unpruned_successors_generated;
    long num_pruned_successors_generated;

    /* stubborn[op_no] is true iff the operator with operator index
       op_no is contained in the stubborn set */
    std::vector<bool> stubborn;

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
    std::vector<std::vector<FactPair>> sorted_op_preconditions;
    std::vector<std::vector<FactPair>> sorted_op_effects;
    std::vector<FactPair> goals;

    /* achievers[var][value] contains all operator indices of
       operators that achieve the fact (var, value). */
    std::vector<std::vector<std::vector<int>>> achievers;

    bool can_disable(int op1_no, int op2_no);
    bool can_conflict(int op1_no, int op2_no);


    // Return the first unsatified goal pair, or (-1, -1) if there is none.
    FactPair find_unsatisfied_goal(const State &state) {
        return find_unsatisfied_condition(goals, state);
    }

    // Return the first unsatified precondition, or (-1, -1) if there is none.
    FactPair find_unsatisfied_precondition(int op_no, const State &state) {
        return find_unsatisfied_condition(sorted_op_preconditions[op_no], state);
    }

    // Returns true iff the operators was enqueued.
    // TODO: rename to enqueue_stubborn_operator?
    bool mark_as_stubborn(int op_no);
    virtual void initialize_stubborn_set(const State &state) = 0;
    virtual void handle_stubborn_operator(const State &state, int op_no) = 0;
public:
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;

    /* TODO: move prune_operators, and also the statistics, to the
       base class to have only one method virtual, and to make the
       interface more obvious */
    virtual void prune_operators(const State &state,
                                 std::vector<int> &op_ids) override;
    virtual void print_statistics() const override;
};

// Return the first unsatified condition, or (-1, -1) if there is none.
inline FactPair find_unsatisfied_condition(
    const std::vector<FactPair> &conditions, const State &state) {
    for (const FactPair &condition : conditions) {
        if (state[condition.var].get_value() != condition.value)
            return condition;
    }
    return FactPair(-1, -1);
}

}

#endif
