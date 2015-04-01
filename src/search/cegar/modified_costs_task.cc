#include "modified_costs_task.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <iostream>
#include <memory>

using namespace std;


ModifiedCostsTask::ModifiedCostsTask(const Options &opts)
    : DelegatingTask(shared_ptr<AbstractTask>(opts.get<AbstractTask *>("transform"))),
      operator_costs(opts.get<vector<int> >("operator_costs")) {
    assert(static_cast<int>(operator_costs.size()) == get_num_operators());
}

int ModifiedCostsTask::get_operator_cost(int index, bool is_axiom) const {
    // Don't change axiom costs. Usually they have cost 0, but we don't enforce this.
    if (is_axiom)
        return parent->get_operator_cost(index, is_axiom);
    return operator_costs[index];
}


static AbstractTask *_parse(OptionParser &parser) {
    parser.add_option<AbstractTask *>(
        "transform",
        "Parent task transformation",
        "no_transform");
    parser.add_list_option<int>(
        "operator_costs",
        "List of operator costs");
    add_cost_type_option_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new ModifiedCostsTask(opts);
}

static Plugin<AbstractTask> _plugin("set_costs", _parse);
