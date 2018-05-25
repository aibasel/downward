#include "modified_operator_costs_task.h"

#include <cassert>

using namespace std;


namespace extra_tasks {
ModifiedOperatorCostsTask::ModifiedOperatorCostsTask(
    const shared_ptr<AbstractTask> &parent,
    vector<int> &&costs)
    : DelegatingTask(parent),
      operator_costs(move(costs)) {
    assert(static_cast<int>(operator_costs.size()) == get_num_operators());
}

int ModifiedOperatorCostsTask::get_operator_cost(int index, bool is_axiom) const {
    // Don't change axiom costs. Usually they have cost 0, but we don't enforce this.
    if (is_axiom)
        return parent->get_operator_cost(index, is_axiom);
    return operator_costs[index];
}
}
