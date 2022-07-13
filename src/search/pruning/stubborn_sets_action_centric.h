#ifndef PRUNING_STUBBORN_SETS_ACTION_CENTRIC_H
#define PRUNING_STUBBORN_SETS_ACTION_CENTRIC_H

#include "stubborn_sets.h"

namespace stubborn_sets {
class StubbornSetsActionCentric : public stubborn_sets::StubbornSets {
    /*
      stubborn_queue contains the operator indices of operators that
      have been marked as stubborn but have not yet been processed
      (i.e. more operators might need to be added to stubborn because
      of the operators in the queue).
    */
    std::vector<int> stubborn_queue;
protected:
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

    // Return true iff the operator was enqueued.
    // TODO: rename to enqueue_stubborn_operator?
    bool mark_as_stubborn(int op_no);
    virtual void compute_stubborn_set(const State &state) override;
    virtual void initialize_stubborn_set(const State &state) = 0;
};
}

#endif
