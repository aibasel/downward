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

class LandmarkCutHeuristicFeature : public plugins::TypedFeature<Evaluator, LandmarkCutHeuristic> {
public:
    LandmarkCutHeuristicFeature() : TypedFeature("lmcut") {
        document_title("Landmark-cut heuristic");

        Heuristic::add_options_to_feature(*this);

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<LandmarkCutHeuristic> create_component(
        const plugins::Options &opts, const utils::Context &) const override {
        return make_shared<LandmarkCutHeuristic>(opts.get_unparsed_config(),
                                                 utils::get_log_from_options(opts),
                                                 opts.get<bool>("cache_estimates"),
                                                 opts.get<shared_ptr<AbstractTask>>("transform"));
    }
};

static plugins::FeaturePlugin<LandmarkCutHeuristicFeature> _plugin;
}
