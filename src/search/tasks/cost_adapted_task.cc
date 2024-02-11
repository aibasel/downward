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



using ConcreteProduct = CostAdaptedTask;
using AbstractProduct = AbstractTask;
using Concrete = TaskIndependentCostAdaptedTask;
// TODO issue559 use templates as 'get_task_specific' is EXACTLY the same for all TI_Components
shared_ptr<AbstractProduct> Concrete::get_task_specific(
    const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) const {
    shared_ptr<ConcreteProduct> task_specific_x;

    if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific " << get_product_name() << " '" << name << "'..." << endl;
        task_specific_x = dynamic_pointer_cast<ConcreteProduct>(
            component_map->at(static_cast<const TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific " << get_product_name() << " '" << name << "'..." << endl;
        task_specific_x = create_ts(task, component_map, depth);
        component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>
                                  (static_cast<const TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}

std::shared_ptr<ConcreteProduct> Concrete::create_ts(const shared_ptr <AbstractTask> &task,
                                                     [[maybe_unused]] unique_ptr <ComponentMap> &component_map,
                                                     [[maybe_unused]] int depth) const {
    return make_shared<CostAdaptedTask>(task, cost_type);
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
