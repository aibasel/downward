#include "goal_count_heuristic.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <iostream>
using namespace std;

namespace goal_count_heuristic {
GoalCountHeuristic::GoalCountHeuristic(
    const shared_ptr<AbstractTask> &task, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(task, cache_estimates, description, verbosity) {
    if (log.is_at_least_normal()) {
        log << "Initializing goal count heuristic..." << endl;
    }
    goals.reserve(task_proxy.get_goals().size());
    for (FactProxy goal : task_proxy.get_goals()) {
	int var = goal.get_variable().get_id();
	int val = goal.get_value();
	goals.push_back({var,val});
    }
}

int GoalCountHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int unsatisfied_goal_count = 0;
    state.unpack();
    const vector<int> &unpacked_state = state.get_unpacked_values();
    for (const auto& it : goals) {
	const int& var = it.first;
	const int& val = it.second;
        if (unpacked_state[var] != val) {
            ++unsatisfied_goal_count;
        }
    }
    return unsatisfied_goal_count;
}

class GoalCountHeuristicFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator> {
public:
    GoalCountHeuristicFeature() : TypedFeature("goalcount") {
        document_title("Goal count heuristic");

        add_heuristic_options_to_feature(*this, "goalcount");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "supported");
        document_language_support("axioms", "supported");

        document_property("admissible", "no");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<
            GoalCountHeuristic, Evaluator>(
            get_heuristic_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<GoalCountHeuristicFeature> _plugin;
}
