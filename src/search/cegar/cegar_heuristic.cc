#include "cegar_heuristic.h"

#include "abstraction.h"
#include "abstract_state.h"
#include "utils.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../landmarks/h_m_landmarks.h"
#include "../landmarks/landmark_graph.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

using namespace std;

namespace cegar_heuristic {
CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      search(opts.get<bool>("search")),
      fact_order(GoalOrder(options.get_enum("fact_order"))),
      original_task(Task::get_original_task()),
      num_states_offline(0) {
    DEBUG = opts.get<bool>("debug");

    for (int i = 0; i < g_operators.size(); ++i)
        remaining_costs.push_back(g_operators[i].get_cost());
}

CegarHeuristic::~CegarHeuristic() {
}

bool sort_cg_forward(pair<int, int> atom1, pair<int, int> atom2) {
    return get_pos_in_causal_graph_ordering(atom1.first) < get_pos_in_causal_graph_ordering(atom2.first);
}

bool sort_domain_size_up(pair<int, int> atom1, pair<int, int> atom2) {
    return g_variable_domain[atom1.first] < g_variable_domain[atom2.first];
}

bool sort_hadd_values_up(Fact fact1, Fact fact2) {
    return get_hadd_value(fact1.first, fact1.second) < get_hadd_value(fact2.first, fact2.second);
}

void CegarHeuristic::get_fact_landmarks(vector<Fact> *facts) const {
    Options opts = Options();
    opts.set<int>("cost_type", 0);
    opts.set<int>("memory_padding", 75);
    opts.set<int>("m", 1);
    // h^m doesn't produce reasonable orders anyway.
    opts.set<bool>("reasonable_orders", false);
    opts.set<bool>("only_causal_landmarks", false);
    opts.set<bool>("disjunctive_landmarks", false);
    opts.set<bool>("conjunctive_landmarks", false);
    opts.set<bool>("no_orders", false);
    opts.set<int>("lm_cost_type", 0);
    opts.set<Exploration *>("explor", new Exploration(opts));
    HMLandmarks lm_graph_factory(opts);
    LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
    if (DEBUG)
        graph->dump();
    const set<LandmarkNode *> &nodes = graph->get_nodes();
    for (auto it = nodes.begin(); it != nodes.end(); it++) {
        const LandmarkNode *node_p = *it;
        facts->push_back(get_fact(node_p));
    }
}

void CegarHeuristic::order_facts(vector<Fact> &facts) const {
    if (fact_order == ORIGINAL) {
        // Nothing to do.
    } else if (fact_order == MIXED) {
        random_shuffle(facts.begin(), facts.end());
    } else if (fact_order == CG_FORWARD or fact_order == CG_BACKWARD) {
        sort(facts.begin(), facts.end(), sort_cg_forward);
        if (fact_order == CG_BACKWARD)
            reverse(facts.begin(), facts.end());
    } else if (fact_order == DOMAIN_SIZE_UP or fact_order == DOMAIN_SIZE_DOWN) {
        sort(facts.begin(), facts.end(), sort_domain_size_up);
        if (fact_order == DOMAIN_SIZE_DOWN)
            reverse(facts.begin(), facts.end());
    } else if (fact_order == HADD_UP or fact_order == HADD_DOWN) {
        sort(facts.begin(), facts.end(), sort_hadd_values_up);
        if (fact_order == HADD_DOWN)
            reverse(facts.begin(), facts.end());
    } else {
        cerr << "Not a valid fact ordering strategy: " << fact_order << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
}

bool is_true_in_initial_state(Fact fact) {
    return (*g_initial_state)[fact.first] == fact.second;
}

int num_nontrivial_goal_facts() {
    int count = 0;
    for (int i = 0; i < g_goal.size(); ++i) {
        if (!is_true_in_initial_state(g_goal[i]))
            ++count;
    }
    return count;
}

void CegarHeuristic::generate_tasks(vector<Task> *tasks) const {
    vector<Fact> facts;
    Decomposition decomposition = Decomposition(options.get_enum("decomposition"));
    if (decomposition == NONE) {
        Task task;
        task.set_goal(g_goal);
        tasks->push_back(task);
        return;
    } else if (decomposition == ALL_LANDMARKS) {
        get_fact_landmarks(&facts);
    } else if (decomposition == GOAL_FACTS) {
        facts = g_goal;
    } else {
        cerr << "Invalid decomposition: " << decomposition << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    order_facts(facts);
    for (int i = 0; i < facts.size(); i++) {
        // Filter facts that are true in initial state.
        if (!options.get<bool>("trivial_facts") && is_true_in_initial_state(facts[i]))
            continue;
        vector<Fact> goal;
        goal.push_back(facts[i]);
        Task task;
        task.set_goal(goal);
        tasks->push_back(task);
    }
}

void CegarHeuristic::initialize() {
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

    vector<PickStrategy> best_pick_strategies;
    best_pick_strategies.push_back(MAX_REFINED);
    best_pick_strategies.push_back(MAX_PREDECESSORS);
    best_pick_strategies.push_back(GOAL);

    generate_tasks(&tasks);

    for (int i = 0; i < tasks.size(); ++i) {
        cout << endl;
        Task &task = tasks[i];
        task.install();
        if (options.get<bool>("relevance_analysis"))
            task.remove_irrelevant_operators();
        task.adapt_operator_costs(remaining_costs);
        task.dump();

        Abstraction *abstraction = new Abstraction(&task);

        int rem_tasks = tasks.size() - i;
        abstraction->set_max_states_offline((max_states_offline - num_states_offline) / rem_tasks);
        abstraction->set_max_time((max_time - g_timer()) / rem_tasks);
        abstraction->set_log_h(options.get<bool>("log_h"));
        abstraction->set_write_dot_files(options.get<bool>("write_dot_files"));
        abstraction->set_use_astar(options.get<bool>("use_astar"));

        double factor = options.get<double>("init_h_factor");
        if (factor != -1) {
            abstraction->set_max_init_h_factor(factor);
        }

        PickStrategy pick_strategy = PickStrategy(options.get_enum("pick"));
        if (pick_strategy == BEST2) {
            pick_strategy = best_pick_strategies[i % 2];
        }
        abstraction->set_pick_strategy(pick_strategy);

        abstraction->build();
        avg_h_values.push_back(abstraction->get_avg_h());
        vector<int> needed_costs;
        abstraction->get_needed_costs(&needed_costs);
        task.adapt_remaining_costs(remaining_costs, needed_costs);
        abstraction->release_memory();

        abstractions.push_back(abstraction);
        num_states_offline += abstraction->get_num_states();

        task.release_memory();

        if (num_states_offline >= max_states_offline || g_timer() > max_time) {
            break;
        }
    }
    cout << endl;
    original_task.install();
    print_statistics();

    if (!search)
        exit_with(EXIT_UNSOLVED_INCOMPLETE);
}

void CegarHeuristic::print_statistics() {
    double sum_avg_h = 0;
    for (int i = 0; i < avg_h_values.size(); ++i) {
        sum_avg_h += avg_h_values[i];
    }
    cout << "Done building abstractions [t=" << g_timer << "]" << endl;
    cout << "CEGAR abstractions: " << abstractions.size() << endl;
    cout << "Abstract states offline: " << num_states_offline << endl;
    // There will always be at least one abstraction.
    cout << "Init h: " << compute_heuristic(*g_initial_state) << endl;
    cout << "Average h: " << sum_avg_h / abstractions.size() << endl;
}

int CegarHeuristic::compute_heuristic(const State &state) {
    assert(abstractions.size() <= tasks.size());
    int sum_h = 0;
    for (int i = 0; i < abstractions.size(); ++i) {
        Task &task = tasks[i];

        // If any fact in state is not reachable in this task, h(state) = 0.
        if (!task.state_is_reachable(state))
            continue;

        State projected_state(state);
        task.project_state(projected_state);

        int h = abstractions[i]->get_h(projected_state);
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
    vector<string> fact_order_strategies;
    fact_order_strategies.push_back("ORIGINAL");
    fact_order_strategies.push_back("MIXED");
    fact_order_strategies.push_back("CG_FORWARD");
    fact_order_strategies.push_back("CG_BACKWARD");
    fact_order_strategies.push_back("DOMAIN_SIZE_UP");
    fact_order_strategies.push_back("DOMAIN_SIZE_DOWN");
    fact_order_strategies.push_back("HADD_UP");
    fact_order_strategies.push_back("HADD_DOWN");
    parser.add_enum_option("fact_order", fact_order_strategies, "MIXED",
                           "order in which the goals are refined for");
    vector<string> decompositions;
    decompositions.push_back("NONE");
    decompositions.push_back("ALL_LANDMARKS");
    decompositions.push_back("RANDOM_LANDMARKS");
    decompositions.push_back("GOAL_FACTS");
    parser.add_enum_option("decomposition", decompositions, "GOAL_FACTS",
                           "build abstractions for each of these facts");
    parser.add_option<bool>("adapt_task", true, "remove redundant operators and facts");
    parser.add_option<bool>("relevance_analysis", false, "remove irrelevant operators");
    parser.add_option<bool>("trivial_facts", false, "include landmarks that are true in the initial state");
    parser.add_option<bool>("use_astar", true, "use A* for finding the *single* next solution");
    parser.add_option<bool>("search", true, "if set to false, abort after refining");
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
