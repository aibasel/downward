#include "cost_adapted_task.h"

#include "../operator_cost.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
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
      parent_is_unit_cost(task_properties::is_unit_cost(TaskProxy(*parent))) {
}

int CostAdaptedTask::get_operator_cost(int index, bool is_axiom) const {
    OperatorProxy op(*parent, index, is_axiom);
    return get_adjusted_action_cost(op, cost_type, parent_is_unit_cost);
}


TaskIndependentCostAdaptedTask::TaskIndependentCostAdaptedTask(OperatorCost cost_type)
    : cost_type(cost_type) {
}

shared_ptr<CostAdaptedTask> TaskIndependentCostAdaptedTask::create_task_specific_CostAdaptedTask(const shared_ptr<AbstractTask> &task, int depth) {
    utils::g_log << "Creating CostAdaptedTask as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_CostAdaptedTask(task, component_map, depth);
}

shared_ptr<CostAdaptedTask> TaskIndependentCostAdaptedTask::create_task_specific_CostAdaptedTask([[maybe_unused]] const shared_ptr<AbstractTask> &task, [[maybe_unused]] shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<CostAdaptedTask> task_specific_x;
    if (component_map->contains_key(make_pair(task, static_cast<void *>(this)))) {
        utils::g_log << std::string(depth, ' ') << "Reusing task CostAdaptedTask..." << endl;
        task_specific_x = plugins::any_cast<shared_ptr<CostAdaptedTask>>(
            component_map->get_dual_key_value(task, this));
    } else {
        utils::g_log << std::string(depth, ' ') << "Creating task specific CostAdaptedTask..." << endl;
        task_specific_x = make_shared<CostAdaptedTask>(task, cost_type);
        component_map->add_dual_key_entry(task, this, plugins::Any(task_specific_x));
    }
    return task_specific_x;
}


shared_ptr<DelegatingTask> TaskIndependentCostAdaptedTask::create_task_specific_DelegatingTask(const shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<CostAdaptedTask> x = create_task_specific_CostAdaptedTask(task, component_map, depth);
    return static_pointer_cast<DelegatingTask>(x);
}


class CostAdaptedTaskFeature : public plugins::TypedFeature<TaskIndependentAbstractTask, TaskIndependentCostAdaptedTask> {
public:
    CostAdaptedTaskFeature() : TypedFeature("adapt_costs") {
        document_title("Cost-adapted task");
        document_synopsis(
            "A cost-adapting transformation of the root task.");

        add_cost_type_option_to_feature(*this);
    }

    virtual shared_ptr<TaskIndependentCostAdaptedTask> create_component(const plugins::Options &options, const utils::Context &) const override {
        OperatorCost cost_type = options.get<OperatorCost>("cost_type");
        return make_shared<TaskIndependentCostAdaptedTask>(cost_type);
    }
};

static plugins::FeaturePlugin<CostAdaptedTaskFeature> _plugin;
}
