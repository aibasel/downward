#include "modified_costs_task.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <memory>

using namespace std;


ModifiedCostsTask::ModifiedCostsTask(const Options &opts)
    : DelegatingTask(opts.get<shared_ptr<AbstractTask> >("transform")),
      operator_costs(opts.get<vector<int> >("operator_costs")) {
    assert(static_cast<int>(operator_costs.size()) == get_num_operators());
}

int ModifiedCostsTask::get_operator_cost(int index, bool is_axiom) const {
    // Don't change axiom costs. Usually they have cost 0, but we don't enforce this.
    if (is_axiom)
        return parent->get_operator_cost(index, is_axiom);
    return operator_costs[index];
}
