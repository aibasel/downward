#include "goal_count_heuristic.h"

#include "globals.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"




GoalCountHeuristic::GoalCountHeuristic(const Options &opts)
    : Heuristic(opts) {
}

GoalCountHeuristic::~GoalCountHeuristic() {
}

void GoalCountHeuristic::initialize() {
    cout << "Initializing goal count heuristic..." << endl;
}

int GoalCountHeuristic::compute_heuristic(const State &state) {
    int unsatisfied_goal_count = 0;
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first, value = g_goal[i].second;
        if (state[var] != value)
            unsatisfied_goal_count++;
    }
    return unsatisfied_goal_count;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new GoalCountHeuristic(opts);
}


static Plugin<ScalarEvaluator> _plugin("goalcount", _parse);
