#include "cost_adapted_task.h"

#include "option_parser.h"
#include "plugin.h"
#include "utilities.h"

#include <iostream>
#include <memory>

using namespace std;


CostAdaptedTask::CostAdaptedTask(const Options &opts)
    : DelegatingTask(opts.get<shared_ptr<AbstractTask> >("transform")),
      cost_type(OperatorCost(opts.get<int>("cost_type"))),
      is_unit_cost(compute_is_unit_cost()) {
}

bool CostAdaptedTask::compute_is_unit_cost() const {
    int num_ops = parent->get_num_operators();
    for (int op_index = 0; op_index < num_ops; ++op_index) {
        if (parent->get_operator_cost(op_index, false) != 1)
            return false;
    }
    return true;
}

int CostAdaptedTask::get_operator_cost(int index, bool is_axiom) const {
    int original_cost = parent->get_operator_cost(index, is_axiom);

    // Don't change axiom costs. Usually they have cost 0, but we don't enforce this.
    if (is_axiom)
        return original_cost;

    switch (cost_type) {
    case NORMAL:
        return original_cost;
    case ONE:
        return 1;
    case PLUSONE:
        if (is_unit_cost)
            return 1;
        else
            return original_cost + 1;
    default:
        cerr << "Unknown cost type" << endl;
        exit_with(EXIT_CRITICAL_ERROR);
    }
}


static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    parser.add_option<shared_ptr<AbstractTask> >(
        "transform",
        "Parent task transformation",
        "no_transform");
    add_cost_type_option_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<CostAdaptedTask>(opts);
}

static PluginShared<AbstractTask> _plugin("adapt_costs", _parse);
