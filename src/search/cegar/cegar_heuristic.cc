#include "cegar_heuristic.h"

#include "abstraction.h"
#include "abstract_state.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <vector>

using namespace std;

namespace cegar_heuristic {
CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      h_updates(opts.get<int>("h_updates")),
      search(opts.get<bool>("search")) {
    DEBUG = opts.get<bool>("debug");
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    int max_states_offline = options.get<int>("max_states_offline");
    if (max_states_offline == -1)
        max_states_offline = INFINITY;
    int max_states_online = options.get<int>("max_states_online");
    if (max_states_online == -1)
        max_states_online = INFINITY;
    int max_time = options.get<int>("max_time");
    if (max_time == -1)
        max_time = INFINITY;

    if ((h_updates > 0) && (max_states_offline == INFINITY)) {
        cout << "If h_updates > 0, max_states_offline must be finite" << endl;
        exit(2);
    }

    abstraction = new Abstraction();
    if (max_states_online > 0)
        g_cegar_abstraction = abstraction;

    abstraction->set_max_states_offline(max_states_offline);
    abstraction->set_max_states_online(max_states_online);
    abstraction->set_max_time(max_time);
    abstraction->set_pick_strategy(PickStrategy(options.get_enum("pick")));
    abstraction->set_use_astar(options.get<bool>("use_astar"));
    abstraction->set_use_new_arc_check(options.get<bool>("new_arc_check"));
    abstraction->set_log_h(options.get<bool>("log_h"));
    abstraction->set_probability_for_random_start(options.get<double>("random"));

    abstraction->build(h_updates);

    if (!search)
        exit(0);
}

int CegarHeuristic::compute_heuristic(const State &state) {
    int h = abstraction->get_abstract_state(state)->get_h();
    assert(h >= 0);
    if (h == INFINITY)
        h = DEAD_END;
    return h;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states_offline", 10000, "maximum number of abstract states created offline");
    parser.add_option<int>("max_states_online", 0, "maximum number of abstract states created online");
    parser.add_option<int>("max_time", -1, "maximum time in seconds for building the abstraction");
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
    pick_strategies.push_back("ALL");
    parser.add_enum_option("pick", pick_strategies, "RANDOM",
                           "how to pick the next unsatisfied condition");
    parser.add_option<int>("h_updates", -1, "how often to update the abstract h-values");
    parser.add_option<bool>("search", true, "if set to false, abort after refining");
    parser.add_option<bool>("debug", false, "print debugging output");
    parser.add_option<bool>("use_astar", true, "find abstract solution with A* or Dijkstra");
    parser.add_option<bool>("new_arc_check", true, "use faster check for adding arcs");
    parser.add_option<bool>("log_h", false, "log development of init-h and avg-h");
    parser.add_option<double>("random", 0, "probability with which we choose a random start state");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);
}
