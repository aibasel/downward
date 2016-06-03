#ifndef PRUNING_STUBBORN_SETS_H
#define PRUNING_STUBBORN_SETS_H

#include "../abstract_task.h"
#include "../pruning_method.h"

namespace stubborn_sets {
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

    void compute_sorted_operators();
    void compute_achievers();

protected:
    std::vector<std::vector<Fact>> sorted_op_preconditions;
    std::vector<std::vector<Fact>> sorted_op_effects;

    /* achievers[var][value] contains all operator indices of
       operators that achieve the fact (var, value). */
    std::vector<std::vector<std::vector<int>>> achievers;

    bool can_disable(int op1_no, int op2_no);
    bool can_conflict(int op1_no, int op2_no);

    // Returns true iff the operators was enqueued.
    // TODO: rename to enqueue_stubborn_operator?
    bool mark_as_stubborn(int op_no);
    virtual void initialize_stubborn_set(const GlobalState &state) = 0;
    virtual void handle_stubborn_operator(const GlobalState &state, int op_no) = 0;
public:
    StubbornSets();
    virtual ~StubbornSets() = default;

    /* TODO: move prune_operators, and also the statistics, to the
       base class to have only one method virtual, and to make the
       interface more obvious */
    virtual void prune_operators(const GlobalState &state,
                                 std::vector<const GlobalOperator *> &ops) override;
    virtual void print_statistics() const override;
};
}

#endif
