#include "cost_adapted_task.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../tasks/root_task.h"
#include "../utils/system.h"

#include <iostream>
#include <memory>

using namespace std;
using utils::ExitCode;

namespace tasks {
CostAdaptedTask::CostAdaptedTask(
    const shared_ptr<AbstractTask> &parent,
    OperatorCost cost_type)
    : DelegatingTask(parent),
      cost_type(cost_type),
      parent_is_unit_cost(compute_parent_is_unit_cost()) {
}

bool CostAdaptedTask::compute_parent_is_unit_cost() const {
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
        if (parent_is_unit_cost)
            return 1;
        else
            return original_cost + 1;
    default:
        cerr << "Unknown cost type" << endl;
        utils::exit_with(ExitCode::CRITICAL_ERROR);
    }
}


static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Cost-adapted task",
        "A cost-adapting transformation of the root task.");
    add_cost_type_option_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
        OperatorCost cost_type = OperatorCost(opts.get<int>("cost_type"));
        return make_shared<CostAdaptedTask>(g_root_task, cost_type);
    }
}

static PluginShared<AbstractTask> _plugin("adapt_costs", _parse);
}
