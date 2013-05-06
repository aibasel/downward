#include "cegar_sum_heuristic.h"

#include "abstraction.h"
#include "abstract_state.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

using namespace std;

namespace cegar_heuristic {
CegarSumHeuristic::CegarSumHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      search(opts.get<bool>("search")),
      goal_order(GoalOrder(options.get_enum("goal_order"))) {
    DEBUG = opts.get<bool>("debug");
}

CegarSumHeuristic::~CegarSumHeuristic() {
}

bool sort_cg_forward(pair<int, int> atom1, pair<int, int> atom2) {
    return g_causal_graph_ordering_pos[atom1.first] < g_causal_graph_ordering_pos[atom2.first];
}

bool sort_domain_size_up(pair<int, int> atom1, pair<int, int> atom2) {
    return g_variable_domain[atom1.first] < g_variable_domain[atom2.first];
}

void CegarSumHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    int max_states_offline = options.get<int>("max_states_offline");
    if (max_states_offline == -1)
        max_states_offline = INF;
    int max_time = options.get<int>("max_time");
    if (max_time == -1)
        max_time = INF;

    // Do not restrict the number of states if a limit has been set.
    if (max_states_offline == DEFAULT_STATES_OFFLINE && max_time != INF)
        max_states_offline = INF;

    vector<pair<int, int> > goal(g_original_goal);

    // Goal ordering strategies.
    if (goal_order == ORIGINAL) {
        // Nothing to do.
    } else if (goal_order == MIXED) {
        random_shuffle(goal.begin(), goal.end());
    } else if (goal_order == CG_FORWARD or goal_order == CG_BACKWARD) {
        sort(goal.begin(), goal.end(), sort_cg_forward);
        if (goal_order == CG_BACKWARD)
            reverse(goal.begin(), goal.end());
    } else if (goal_order == DOMAIN_SIZE_UP or goal_order == DOMAIN_SIZE_DOWN) {
        sort(goal.begin(), goal.end(), sort_domain_size_up);
        if (goal_order == DOMAIN_SIZE_DOWN)
            reverse(goal.begin(), goal.end());
    } else {
        cerr << "Not a valid goal ordering strategy: " << goal_order << endl;
        exit(1);
    }
    assert(goal.size() == g_original_goal.size());

    vector<PickStrategy> best_pick_strategies;
    best_pick_strategies.push_back(MAX_REFINED);
    best_pick_strategies.push_back(MAX_PREDECESSORS);
    best_pick_strategies.push_back(GOAL);

    // In unit-cost tasks, operators might be assigned a cost of 0, so we can't
    // use BFS.
    g_is_unit_cost = false;

    int states_offline = 0;
    for (int i = 0; i < goal.size(); ++i) {
        // Set specific goal.
        g_goal.clear();
        g_goal.push_back(goal[i]);

        Abstraction *abstraction = new Abstraction();

        abstraction->set_max_states_offline(max_states_offline - states_offline);
        abstraction->set_max_time((max_time - g_timer()) / (g_original_goal.size() - i));
        abstraction->set_log_h(options.get<bool>("log_h"));

        PickStrategy pick_strategy = PickStrategy(options.get_enum("pick"));
        if (pick_strategy == BEST2) {
            pick_strategy = best_pick_strategies[i % 2];
        }
        cout << "Refine for " << goal[i].first << "=" << goal[i].second
             << " with strategy " << pick_strategy << endl;
        abstraction->set_pick_strategy(pick_strategy);

        abstraction->build();
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
    reset_original_goals_and_costs();

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
        if (h == INF)
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
    pick_strategies.push_back("MIN_OPS");
    pick_strategies.push_back("MAX_OPS");
    parser.add_enum_option("pick", pick_strategies, "RANDOM",
                           "how to pick the next unsatisfied condition");
    vector<string> goal_order_strategies;
    goal_order_strategies.push_back("ORIGINAL");
    goal_order_strategies.push_back("MIXED");
    goal_order_strategies.push_back("CG_FORWARD");
    goal_order_strategies.push_back("CG_BACKWARD");
    goal_order_strategies.push_back("DOMAIN_SIZE_UP");
    goal_order_strategies.push_back("DOMAIN_SIZE_DOWN");
    parser.add_enum_option("goal_order", goal_order_strategies, "ORIGINAL",
                           "order in which the goals are refined for");
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
