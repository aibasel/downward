#include "stubborn_sets_action_centric.h"

using namespace std;

namespace stubborn_sets {
// Relies on both fact sets being sorted by variable.
static bool contain_conflicting_fact(const vector<FactPair> &facts1,
                                     const vector<FactPair> &facts2) {
    auto facts1_it = facts1.begin();
    auto facts2_it = facts2.begin();
    while (facts1_it != facts1.end() && facts2_it != facts2.end()) {
        if (facts1_it->var < facts2_it->var) {
            ++facts1_it;
        } else if (facts1_it->var > facts2_it->var) {
            ++facts2_it;
        } else {
            if (facts1_it->value != facts2_it->value)
                return true;
            ++facts1_it;
            ++facts2_it;
        }
    }
    return false;
}

StubbornSetsActionCentric::StubbornSetsActionCentric(
    utils::Verbosity verbosity)
    : StubbornSets(verbosity) {
}

void StubbornSetsActionCentric::compute_stubborn_set(const State &state) {
    assert(stubborn_queue.empty());

    initialize_stubborn_set(state);
    /* Iteratively insert operators to stubborn according to the
       definition of strong stubborn sets until a fixpoint is reached. */
    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();
        stubborn_queue.pop_back();
        handle_stubborn_operator(state, op_no);
    }
}

// Relies on op_preconds and op_effects being sorted by variable.
bool StubbornSetsActionCentric::can_disable(int op1_no, int op2_no) const {
    return contain_conflicting_fact(sorted_op_effects[op1_no],
                                    sorted_op_preconditions[op2_no]);
}

// Relies on op_effect being sorted by variable.
bool StubbornSetsActionCentric::can_conflict(int op1_no, int op2_no) const {
    return contain_conflicting_fact(sorted_op_effects[op1_no],
                                    sorted_op_effects[op2_no]);
}

bool StubbornSetsActionCentric::enqueue_stubborn_operator(int op_no) {
    if (!stubborn[op_no]) {
        stubborn[op_no] = true;
        stubborn_queue.push_back(op_no);
        return true;
    }
    return false;
}
}
