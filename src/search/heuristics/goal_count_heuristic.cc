#include "goal_count_heuristic.h"

#include "../plugins/plugin.h"

#include "../utils/logging.h"

#include <iostream>
using namespace std;

namespace goal_count_heuristic {
GoalCountHeuristic::GoalCountHeuristic(const plugins::Options &opts)
    : Heuristic(opts) {
    if (log.is_at_least_normal()) {
        log << "Initializing goal count heuristic..." << endl;
    }
}

int GoalCountHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int unsatisfied_goal_count = 0;

    for (FactProxy goal : task_proxy.get_goals()) {
        const VariableProxy var = goal.get_variable();
        if (state[var] != goal) {
            ++unsatisfied_goal_count;
        }
    }
    return unsatisfied_goal_count;
}

class GoalCountHeuristicFeature : public plugins::TypedFeature<Evaluator, GoalCountHeuristic> {
public:
    GoalCountHeuristicFeature() : TypedFeature("goalcount") {
        document_title("Goal count heuristic");

        Heuristic::add_options_to_feature(*this);

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "supported");
        document_language_support("axioms", "supported");

        document_property("admissible", "no");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }
};

static plugins::FeaturePlugin<GoalCountHeuristicFeature> _plugin;
}
