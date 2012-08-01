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
      h_updates(opts.get<int>("h_updates")),
      search(opts.get<bool>("search")),
      abstraction(PickStrategy(opts.get_enum("pick_deviation")),
                  PickStrategy(opts.get_enum("pick_precondition")),
                  PickStrategy(opts.get_enum("pick_goal"))) {
    if (max_states == -1)
        max_states = INFINITY;
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Peak memory before building abstraction: "
         << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Initializing cegar heuristic..." << endl;
    int saved_searches = 0;
    int updates = 0;
    const int update_step = (max_states / (h_updates + 1)) + 1;
    bool success = false;
    int num_states = abstraction.get_num_states();
    int logged_states = 0;
    const int states_log_step = 100;
    if (WRITE_DOT_FILES)
        write_causal_graph(*g_causal_graph);
    while (num_states < max_states) {
        if (WRITE_DOT_FILES)
            abstraction.write_dot_file(num_states);
        if (num_states - logged_states >= states_log_step) {
            cout << "Abstract states: "
                 << num_states << "/" << max_states << endl;
            logged_states += states_log_step;
        }
        if (!abstraction.can_reuse_last_solution()) {
            abstraction.find_solution();
        } else {
            ++saved_searches;
        }
        success = abstraction.check_solution();
        num_states = abstraction.get_num_states();
        if (success)
            break;
        // Update costs to goal evenly distributed over time.
        if (num_states >= (updates + 1) * update_step) {
            abstraction.update_h_values();
            ++updates;
        }
    }
    if (WRITE_DOT_FILES)
        abstraction.write_dot_file(num_states);
    cout << "Done building abstraction [t=" << g_timer << "]" << endl;
    cout << "Peak memory after building abstraction: "
         << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Solution found while refining: " << success << endl;
    cout << "Abstract states: " << num_states << endl;
    cout << "Cost updates: " << updates << "/" << h_updates << endl;
    cout << "Saved searches: " << saved_searches << endl;
    cout << "A* expansions: " << abstraction.get_num_expansions() << endl;
    if (TEST_WITH_DIJKSTRA) {
        cout << "Dijkstra expansions: "
             << abstraction.get_num_expansions_dijkstra() << endl;
        cout << "Ratio A*/Dijkstra: "
             << abstraction.get_num_expansions() /
        float(abstraction.get_num_expansions_dijkstra()) << endl;
    }
    if (!success)
        assert(num_states >= max_states);
    abstraction.update_h_values();
    assert(num_states == abstraction.get_num_states());
    abstraction.print_statistics();
    abstraction.release_memory();

    if (!search)
        exit(0);

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
    pick_strategies.push_back("NO_GOAL");
    pick_strategies.push_back("MIN_CONSTRAINED");
    pick_strategies.push_back("MAX_CONSTRAINED");
    pick_strategies.push_back("MIN_REFINED");
    pick_strategies.push_back("MAX_REFINED");
    pick_strategies.push_back("MIN_PREDECESSORS");
    pick_strategies.push_back("MAX_PREDECESSORS");
    pick_strategies.push_back("BREAK");
    pick_strategies.push_back("KEEP");
    pick_strategies.push_back("ALL");
    parser.add_enum_option("pick_deviation", pick_strategies, "FIRST",
                           "how to pick the next unsatisfied deviation precondition");
    parser.add_enum_option("pick_precondition", pick_strategies, "FIRST",
                           "how to pick the next unsatisfied precondition");
    parser.add_enum_option("pick_goal", pick_strategies, "FIRST",
                           "how to pick the next unsatisfied goal");
    parser.add_option<int>("h_updates", 3, "how often to update the abstract h-values");
    parser.add_option<bool>("search", true, "if set to false, abort after refining");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);
}
