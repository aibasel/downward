#include "lm_cut_heuristic.h"

#include "lm_cut_landmarks.h"

#include "../tasks/cost_adapted_task.h"
#include "../plugins/plugin.h"

#include <iostream>

using namespace std;

namespace lm_cut_heuristic {
LandmarkCutHeuristic::LandmarkCutHeuristic(basic_string<char> unparsed_config,
                                           utils::LogProxy log,
                                           bool cache_evaluator_values,
                                           const shared_ptr<AbstractTask> task)
    : Heuristic(unparsed_config, log, cache_evaluator_values, task),
      landmark_generator(utils::make_unique_ptr<LandmarkCutLandmarks>(task_proxy)) {
    if (log.is_at_least_normal()) {
        log << "Initializing landmark cut heuristic..." << endl;
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



TaskIndependentLandmarkCutHeuristic::TaskIndependentLandmarkCutHeuristic(string unparsed_config, utils::LogProxy log, bool cache_evaluator_values, shared_ptr<TaskIndependentAbstractTask> task_transformation)
    : TaskIndependentHeuristic(unparsed_config, log, cache_evaluator_values, task_transformation),
      log(log) {
}

TaskIndependentLandmarkCutHeuristic::~TaskIndependentLandmarkCutHeuristic() {
}


shared_ptr<LandmarkCutHeuristic> TaskIndependentLandmarkCutHeuristic::create_task_specific_LandmarkCutHeuristic(const shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<LandmarkCutHeuristic> task_specific_lm_cut_heurisitc;
    if (component_map->contains_key(make_pair(task, static_cast<void *>(this)))) {
        log << std::string(depth, ' ') << "Reusing task LandmarkCutHeuristic..." << endl;
        task_specific_lm_cut_heurisitc = plugins::any_cast<shared_ptr<LandmarkCutHeuristic>>(
            component_map->get_dual_key_value(task, this));
    } else {
        log << std::string(depth, ' ') << "Creating task specific LandmarkCutHeuristic..." << endl;

        task_specific_lm_cut_heurisitc = make_shared<LandmarkCutHeuristic>(unparsed_config,
                                                                           log,
                                                                           cache_evaluator_values,
                                                                           task_transformation->create_task_specific_AbstractTask(task, component_map, depth >=0 ? depth+1 : depth));
        component_map->add_dual_key_entry(task, this, plugins::Any(task_specific_lm_cut_heurisitc));
    }
    return task_specific_lm_cut_heurisitc;
}


shared_ptr<LandmarkCutHeuristic> TaskIndependentLandmarkCutHeuristic::create_task_specific_LandmarkCutHeuristic(const shared_ptr<AbstractTask> &task, int depth) {
    log << std::string(depth, ' ') << "Creating LandmarkCutHeuristic as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_LandmarkCutHeuristic(task, component_map, depth);
}


shared_ptr<Heuristic> TaskIndependentLandmarkCutHeuristic::create_task_specific_Heuristic(const shared_ptr<AbstractTask> &task, int depth) {
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_Heuristic(task, component_map, depth);
}

shared_ptr<Heuristic> TaskIndependentLandmarkCutHeuristic::create_task_specific_Heuristic(const shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<LandmarkCutHeuristic> x = create_task_specific_LandmarkCutHeuristic(task, component_map, depth);
    return static_pointer_cast<Heuristic>(x);
}


class LandmarkCutHeuristicFeature : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentLandmarkCutHeuristic> {
public:
    LandmarkCutHeuristicFeature() : TypedFeature("lmcut") {
        document_title("Landmark-cut heuristic");

        TaskIndependentHeuristic::add_options_to_feature(*this);

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
        return make_shared<TaskIndependentLandmarkCutHeuristic>(opts.get_unparsed_config(),
                                                                utils::get_log_from_options(opts),
                                                                opts.get<bool>("cache_estimates"),
                                                                opts.get<shared_ptr<TaskIndependentAbstractTask>>("transform")
        );
    }
};

static plugins::FeaturePlugin<LandmarkCutHeuristicFeature> _plugin;
}
