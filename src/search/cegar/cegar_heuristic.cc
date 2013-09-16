#include "cegar_heuristic.h"

#include "abstraction.h"
#include "abstract_state.h"
#include "task.h"
#include "utils.h"
#include "../additive_heuristic.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

using namespace std;

namespace cegar_heuristic {
CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      search(opts.get<bool>("search")) {
    DEBUG = opts.get<bool>("debug");
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    cout << "Unit-cost: " << g_is_unit_cost << endl;
    int max_states_offline = options.get<int>("max_states_offline");
    if (max_states_offline == -1)
        max_states_offline = INF;
    int max_states_online = options.get<int>("max_states_online");
    if (max_states_online == -1)
        max_states_online = INF;
    int max_time = options.get<int>("max_time");
    if (max_time == -1)
        max_time = INF;

    // Do not restrict the number of states if a limit has been set.
    if (max_states_offline == DEFAULT_STATES_OFFLINE && max_time != INF)
        max_states_offline = INF;

    Task task;
    task.goal = g_goal;
    task.operators = g_operators;
    for (int i = 0; i < task.operators.size(); ++i)
        task.original_operator_numbers.push_back(i);
    task.variable_domain = g_variable_domain;

    abstraction = new Abstraction(&task);
    if (max_states_online > 0)
        g_cegar_abstraction = abstraction;

    double factor = options.get<double>("init_h_factor");
    if (factor != -1) {
        abstraction->set_max_init_h_factor(factor);
    }

    abstraction->set_max_states_offline(max_states_offline);
    abstraction->set_max_states_online(max_states_online);
    abstraction->set_max_time(max_time);
    abstraction->set_pick_strategy(PickStrategy(options.get_enum("pick")));
    abstraction->set_use_astar(options.get<bool>("use_astar"));
    abstraction->set_log_h(options.get<bool>("log_h"));
    abstraction->set_write_dot_files(options.get<bool>("write_dot_files"));

    abstraction->build();
    abstraction->print_statistics();
    if (max_states_online <= 0)
        abstraction->release_memory();

    if (!search)
        exit_with(EXIT_UNSOLVED_INCOMPLETE);
}

int CegarHeuristic::compute_heuristic(const State &state) {
    int h = abstraction->get_h(state);
    assert(h >= 0);
    if (h == INF)
        h = DEAD_END;
    return h;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states_offline", DEFAULT_STATES_OFFLINE,
                           "maximum number of abstract states created offline");
    parser.add_option<int>("max_states_online", 0, "maximum number of abstract states created online");
    parser.add_option<int>("max_time", -1, "maximum time in seconds for building the abstraction");
    parser.add_option<double>("init_h_factor", -1, "stop refinement after h(s_0) reaches h^add(s_0) * factor");
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
    pick_strategies.push_back("BEST2");
    pick_strategies.push_back("MIN_OPS");
    pick_strategies.push_back("MAX_OPS");
    pick_strategies.push_back("MIN_HADD");
    pick_strategies.push_back("MAX_HADD");
    pick_strategies.push_back("MIN_LM");
    pick_strategies.push_back("MAX_LM");
    pick_strategies.push_back("MIN_HADD_MIN_LM");
    pick_strategies.push_back("MIN_HADD_MAX_LM");
    pick_strategies.push_back("MAX_HADD_MIN_LM");
    pick_strategies.push_back("MAX_HADD_MAX_LM");
    pick_strategies.push_back("MIN_HADD_DYN");
    pick_strategies.push_back("MAX_HADD_DYN");
    parser.add_enum_option("pick", pick_strategies, "RANDOM",
                           "how to pick the next unsatisfied condition");
    parser.add_option<bool>("search", true, "if set to false, abort after refining");
    parser.add_option<bool>("use_astar", true, "use A* for finding the *single* next solution");
    parser.add_option<bool>("debug", false, "print debugging output");
    parser.add_option<bool>("log_h", false, "log development of init-h and avg-h");
    parser.add_option<bool>("write_dot_files", false, "write graph files for debugging");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);
}
