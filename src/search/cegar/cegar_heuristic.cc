#include "cegar_heuristic.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <vector>

using namespace std;

namespace cegar_heuristic {

CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      max_states(opts.get<int>("max_states")),
      pick_strategy(PickStrategy(opts.get_enum("pick_strategy"))) {
    if (max_states == -1)
        max_states = INFINITY;
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Peak memory before refining: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Initializing cegar heuristic..." << endl;
    abstraction = Abstraction();
    bool success = false;
    int num_states;
    for (num_states = 1; num_states < max_states; ++num_states) {
        if (num_states % 100 == 0)
            cout << "Abstract states: " << num_states << "/" << max_states << endl;
        bool solution_found = abstraction.find_solution();
        assert(solution_found);
        if (DEBUG)
            cout << "SOLUTION: " << abstraction.get_solution_string() << endl;
        success = abstraction.check_solution();
        if (DEBUG)
            cout << "NEXT ARCS: " << abstraction.get_init()->get_next_as_string()
                 << endl;
        if (success)
            break;
    }
    cout << "Done refining [t=" << g_timer << "]" << endl;
    cout << "Peak memory after refining: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Solution found while refining: " << success << endl;
    cout << "Abstract states: " << num_states << endl;
    if (!success)
        assert(num_states == max_states);
    abstraction.calculate_costs();

    // Unreachable goal.
    g_goal.clear();
    g_goal.push_back(make_pair(0, 0));
    g_goal.push_back(make_pair(0, 1));
}

int CegarHeuristic::compute_heuristic(const State &state) {
    return abstraction.get_abstract_state(state)->get_distance();
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states", 100, "maximum number of abstract states");
    vector<string> pick_strategies;
    pick_strategies.push_back("FIRST");
    pick_strategies.push_back("RANDOM");
    parser.add_enum_option("pick_strategy", pick_strategies, "FIRST",
                           "how to pick next unsatisfied condition");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);
}
