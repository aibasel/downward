#include "blind_search_heuristic.h"

#include "../plugins/plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

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
        log << "Initializing blind search heuristic..." << endl;
    }
}

int BlindSearchHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    if (task_properties::is_goal_state(task_proxy, state))
        return 0;
    else
        return min_operator_cost;
}

class BlindSearchHeuristicFeature
    : public plugins::TypedFeature<Evaluator, BlindSearchHeuristic> {
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

    virtual shared_ptr<BlindSearchHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<BlindSearchHeuristic>(
            get_heuristic_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<BlindSearchHeuristicFeature> _plugin;
}
