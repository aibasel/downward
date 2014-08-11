#include "cegar_heuristic.h"

#include "abstraction.h"
#include "abstract_state.h"
#include "utils.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../landmarks/h_m_landmarks.h"
#include "../landmarks/landmark_graph.h"

#include <ext/hash_map>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <tr1/unordered_set>
#include <vector>

using namespace std;
using namespace std::tr1;

namespace cegar_heuristic {
CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      search(opts.get<bool>("search")),
      max_states(options.get<int>("max_states")),
      max_time(options.get<int>("max_time")),
      fact_order(GoalOrder(options.get_enum("fact_order"))),
      original_task(Task::get_original_task()),
      num_states(0),
      landmark_graph(get_landmark_graph()),
      temp_state_buffer(new int[g_variable_domain.size()]) {
    DEBUG = opts.get<bool>("debug");
    assert(max_time >= 0);

    verify_no_axioms_no_conditional_effects();

    if (DEBUG)
        landmark_graph.dump();
    if (options.get<bool>("dump_graphs")) {
        write_landmark_graph(landmark_graph);
        write_causal_graph();
    }

    for (int i = 0; i < g_operators.size(); ++i)
        remaining_costs.push_back(g_operators[i].get_cost());
}

CegarHeuristic::~CegarHeuristic() {
    delete[] temp_state_buffer;
    temp_state_buffer = 0;
    for (int i = 0; i < abstractions.size(); ++i)
        delete abstractions[i];
}

struct SortHaddValuesUp {
    const Task &task;
    explicit SortHaddValuesUp(const Task &task_) : task(task_) {}
    bool operator()(Fact a, Fact b) {
        return task.get_hadd_value(a.first, a.second) < task.get_hadd_value(b.first, b.second);
    }
};

void CegarHeuristic::get_fact_landmarks(vector<Fact> *facts) const {
    const set<LandmarkNode *> &nodes = landmark_graph.get_nodes();
    for (set<LandmarkNode *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        const LandmarkNode *node_p = *it;
        facts->push_back(get_fact(node_p));
    }
}

LandmarkGraph CegarHeuristic::get_landmark_graph() const {
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
    opts.set<bool>("supports_conditional_effects", false);
    opts.set<Exploration *>("explor", new Exploration(opts));
    HMLandmarks lm_graph_factory(opts);
    return *lm_graph_factory.compute_lm_graph();
}

/*
  Do a breadth-first search through the landmark graph ignoring
  duplicates. Start at the node for the given fact and collect for each
  variable the facts that have to be made true before the fact is made
  true for the first time. */
void CegarHeuristic::get_prev_landmarks(Fact fact, unordered_map<int, unordered_set<int> > *groups) const {
    assert(groups->empty());
    LandmarkNode *node = landmark_graph.get_landmark(fact);
    assert(node);
    vector<const LandmarkNode *> open;
    unordered_set<const LandmarkNode *> closed;
    for (__gnu_cxx::hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator it =
             node->parents.begin(); it != node->parents.end(); ++it) {
        const LandmarkNode *parent = it->first;
        open.push_back(parent);
    }
    while (!open.empty()) {
        const LandmarkNode *ancestor = open.back();
        open.pop_back();
        if (closed.count(ancestor) == 1)
            continue;
        closed.insert(ancestor);
        Fact ancestor_fact = get_fact(ancestor);
        (*groups)[ancestor_fact.first].insert(ancestor_fact.second);
        for (__gnu_cxx::hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator it =
                 ancestor->parents.begin(); it != ancestor->parents.end(); ++it) {
            const LandmarkNode *parent = it->first;
            open.push_back(parent);
        }
    }
}

void CegarHeuristic::order_facts(vector<Fact> &facts) const {
    cout << "Sort " << facts.size() << " facts" << endl;
    if (fact_order == ORIGINAL) {
        // Nothing to do.
    } else if (fact_order == MIXED) {
        random_shuffle(facts.begin(), facts.end());
    } else if (fact_order == HADD_UP || fact_order == HADD_DOWN) {
        sort(facts.begin(), facts.end(), SortHaddValuesUp(original_task));
        if (fact_order == HADD_DOWN)
            reverse(facts.begin(), facts.end());
    } else {
        cerr << "Not a valid fact ordering strategy: " << fact_order << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
}

bool is_true_in_initial_state(Fact fact) {
    return g_initial_state()[fact.first] == fact.second;
}

void CegarHeuristic::get_facts(vector<Fact> &facts, Decomposition decomposition) const {
    assert(decomposition != NONE);
    if (decomposition == LANDMARKS) {
        get_fact_landmarks(&facts);
    } else if (decomposition == GOALS) {
        facts = original_task.get_goal();
    } else {
        cerr << "Invalid decomposition: " << decomposition << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    // Filter facts that are true in initial state.
    facts.erase(remove_if(facts.begin(), facts.end(), is_true_in_initial_state), facts.end());
    order_facts(facts);
}

void CegarHeuristic::install_task(Task &task) const {
    task.adapt_operator_costs(remaining_costs);
    task.dump();
    task.install();
}

void CegarHeuristic::build_abstractions(Decomposition decomposition) {
    vector<Fact> facts;
    int num_abstractions = 1;
    int max_abstractions = options.get<int>("max_abstractions");
    if (decomposition == NONE) {
        if (max_abstractions != INF)
            num_abstractions = max_abstractions;
    } else {
        get_facts(facts, decomposition);
        num_abstractions = min(static_cast<int>(facts.size()), max_abstractions);
    }

    if (!facts.empty()) {
        cout << "h^add values: ";
        for (int i = 0; i < facts.size(); ++i) {
            cout << to_string(facts[i]) << ":" << original_task.get_hadd_value(facts[i].first, facts[i].second) << " ";
        }
    }

    for (int i = 0; i < num_abstractions; ++i) {
        cout << endl;
        Task task = original_task;
        if (decomposition != NONE) {
            bool combine_facts = (options.get<bool>("combine_facts") && decomposition == LANDMARKS);
            task.set_goal(facts[i], options.get<bool>("adapt_task") || combine_facts);
            if (combine_facts) {
                unordered_map<int, unordered_set<int> > groups;
                get_prev_landmarks(facts[i], &groups);
                for (unordered_map<int, unordered_set<int> >::iterator it =
                         groups.begin(); it != groups.end(); ++it) {
                    if (it->second.size() >= 2)
                        task.combine_facts(it->first, it->second);
                }
            }
        }
        install_task(task);
        if (decomposition != NONE) {
            int fact_index = task.get_projected_index(facts[i].first, facts[i].second);
            int goal_fact_hadd = task.get_hadd_value(facts[i].first, fact_index);
            cout << "h^add(s*): " << goal_fact_hadd << endl;
            assert(options.get<bool>("negative_costs") || goal_fact_hadd >= 0);
            assert(goal_fact_hadd != -1);
            if (goal_fact_hadd < options.get<int>("min_hadd"))
                continue;
        }

        Abstraction *abstraction = new Abstraction(&task);

        int rem_tasks = num_abstractions - i;
        abstraction->set_max_states((max_states - num_states) / rem_tasks);
        abstraction->set_max_time(ceil((max_time - g_timer()) / rem_tasks));
        abstraction->set_dump_graphs(options.get<bool>("dump_graphs"));
        abstraction->set_use_astar(options.get<bool>("use_astar"));
        abstraction->set_use_negative_costs(options.get<bool>("negative_costs"));

        abstraction->set_pick_strategy(PickStrategy(options.get_enum("pick")));

        abstraction->build();
        avg_h_values.push_back(abstraction->get_avg_h());
        num_states += abstraction->get_num_states();
        if (decomposition == NONE && num_abstractions == 1 && !search)
            abstraction->print_histograms();
        vector<int> needed_costs;
        abstraction->get_needed_costs(&needed_costs);
        task.adapt_remaining_costs(remaining_costs, needed_costs);
        int init_h = abstraction->get_init_h();
        abstraction->release_memory();
        task.release_memory();

        if (init_h > 0) {
            tasks.push_back(task);
            abstractions.push_back(abstraction);
        }
        if (init_h == INF) {
            cout << "Abstraction is unsolvable" << endl;
            break;
        }

        if (num_states >= max_states || g_timer() > max_time)
            break;
    }
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    cout << "Peak memory before initialization: "
         << get_peak_memory_in_kb() << " KB" << endl;
    if (DEBUG) {
        cout << "Original task:" << endl;
        original_task.dump();
    }
    Decomposition decomposition(Decomposition(options.get_enum("decomposition")));
    vector<Decomposition> decompositions;
    if (decomposition == LANDMARKS_AND_GOALS_AND_NONE) {
        decompositions.push_back(LANDMARKS);
        decompositions.push_back(GOALS);
        decompositions.push_back(NONE);
    } else if (decomposition == LANDMARKS_AND_GOALS) {
        decompositions.push_back(LANDMARKS);
        decompositions.push_back(GOALS);
    } else {
        decompositions.push_back(decomposition);
    }
    for (int i = 0; i < decompositions.size(); ++i) {
        cout << endl << "Using decomposition " << decompositions[i] << endl;
        build_abstractions(decompositions[i]);
        cout << endl;
        original_task.install();
        if (num_states >= max_states || g_timer() > max_time ||
            compute_heuristic(g_initial_state()) == DEAD_END)
            break;
    }
    cout << endl;
    print_statistics();

    if (!search) {
        cout << "search=false --> stop" << endl;
        exit_with(EXIT_UNSOLVED_INCOMPLETE);
    }
}

void CegarHeuristic::print_statistics() {

    cout << "Done building abstractions [t=" << g_timer << "]" << endl;
    cout << "Peak memory after initialization: "
         << get_peak_memory_in_kb() << " KB" << endl;
    cout << "CEGAR abstractions: " << abstractions.size() << "/" << avg_h_values.size() << endl;
    cout << "Total abstract states: " << num_states << endl;
    cout << "Init h: " << compute_heuristic(g_initial_state()) << endl;
    if (!avg_h_values.empty()) {
        double sum_avg_h = 0;
        for (int i = 0; i < avg_h_values.size(); ++i)
            sum_avg_h += avg_h_values[i];
        cout << "Average h: " << sum_avg_h / avg_h_values.size() << endl;
    }
    cout << endl;
}

int CegarHeuristic::compute_heuristic(const State &state) {
    assert(abstractions.size() <= tasks.size());
    int sum_h = 0;
    for (int i = 0; i < abstractions.size(); ++i) {
        Task &task = tasks[i];

        const int *buffer = 0;
        // TODO: Use the state's buffer directly.
        if (false && (Decomposition(options.get_enum("decomposition")) == NONE ||
                      (Decomposition(options.get_enum("decomposition")) == GOALS &&
                       !options.get<bool>("adapt_task")))) {
            ABORT("Not implemented");
        } else {
            // If any fact in state is not reachable in this task, h(state) = 0.
            bool reachable = task.translate_state(state, temp_state_buffer);
            if (!reachable)
                continue;
            buffer = temp_state_buffer;
        }

        int h = abstractions[i]->get_h(buffer);
        assert(h >= 0);
        if (h == INF)
            return DEAD_END;
        sum_h += h;
    }
    assert(sum_h >= 0);
    return sum_h;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.add_option<int>("max_states", "maximum number of abstract states", "infinity");
    parser.add_option<int>("max_time", "maximum time in seconds for building the abstraction", "900");
    vector<string> pick_strategies;
    pick_strategies.push_back("RANDOM");
    pick_strategies.push_back("MIN_CONSTRAINED");
    pick_strategies.push_back("MAX_CONSTRAINED");
    pick_strategies.push_back("MIN_REFINED");
    pick_strategies.push_back("MAX_REFINED");
    pick_strategies.push_back("MIN_HADD");
    pick_strategies.push_back("MAX_HADD");
    parser.add_enum_option("pick",
                           pick_strategies,
                           "how to pick the next unsatisfied condition",
                           "MAX_REFINED");
    vector<string> fact_order_strategies;
    fact_order_strategies.push_back("ORIGINAL");
    fact_order_strategies.push_back("MIXED");
    fact_order_strategies.push_back("HADD_UP");
    fact_order_strategies.push_back("HADD_DOWN");
    parser.add_enum_option("fact_order",
                           fact_order_strategies,
                           "order in which the goals are refined for",
                           "HADD_DOWN");
    vector<string> decompositions;
    decompositions.push_back("NONE");
    decompositions.push_back("LANDMARKS");
    decompositions.push_back("GOALS");
    decompositions.push_back("LANDMARKS_AND_GOALS");
    decompositions.push_back("LANDMARKS_AND_GOALS_AND_NONE");
    parser.add_enum_option("decomposition",
                           decompositions,
                           "build abstractions for each of these facts",
                           "LANDMARKS_AND_GOALS");
    parser.add_option<int>("max_abstractions", "max number of abstractions to build", "infinity");
    parser.add_option<bool>("adapt_task", "remove redundant operators and facts", "true");
    parser.add_option<bool>("combine_facts", "combine landmark facts", "true");
    parser.add_option<bool>("use_astar", "use A* for finding the *single* next solution", "true");
    parser.add_option<int>("min_hadd", "ignore facts with too low h^add values", "1");
    parser.add_option<bool>("negative_costs", "allow negative costs in cost-partitioning", "false");
    parser.add_option<bool>("search", "if set to false, abort after refining", "true");
    parser.add_option<bool>("debug", "print debugging output", "false");
    parser.add_option<bool>("dump_graphs", "write causal and landmark graphs", "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cegar", _parse);
}
