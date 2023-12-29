#include "lm_cut_heuristic.h"

#include "lm_cut_landmarks.h"

#include "../tasks/cost_adapted_task.h"
#include "../plugins/plugin.h"

#include <iostream>

using namespace std;

namespace lm_cut_heuristic {
LandmarkCutHeuristic::LandmarkCutHeuristic(const string &name,
                                           utils::Verbosity verbosity,
                                           const shared_ptr<AbstractTask> task,
                                           bool cache_evaluator_values)
    : Heuristic(name, verbosity, task, cache_evaluator_values),
      landmark_generator(utils::make_unique_ptr<LandmarkCutLandmarks>(task_proxy)) {
    if (log.is_at_least_normal()) {
        log << "Initializing landmark cut heuristic named '" << name << "'..." << endl;
    }
}

LandmarkCutHeuristic::~LandmarkCutHeuristic() {
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
    const string &name,
    utils::Verbosity verbosity,
    shared_ptr<TaskIndependentAbstractTask> task_transformation,
    bool cache_evaluator_values)
    : TaskIndependentHeuristic(name, verbosity, task_transformation, cache_evaluator_values) {
}

TaskIndependentLandmarkCutHeuristic::~TaskIndependentLandmarkCutHeuristic() {
}


shared_ptr<Evaluator> TaskIndependentLandmarkCutHeuristic::create_task_specific(
    const shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) const {
    shared_ptr<LandmarkCutHeuristic> task_specific_x;
    if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific LandmarkCutHeuristic '" + name + "'..." << endl;
        task_specific_x = dynamic_pointer_cast<LandmarkCutHeuristic>(
            component_map->at(static_cast<const TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific LandmarkCutHeuristic '" + name + "'..." << endl;

        task_specific_x = make_shared<LandmarkCutHeuristic>(name,
                                                            verbosity,
                                                            task_transformation->create_task_specific(
                                                                task, component_map,
                                                                depth >= 0 ? depth + 1 : depth),
                                                            cache_evaluator_values);
        component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>(
                                  static_cast<const TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}


class LandmarkCutHeuristicFeature : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentLandmarkCutHeuristic> {
public:
    LandmarkCutHeuristicFeature() : TypedFeature("lmcut") {
        document_title("Landmark-cut heuristic");

        Heuristic::add_options_to_feature(*this, "lmcut");

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<TaskIndependentLandmarkCutHeuristic> create_component(
        const plugins::Options &opts, const utils::Context &) const override {
        return make_shared<TaskIndependentLandmarkCutHeuristic>(opts.get<string>("name"),
                                                                opts.get<utils::Verbosity>("verbosity"),
                                                                opts.get<shared_ptr<TaskIndependentAbstractTask>>(
                                                                        "transform"),
                                                                opts.get<bool>("cache_estimates")
                                                                );
    }
};

static plugins::FeaturePlugin<LandmarkCutHeuristicFeature> _plugin;
}
