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
      temp_state_buffer(new state_var_t[g_variable_domain.size()]) {
    DEBUG = opts.get<bool>("debug");
    assert(max_time >= 0);

    verify_no_axioms_no_cond_effects();

    if (DEBUG)
        landmark_graph.dump();
    if (options.get<bool>("write_dot_files"))
        write_landmark_graph(landmark_graph);

    original_task.dump();

    for (int i = 0; i < g_operators.size(); ++i)
        remaining_costs.push_back(g_operators[i].get_cost());
}

CegarHeuristic::~CegarHeuristic() {
    delete temp_state_buffer;
    temp_state_buffer = 0;
    // TODO: Delete abstractions.
}

struct SortHaddValuesUp {
    const Task task;
    explicit SortHaddValuesUp(const Task &t) : task(t) {}
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
    opts.set<Exploration *>("explor", new Exploration(opts));
    HMLandmarks lm_graph_factory(opts);
    return *lm_graph_factory.compute_lm_graph();
}

void CegarHeuristic::get_prev_landmarks(Fact fact, unordered_map<int, unordered_set<int> > *groups) const {
    assert(groups->empty());
    LandmarkNode *node = landmark_graph.get_landmark(fact);
    assert(node);
    vector<const LandmarkNode *> open;
    for (__gnu_cxx::hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator it =
             node->parents.begin(); it != node->parents.end(); ++it) {
        const LandmarkNode *parent = it->first;
        open.push_back(parent);
    }
    while (!open.empty()) {
        const LandmarkNode *ancestor = open.back();
        open.pop_back();
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

struct is_not_leaf_landmark {
    const LandmarkGraph graph;
    explicit is_not_leaf_landmark(const LandmarkGraph &g) : graph(g) {}
    bool operator()(Fact fact) {
        LandmarkNode *node = graph.get_landmark(fact);
        return !node->children.empty();
    }
};

void CegarHeuristic::get_facts(vector<Fact> &facts, Decomposition decomposition) const {
    assert(decomposition != NONE);
    if (decomposition == LANDMARKS) {
        get_fact_landmarks(&facts);
    } else if (decomposition == GOALS) {
        facts = original_task.get_goal();
    } else if (decomposition == GOAL_LEAVES) {
        facts = original_task.get_goal();
        facts.erase(remove_if(facts.begin(), facts.end(),
                              is_not_leaf_landmark(landmark_graph)), facts.end());
    } else {
        cerr << "Invalid decomposition: " << decomposition << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    // Filter facts that are true in initial state.
    if (!options.get<bool>("trivial_facts")) {
        facts.erase(remove_if(facts.begin(), facts.end(), is_true_in_initial_state), facts.end());
    }
    order_facts(facts);
}

void CegarHeuristic::install_task(Task &task) const {
    if (options.get<bool>("relevance_analysis"))
        task.remove_irrelevant_operators();
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

    for (int i = 0; i < num_abstractions; ++i) {
        cout << endl;
        Task task = original_task;
        task.reset_pointers();
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
        tasks.push_back(task);
        install_task(task);

        Abstraction *abstraction = new Abstraction(&task);

        int rem_tasks = num_abstractions - i;
        abstraction->set_max_states((max_states - num_states) / rem_tasks);
        abstraction->set_max_time(ceil((max_time - g_timer()) / rem_tasks));
        abstraction->set_write_dot_files(options.get<bool>("write_dot_files"));
        abstraction->set_use_astar(options.get<bool>("use_astar"));

        abstraction->set_pick_strategy(PickStrategy(options.get_enum("pick")));

        abstraction->build();
        avg_h_values.push_back(abstraction->get_avg_h());
        vector<int> needed_costs;
        abstraction->get_needed_costs(&needed_costs);
        task.adapt_remaining_costs(remaining_costs, needed_costs);
        abstraction->release_memory();

        abstractions.push_back(abstraction);
        num_states += abstraction->get_num_states();

        task.release_memory();

        if (num_states >= max_states || g_timer() > max_time)
            break;
    }
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    cout << "Peak memory before initialization: "
         << get_peak_memory_in_kb() << " KB" << endl;
    Decomposition decomposition(Decomposition(options.get_enum("decomposition")));
    vector<Decomposition> decompositions;
    if (decomposition == LANDMARKS_AND_GOALS) {
        decompositions.push_back(LANDMARKS);
        decompositions.push_back(GOALS);
    } else {
        decompositions.push_back(decomposition);
    }
    for (int i = 0; i < decompositions.size(); ++i) {
        cout << endl << "Using decomposition " << decompositions[i] << endl;
        build_abstractions(decompositions[i]);
        original_task.install();
        if (num_states >= max_states || g_timer() > max_time)
            break;
    }
    cout << endl;
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
    cout << "Peak memory after initialization: "
         << get_peak_memory_in_kb() << " KB" << endl;
    cout << "CEGAR abstractions: " << abstractions.size() << endl;
    cout << "Abstract states: " << num_states << endl;
    // There will always be at least one abstraction.
    cout << "Init h: " << compute_heuristic(g_initial_state()) << endl;
    cout << "Average h: " << sum_avg_h / abstractions.size() << endl;
    cout << endl;
}

int CegarHeuristic::compute_heuristic(const State &state) {
    assert(abstractions.size() <= tasks.size());
    int sum_h = 0;
    for (int i = 0; i < abstractions.size(); ++i) {
        Task &task = tasks[i];

        // If any fact in state is not reachable in this task, h(state) = 0.
        bool reachable = task.translate_state(state, temp_state_buffer);
        if (!reachable)
            continue;

        int h = abstractions[i]->get_h(temp_state_buffer);
        assert(h >= 0);
        if (h == INF)
            return DEAD_END;
        sum_h += h;
    }
    assert(sum_h >= 0 && "check against overflow");
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
    pick_strategies.push_back("MIN_HADD_DYN");
    pick_strategies.push_back("MAX_HADD_DYN");
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
    decompositions.push_back("GOAL_LEAVES");
    decompositions.push_back("LANDMARKS_AND_GOALS");
    parser.add_enum_option("decomposition",
                           decompositions,
                           "build abstractions for each of these facts",
                           "LANDMARKS_AND_GOALS");
    parser.add_option<int>("max_abstractions", "max number of abstractions to build", "infinity");
    parser.add_option<bool>("adapt_task", "remove redundant operators and facts", "true");
    parser.add_option<bool>("combine_facts", "combine landmark facts", "true");
    parser.add_option<bool>("relevance_analysis", "remove irrelevant operators", "false");
    parser.add_option<bool>("trivial_facts", "include landmarks that are true in the initial state", "false");
    parser.add_option<bool>("use_astar", "use A* for finding the *single* next solution", "true");
    parser.add_option<bool>("search", "if set to false, abort after refining", "true");
    parser.add_option<bool>("debug", "print debugging output", "false");
    parser.add_option<bool>("write_dot_files", "write graph files for debugging", "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cegar", _parse);
}
