#include "lm_cut_heuristic.h"

#include "lm_cut_landmarks.h"

#include "../tasks/cost_adapted_task.h"
#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <iostream>

using namespace std;

namespace lm_cut_heuristic {
LandmarkCutHeuristic::LandmarkCutHeuristic(
    const shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(transform, cache_estimates, description, verbosity),
      landmark_generator(utils::make_unique_ptr<LandmarkCutLandmarks>(task_proxy)) {
    if (log.is_at_least_normal()) {
        log << "Initializing landmark cut heuristic named '" << description << "'..." << endl;
    }
}

int LandmarkCutHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int total_cost = 0;
    bool dead_end = landmark_generator->compute_landmarks(
        state,
        [&total_cost](int cut_cost) {total_cost += cut_cost;},
        nullptr);

    if (dead_end)
        return DEAD_END;
    return total_cost;
}



TaskIndependentLandmarkCutHeuristic::TaskIndependentLandmarkCutHeuristic(
    const shared_ptr<TaskIndependentAbstractTask> transform,
    bool cache_estimates,
    const string &description,
    utils::Verbosity verbosity)
    : TaskIndependentHeuristic(transform, cache_estimates, description, verbosity) {
}

TaskIndependentLandmarkCutHeuristic::~TaskIndependentLandmarkCutHeuristic() {
}

std::shared_ptr<Evaluator> TaskIndependentLandmarkCutHeuristic::create_task_specific(
    const shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) const {
    return make_shared<LandmarkCutHeuristic>(
        task_transformation->get_task_specific(
            task, component_map,
            depth >= 0 ? depth + 1 : depth),
        cache_evaluator_values,
        description,
        verbosity
        );
}


class LandmarkCutHeuristicFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentLandmarkCutHeuristic> {
public:
    LandmarkCutHeuristicFeature() : TypedFeature("lmcut") {
        document_title("Landmark-cut heuristic");

        add_heuristic_options_to_feature(*this, "lmcut");

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<TaskIndependentLandmarkCutHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<TaskIndependentLandmarkCutHeuristic>(
            get_heuristic_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<LandmarkCutHeuristicFeature> _plugin;
}
