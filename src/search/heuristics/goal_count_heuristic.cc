#include "goal_count_heuristic.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"

#include <iostream>
using namespace std;

namespace goal_count_heuristic {
GoalCountHeuristic::GoalCountHeuristic(const Options &opts)
    : Heuristic(opts) {
    utils::g_log << "Initializing goal count heuristic..." << endl;
}

int GoalCountHeuristic::compute_heuristic(const GlobalState &global_state) {
    const State state = convert_global_state(global_state);
    int unsatisfied_goal_count = 0;

    for (FactProxy goal : task_proxy.get_goals()) {
        const VariableProxy var = goal.get_variable();
        if (state[var] != goal) {
            ++unsatisfied_goal_count;
        }
    }
    return unsatisfied_goal_count;
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis("Goal count heuristic", "");
    parser.document_language_support("action costs", "ignored by design");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support("axioms", "supported");
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<GoalCountHeuristic>(opts);
}


static Plugin<Evaluator> _plugin("goalcount", _parse);
}
