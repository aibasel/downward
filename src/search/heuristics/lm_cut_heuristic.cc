#include "lm_cut_heuristic.h"

#include "lm_cut_landmarks.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <iostream>

using namespace std;

namespace lm_cut_heuristic {
LandmarkCutHeuristic::LandmarkCutHeuristic(basic_string<char> unparsed_config,
                                           utils::LogProxy log,
                                           bool cache_evaluator_values,
                                           shared_ptr<AbstractTask> task)
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



TaskIndependentLandmarkCutHeuristic::TaskIndependentLandmarkCutHeuristic(string unparsed_config, utils::LogProxy log, bool cache_evaluator_values)
    : TaskIndependentHeuristic(unparsed_config, log, cache_evaluator_values),
      unparsed_config(unparsed_config), log(log), cache_evaluator_values(cache_evaluator_values) {
}

TaskIndependentLandmarkCutHeuristic::~TaskIndependentLandmarkCutHeuristic() {
}

plugins::Any TaskIndependentLandmarkCutHeuristic::create_task_specific(shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) {
    shared_ptr<LandmarkCutHeuristic> task_specific_lm_cut_heurisitc;
    plugins::Any any_obj;
    if (component_map -> contains_key(make_pair(task, static_cast<void*>(this)))){
        utils::g_log << "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRReuse task specific LandmarkCutHeuristic..." << endl;
        any_obj = component_map -> get_value(make_pair(task, static_cast<void*>(this)));
    } else {
        utils::g_log << "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCreating task specific LandmarkCutHeuristic..." << endl;
        task_specific_lm_cut_heurisitc = make_shared<LandmarkCutHeuristic>(unparsed_config, log, cache_evaluator_values, task);
        any_obj = plugins::Any(task_specific_lm_cut_heurisitc);
        component_map -> add_entry(make_pair(task, static_cast<void*>(this)), any_obj);
    }
    return any_obj;
}

class TaskIndependentLandmarkCutHeuristicFeature : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentLandmarkCutHeuristic> {
public:
    TaskIndependentLandmarkCutHeuristicFeature() : TypedFeature("lmcut") {
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

    virtual shared_ptr<LandmarkCutHeuristic> create_TD_component(
        const plugins::Options &opts, const utils::Context &) const {
        return make_shared<LandmarkCutHeuristic>(opts.get_unparsed_config(),
                                                 utils::get_log_from_options(opts),
                                                 opts.get<bool>("cache_estimates"),
                                                 opts.get<shared_ptr<AbstractTask>>("transform"));
    }

    virtual shared_ptr<TaskIndependentLandmarkCutHeuristic> create_component(
        const plugins::Options &opts, const utils::Context &) const override {
        return make_shared<TaskIndependentLandmarkCutHeuristic>(opts.get_unparsed_config(),
                                                                utils::get_log_from_options(opts),
                                                                opts.get<bool>("cache_estimates")
                                                                );
    }
};

static plugins::FeaturePlugin<TaskIndependentLandmarkCutHeuristicFeature> _plugin;
}
