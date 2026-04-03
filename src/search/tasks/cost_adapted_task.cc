#include "cost_adapted_task.h"

#include "../operator_cost.h"

#include "../task_utils/task_properties.h"

#include <memory>

using namespace std;
using utils::ExitCode;

namespace tasks {
CostAdaptedTask::CostAdaptedTask(
    const shared_ptr<AbstractTask> &parent, OperatorCost cost_type)
    : DelegatingTask(parent),
      cost_type(cost_type),
      parent_is_unit_cost(task_properties::is_unit_cost(TaskProxy(*parent))) {
}

int CostAdaptedTask::get_operator_cost(int index, bool is_axiom) const {
    OperatorProxy op(*parent, index, is_axiom);
    return get_adjusted_action_cost(op, cost_type, parent_is_unit_cost);
}
}
