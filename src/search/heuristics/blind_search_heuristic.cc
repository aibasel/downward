#include "blind_search_heuristic.h"

#include "../plugins/plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

using namespace std;

namespace blind_search_heuristic {
BlindSearchHeuristic::BlindSearchHeuristic(
    const shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(transform, cache_estimates, description, verbosity),
      min_operator_cost(
          task_properties::get_min_operator_cost(task_proxy)) {
    if (log.is_at_least_normal()) {
        log << "Initializing blind search heuristic '" << description << "'..." << endl;
    }
}

int BlindSearchHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    if (task_properties::is_goal_state(task_proxy, state))
        return 0;
    else
        return min_operator_cost;
}


TaskIndependentBlindSearchHeuristic::TaskIndependentBlindSearchHeuristic(
    const shared_ptr<TaskIndependentAbstractTask> transform,
    bool cache_estimates,
    const string &description,
    utils::Verbosity verbosity)
    : TaskIndependentHeuristic(transform, cache_estimates, description, verbosity) {
}

TaskIndependentBlindSearchHeuristic::~TaskIndependentBlindSearchHeuristic() {
}


using ConcreteProduct = BlindSearchHeuristic;
using AbstractProduct = Evaluator;
using Concrete = TaskIndependentBlindSearchHeuristic;
// TODO issue559 use templates as 'get_task_specific' is EXACTLY the same for all TI_Components
shared_ptr<AbstractProduct> Concrete::get_task_specific(
    [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) const {
    shared_ptr<ConcreteProduct> task_specific_x;

    if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific " << get_product_name() << " '" << description << "'..." << endl;
        task_specific_x = dynamic_pointer_cast<ConcreteProduct>(
            component_map->at(static_cast<const TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific " << get_product_name() << " '" << description << "'..." << endl;
        task_specific_x = create_ts(task, component_map, depth);
        component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>
                                  (static_cast<const TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}

std::shared_ptr<ConcreteProduct> Concrete::create_ts(
    const shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) const {
    return make_shared<BlindSearchHeuristic>(
                                             task_transformation->get_task_specific(
                                                 task, component_map,
                                                 depth >= 0 ? depth + 1 : depth),
                                            cache_evaluator_values,
                                            description,
                                            verbosity
                                            );
}


class BlindSearchHeuristicFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentBlindSearchHeuristic> {
public:
    BlindSearchHeuristicFeature() : TypedFeature("blind") {
        document_title("Blind heuristic");
        document_synopsis(
            "Returns cost of cheapest action for non-goal states, "
            "0 for goal states");

        add_heuristic_options_to_feature(*this, "blind");

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "supported");
        document_language_support("axioms", "supported");

        document_property("admissible", "yes");
        document_property("consistent", "yes");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<TaskIndependentBlindSearchHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<TaskIndependentBlindSearchHeuristic>(
            get_heuristic_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<BlindSearchHeuristicFeature> _plugin;
}
