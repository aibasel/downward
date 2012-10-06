#include "cegar_heuristic.h"

#include "abstraction.h"
#include "abstract_state.h"
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
      max_states_offline(opts.get<int>("max_states_offline")),
      h_updates(opts.get<int>("h_updates")),
      search(opts.get<bool>("search")) {
    if (max_states_offline == -1)
        max_states_offline = INFINITY;

    DEBUG = opts.get<bool>("debug");

    abstraction = new Abstraction(PickStrategy(opts.get_enum("pick_deviation")),
                  PickStrategy(opts.get_enum("pick_precondition")),
                  PickStrategy(opts.get_enum("pick_goal")));
    g_cegar_abstraction = abstraction;
    g_cegar_abstraction_max_states_online = opts.get<int>("max_states_online");
    abstraction->set_use_astar(opts.get<bool>("use_astar"));
    bool use_new_arc_check = opts.get<bool>("new_arc_check");
    abstraction->set_use_new_arc_check(use_new_arc_check);
    cout << "Using new arc check: " << use_new_arc_check << endl;
    abstraction->set_log_h(opts.get<bool>("log_h"));
    abstraction->set_num_seen_conc_states(opts.get<int>("num_seen_conc_states"));
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    int updates = 0;
    const int update_step = (max_states_offline / (h_updates + 1)) + 1;
    bool valid_complete_conc_solution = false;
    int num_states = abstraction->get_num_states();
    int logged_states = 0;
    const int states_log_step = 1000;
    if (WRITE_DOT_FILES) {
        write_causal_graph(*g_causal_graph);
        abstraction->write_dot_file(num_states);
    }
    while (num_states < max_states_offline) {
        if (num_states - logged_states >= states_log_step) {
            cout << "Abstract states: "
                 << num_states << "/" << max_states_offline << endl;
            logged_states += states_log_step;
        }
        valid_complete_conc_solution = abstraction->find_and_break_solution();
        num_states = abstraction->get_num_states();
        if (valid_complete_conc_solution)
            break;
        if (WRITE_DOT_FILES)
            abstraction->write_dot_file(num_states);
        // Update costs to goal evenly distributed over time.
        if (num_states >= (updates + 1) * update_step) {
            abstraction->update_h_values();
            ++updates;
        }
    }
    abstraction->log_h_values();
    abstraction->remember_num_states_offline();
    cout << "Done building abstraction [t=" << g_timer << "]" << endl;
    cout << "Peak memory after building abstraction: "
         << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Solution found while refining: " << valid_complete_conc_solution << endl;
    cout << "Abstract states offline: " << num_states << endl;
    cout << "Cost updates: " << updates << "/" << h_updates << endl;
    cout << "A* expansions: " << abstraction->get_num_expansions() << endl;
    if (!valid_complete_conc_solution)
        assert(num_states >= max_states_offline);
    abstraction->update_h_values();
    assert(num_states == abstraction->get_num_states());
    abstraction->print_statistics();
    if (g_cegar_abstraction_max_states_online <= 0) {
        cout << "Release memory" << endl;
        abstraction->release_memory();
    }

    if (!search)
        exit(0);
}

int CegarHeuristic::compute_heuristic(const State &state) {
    int dist = abstraction->get_abstract_state(state)->get_h();
    assert(dist >= 0);
    if (dist == INFINITY)
        dist = DEAD_END;
    return dist;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states_offline", 100, "maximum number of abstract states created offline");
    parser.add_option<int>("max_states_online", 0, "maximum number of abstract states created online");
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
    parser.add_enum_option("pick_deviation", pick_strategies, "RANDOM",
                           "how to pick the next unsatisfied deviation precondition");
    parser.add_enum_option("pick_precondition", pick_strategies, "RANDOM",
                           "how to pick the next unsatisfied precondition");
    parser.add_enum_option("pick_goal", pick_strategies, "RANDOM",
                           "how to pick the next unsatisfied goal");
    parser.add_option<int>("h_updates", 3, "how often to update the abstract h-values");
    parser.add_option<bool>("search", true, "if set to false, abort after refining");
    parser.add_option<bool>("debug", false, "print debugging output");
    parser.add_option<bool>("use_astar", true, "find abstract solution with A* or Dijkstra");
    parser.add_option<bool>("new_arc_check", true, "use faster check for adding arcs");
    parser.add_option<bool>("log_h", false, "log development of init-h and avg-h");
    parser.add_option<int>("num_seen_conc_states", 0, "how many last seen conc states to choose from");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);
}
