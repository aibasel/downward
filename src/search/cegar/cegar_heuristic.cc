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
    : Heuristic(opts) {
    int max_states = opts.get<int>("max_states");
    if (max_states == -1)
        max_states = INFINITY;
    this->max_states = max_states;

    abstraction = Abstraction(PickStrategy(opts.get_enum("pick")));
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Peak memory before refining: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Initializing cegar heuristic..." << endl;
    int saved_searches = 0;
    int updates = 0;
    bool success = false;
    int num_states = abstraction.get_num_states();
    while (num_states < max_states) {
        //abstraction.write_dot_file(num_states);
        if (num_states % 100 == 0)
            cout << "Abstract states: " << num_states << "/" << max_states << endl;
        if (!abstraction.can_reuse_last_solution()) {
            abstraction.find_solution();
        } else {
            ++saved_searches;
        }
        success = abstraction.check_solution();
        num_states = abstraction.get_num_states();
        if (success)
            break;
        // Update costs to goal COST_UPDATES times evenly distributed over time.
        if (num_states % ((max_states / (COST_UPDATES + 1)) + 1) == 0) {
            abstraction.update_h_values();
            ++updates;
        }
    }
    cout << "Done refining [t=" << g_timer << "]" << endl;
    cout << "Peak memory after refining: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Solution found while refining: " << success << endl;
    cout << "Abstract states: " << num_states << endl;
    cout << "SAME: " << same << " DIFFERENT: " << different << " DOUBLES: " << doubles << endl;
    cout << "Saved searches: " << saved_searches << endl;
    cout << "A* expansions: " << abstraction.get_num_expansions() << endl;
    if (TEST_WITH_DIJKSTRA) {
        cout << "Dijkstra expansions: " << abstraction.get_num_expansions_dijkstra() << endl;
        cout << "Ratio A*/Dijkstra: "
             << abstraction.get_num_expansions() / float(abstraction.get_num_expansions_dijkstra()) << endl;
    }
    if (!success)
        assert(num_states == max_states);
    abstraction.update_h_values();
    assert(num_states == abstraction.get_num_states());
    if (num_states == max_states)
        assert(updates == COST_UPDATES);

    // Unreachable goal.
    //g_goal.clear();
    //g_goal.push_back(make_pair(0, 0));
    //g_goal.push_back(make_pair(0, 1));
}

int CegarHeuristic::compute_heuristic(const State &state) {
    int dist = abstraction.get_abstract_state(state)->get_h();
    assert(dist >= 0);
    if (dist == INFINITY)
        dist = DEAD_END;
    return dist;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states", 100, "maximum number of abstract states");
    vector<string> pick_strategies;
    pick_strategies.push_back("FIRST");
    pick_strategies.push_back("RANDOM");
    pick_strategies.push_back("GOAL");
    parser.add_enum_option("pick", pick_strategies, "FIRST",
                           "how to pick the next unsatisfied condition");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);
}
