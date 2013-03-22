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

    // Vector with operator costs. These will be set to zero by the abstractions.
    vector<int> admissible_operator_costs;
    for (int i = 0; i < g_operators.size(); ++i) {
        admissible_operator_costs.push_back(g_operators[i].get_cost());
    }

    for (int i = 0; i < g_goal.size(); ++i) {
        // Set specific goal.
        g_cegar_goal.clear();
        g_cegar_goal.push_back(g_goal[i]);

        Abstraction *abstraction = new Abstraction();
        abstraction->set_operator_costs(&admissible_operator_costs);

        abstraction->set_max_states_offline(max_states_offline);
        abstraction->set_max_time(max_time);
        abstraction->set_pick_strategy(PickStrategy(options.get_enum("pick")));
        abstraction->set_log_h(options.get<bool>("log_h"));

        abstraction->build(h_updates);

        // Set operator costs to zero for operators that lead from state a1 to
        // state a2 for which h(a1) > h(a2).
        abstraction->adapt_operator_costs();

        abstraction->release_memory();

        abstractions.push_back(abstraction);
    }

    if (!search)
        exit(0);
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
