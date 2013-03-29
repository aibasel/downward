#include "cegar_sum_heuristic.h"

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
CegarSumHeuristic::CegarSumHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      h_updates(opts.get<int>("h_updates")),
      search(opts.get<bool>("search")) {
    DEBUG = opts.get<bool>("debug");
}

CegarSumHeuristic::~CegarSumHeuristic() {
}

void CegarSumHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    int max_states_offline = options.get<int>("max_states_offline");
    if (max_states_offline == -1)
        max_states_offline = INFINITY;
    int max_time = options.get<int>("max_time");
    if (max_time == -1)
        max_time = INFINITY;

    if ((h_updates > 0) && (max_states_offline == INFINITY)) {
        cout << "If h_updates > 0, max_states_offline must be finite" << endl;
        exit(2);
    }

    // Do not restrict the number of states if a limit has been set.
    if (max_states_offline == DEFAULT_STATES_OFFLINE && max_time != INFINITY)
        max_states_offline = INFINITY;

    vector<PickStrategy> best_pick_strategies;
    best_pick_strategies.push_back(MAX_REFINED);
    best_pick_strategies.push_back(MAX_PREDECESSORS);
    best_pick_strategies.push_back(GOAL);

    // Copy original goal in order to be able to change it for additive abstractions.
    g_original_goal = g_goal;

    int states_offline = 0;
    for (int i = 0; i < g_original_goal.size(); ++i) {
        // Set specific goal.
        g_goal.clear();
        g_goal.push_back(g_original_goal[i]);

        Abstraction *abstraction = new Abstraction();

        abstraction->set_max_states_offline(max_states_offline - states_offline);
        abstraction->set_max_time((max_time - g_timer()) / g_goal.size());
        abstraction->set_log_h(options.get<bool>("log_h"));

        PickStrategy pick_strategy = PickStrategy(options.get_enum("pick"));
        if (pick_strategy == BEST2) {
            pick_strategy = best_pick_strategies[i % 2];
        }
        cout << "Pick strategy: " << pick_strategy << endl;
        abstraction->set_pick_strategy(pick_strategy);

        abstraction->build(h_updates);
        avg_h_values.push_back(abstraction->get_avg_h());
        abstraction->adapt_operator_costs();
        abstraction->release_memory();

        abstractions.push_back(abstraction);
        states_offline += abstraction->get_num_states();

        if (states_offline >= max_states_offline || g_timer() > max_time) {
            break;
        }
    }
    print_statistics();
    g_goal = g_original_goal;
    for (int i = 0; i < g_operators.size(); ++i)
        g_operators[i].set_cost(g_original_op_costs[i]);

    if (!search)
        exit(0);
}

void CegarSumHeuristic::print_statistics() {
    double sum_avg_h = 0;
    for (int i = 0; i < avg_h_values.size(); ++i) {
        sum_avg_h += avg_h_values[i];
    }
    cout << "CEGAR abstractions: " << abstractions.size() << endl;
    // There will always be at least one abstraction.
    cout << "Init h: " << compute_heuristic(*g_initial_state) << endl;
    cout << "Average h: " << sum_avg_h / abstractions.size() << endl;
}

int CegarSumHeuristic::compute_heuristic(const State &state) {
    int sum_h = 0;
    for (int i = 0; i < abstractions.size(); ++i) {
        int h = abstractions[i]->get_h(state);
        assert(h >= 0);
        if (h == INFINITY)
            return DEAD_END;
        sum_h += h;
    }
    assert(sum_h >= 0 && "check against overflow");
    return sum_h;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states_offline", DEFAULT_STATES_OFFLINE,
                           "maximum number of abstract states created offline");
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
    pick_strategies.push_back("BEST2");
    parser.add_enum_option("pick", pick_strategies, "RANDOM",
                           "how to pick the next unsatisfied condition");
    parser.add_option<int>("h_updates", -1, "how often to update the abstract h-values");
    parser.add_option<bool>("search", true, "if set to false, abort after refining");
    parser.add_option<bool>("debug", false, "print debugging output");
    parser.add_option<bool>("log_h", false, "log development of init-h and avg-h");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarSumHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar_sum", _parse);
}
