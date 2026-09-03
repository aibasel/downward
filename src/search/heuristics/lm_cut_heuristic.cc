#include "lm_cut_heuristic.h"

#include "lm_cut_landmarks.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include "../utils/markup.h"

#include <iostream>

using namespace std;

namespace lm_cut_heuristic {
LandmarkCutHeuristic::LandmarkCutHeuristic(
    const shared_ptr<AbstractTask> &task, bool use_goal_zone_detection,
    bool use_border_detection, bool cache_estimates, const string &description,
    utils::Verbosity verbosity)
    : Heuristic(task, cache_estimates, description, verbosity),
      landmark_generator(make_unique<LandmarkCutLandmarks>(
          task_proxy, use_goal_zone_detection, use_border_detection)) {
    if (log.is_at_least_normal()) {
        log << "Initializing landmark cut heuristic..." << endl;
    }
}

int LandmarkCutHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int total_cost = 0;
    bool dead_end = landmark_generator->compute_landmarks(
        state, [&total_cost](int cut_cost) { total_cost += cut_cost; },
        nullptr);

    if (dead_end)
        return DEAD_END;
    return total_cost;
}

class LandmarkCutHeuristicFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator> {
public:
    LandmarkCutHeuristicFeature() : TypedFeature("lmcut") {
        document_title("Landmark-cut heuristic");
        document_synopsis(
            "This heuristic was introduced in the following paper:" +
            utils::format_conference_reference(
                {"Malte Helmert", "Carmel Domshlak"},
                "Landmarks, Critical Paths and Abstractions: What's the "
                "Difference Anyway?",
                "https://ai.dmi.unibas.ch/papers/helmert-domshlak-icaps2009.pdf",
                "Proceedings of the 19th International Conference on Automated "
                "Planning and Scheduling (ICAPS 2009)",
                "162-169", "", "2009") +
            "\n" +
            "The tie-breaking strategies for the precondition choice function "
            "(options {{{goal_zone_detection}}} and {{{border_detection}}}) "
            "are described in the following paper:" +
            utils::format_conference_reference(
                {"Pascal Lauer", "Maximilian Fickert"},
                "Beating LM-cut with LM-cut: Quick Cutting and Practical Tie "
                "Breaking for the Precondition Choice Function",
                "https://fai.cs.uni-saarland.de/lauer/papers/hsdip2020.pdf",
                "Proceedings of the 12th Workshop on Heuristic Search for "
                "Domain-Independent Planning (HSDIP 2020)",
                "9-15", "", "2020"));

        add_landmark_cut_landmarks_options_to_feature(*this);
        add_heuristic_options_to_feature(*this, "lmcut");

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<
            LandmarkCutHeuristic, Evaluator>(
            get_landmark_cut_landmarks_arguments_from_options(opts),
            get_heuristic_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkCutHeuristicFeature> _plugin;
}
