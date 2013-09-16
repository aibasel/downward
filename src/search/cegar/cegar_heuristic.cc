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
CegarSumHeuristic::CegarSumHeuristic(const Options &opts)
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

CegarSumHeuristic::~CegarSumHeuristic() {
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

bool operator_relaxed_applicable(const Operator &op, const unordered_set<int> &reached) {
    for (int i = 0; i < op.get_prevail().size(); i++) {
        const Prevail &prevail = op.get_prevail()[i];
        if (reached.count(get_fact_number(prevail.var, prevail.prev)) == 0)
            return false;
    }
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.pre != UNDEFINED && reached.count(get_fact_number(pre_post.var, pre_post.pre)) == 0)
            return false;
    }
    return true;
}

void get_fact_numbers(const State &state, const Task &task, unordered_set<int> *fact_numbers) {
    for (int var = 0; var < task.variable_domain.size(); ++var)
        fact_numbers->insert(get_fact_number(var, state[var]));
}

void CegarSumHeuristic::get_possibly_before_facts(const Fact last_fact, unordered_set<int> *reached) const {
    // Add facts from initial state.
    get_fact_numbers(*g_initial_state, original_task, reached);

    // Until no more facts can be added:
    int last_num_reached = 0;
    while (last_num_reached != reached->size()) {
        last_num_reached = reached->size();
        for (int i = 0; i < g_operators.size(); ++i) {
            Operator &op = g_operators[i];
            // Ignore operators that achieve last_fact.
            if (get_eff(op, last_fact.first) == last_fact.second)
                continue;
            // Add all facts that are achieved by an applicable operator.
            if (operator_relaxed_applicable(op, *reached)) {
                for (int i = 0; i < op.get_pre_post().size(); i++) {
                    const PrePost &pre_post = op.get_pre_post()[i];
                    reached->insert(get_fact_number(pre_post.var, pre_post.post));
                }
            }
        }
    }
}

void CegarSumHeuristic::get_fact_landmarks(vector<Fact> *facts) const {
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

void CegarSumHeuristic::order_facts(vector<Fact> &facts) const {
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

void CegarSumHeuristic::generate_tasks(vector<Task> *tasks) const {
    vector<Fact> facts;
    Decomposition decomposition = Decomposition(options.get_enum("decomposition"));
    if (decomposition == NONE) {
        Task task;
        task.goal = g_goal;
        task.variable_domain = g_variable_domain;
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
        Task task;
        task.goal.push_back(facts[i]);
        task.variable_domain = g_variable_domain;
        tasks->push_back(task);
    }
}

void CegarSumHeuristic::adapt_remaining_costs(const Task &task, const vector<int> &needed_costs) {
    if (DEBUG)
        cout << "Needed:    " << to_string(needed_costs) << endl;
    assert(task.operators.size() == task.original_operator_numbers.size());
    for (int i = 0; i < task.operators.size(); ++i) {
        int op_number = task.original_operator_numbers[i];
        assert(op_number >= 0 && op_number < remaining_costs.size());
        remaining_costs[op_number] -= needed_costs[i];
        assert(remaining_costs[op_number] >= 0);
    }
    if (DEBUG)
        cout << "Remaining: " << to_string(remaining_costs) << endl;
}

void CegarSumHeuristic::add_operators(Task &task) {
    if (task.goal.size() > 1) {
        task.operators = g_operators;
        for (int i = 0; i < task.operators.size(); ++i)
            task.original_operator_numbers.push_back(i);
        task.fact_numbers = original_task.fact_numbers;
        return;
    }

    assert(task.goal.size() == 1);
    Fact &last_fact = task.goal[0];
    get_possibly_before_facts(last_fact, &task.fact_numbers);

    for (int i = 0; i < g_operators.size(); ++i) {
        // Only keep operators with all preconditions in reachable set of facts.
        if (!options.get<bool>("adapt_task") ||
            operator_relaxed_applicable(g_operators[i], task.fact_numbers)) {
            Operator op = g_operators[i];
            op.set_cost(remaining_costs[i]);
            // If op achieves last_fact set eff(op) = {last_fact}.
            if (get_eff(op, last_fact.first) == last_fact.second) {
                op.set_effect(last_fact.first, get_pre(op, last_fact.first), last_fact.second);
            }
            task.operators.push_back(op);
            task.original_operator_numbers.push_back(i);
        }
    }
    // TODO: task.facts = F + {last_fact}
    // TODO: Adapt variable_domain.
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

    vector<PickStrategy> best_pick_strategies;
    best_pick_strategies.push_back(MAX_REFINED);
    best_pick_strategies.push_back(MAX_PREDECESSORS);
    best_pick_strategies.push_back(GOAL);

    generate_tasks(&tasks);

    for (int i = 0; i < tasks.size(); ++i) {
        cout << endl;
        Task &task = tasks[i];
        task.install();

        for (int i = 0; i < task.goal.size(); ++i)
            cout << "Refine for " << g_fact_names[task.goal[i].first][task.goal[i].second]
                 << " (" << task.goal[i].first << "=" << task.goal[i].second << ")" << endl;

        add_operators(task);
        int num_facts = task.fact_numbers.size();
        if (options.get<bool>("adapt_task"))
            ++num_facts;
        cout << "Facts: " << num_facts << "/" << g_num_facts << endl;
        cout << "Operators: " << task.operators.size() << "/" << g_operators.size() << endl;
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
        adapt_remaining_costs(task, needed_costs);
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

void CegarSumHeuristic::print_statistics() {
    double sum_avg_h = 0;
    for (int i = 0; i < avg_h_values.size(); ++i) {
        sum_avg_h += avg_h_values[i];
    }
    cout << "CEGAR abstractions: " << abstractions.size() << endl;
    cout << "Abstract states offline: " << num_states_offline << endl;
    // There will always be at least one abstraction.
    cout << "Init h: " << compute_heuristic(*g_initial_state) << endl;
    cout << "Average h: " << sum_avg_h / abstractions.size() << endl;
}

int CegarSumHeuristic::compute_heuristic(const State &state) {
    assert(abstractions.size() <= tasks.size());
    int sum_h = 0;
    for (int i = 0; i < abstractions.size(); ++i) {
        Task &task = tasks[i];

        // If any fact in state is not reachable in this task, h(state) = 0.
        unordered_set<int> state_fact_numbers;
        get_fact_numbers(state, task, &state_fact_numbers);
        if (!is_subset(state_fact_numbers, task.fact_numbers))
            continue;

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
        return new CegarSumHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);
}
