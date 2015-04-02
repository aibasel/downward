#include "cegar_heuristic.h"

#include "abstraction.h"
#include "abstract_state.h"
#include "modified_costs_task.h"
#include "utils.h"
#include "values.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state_registry.h"
#include "../task_tools.h"

#include "../landmarks/h_m_landmarks.h"
#include "../landmarks/landmark_graph.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace cegar {

shared_ptr<AdditiveHeuristic> get_additive_heuristic(TaskProxy task) {
    cout << "Start computing h^add values [t=" << g_timer << "] for ";
    Options opts;
    opts.set<TaskProxy *>("task_proxy", &task);
    opts.set<int>("cost_type", 0);
    shared_ptr<AdditiveHeuristic> additive_heuristic = make_shared<AdditiveHeuristic>(opts);
    additive_heuristic->initialize_and_compute_heuristic(task.get_initial_state());
    cout << "Done computing h^add values [t=" << g_timer << "]" << endl;
    return additive_heuristic;
}

CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      search(opts.get<bool>("search")),
      max_states(options.get<int>("max_states")),
      max_time(options.get<int>("max_time")),
      fact_order(GoalOrder(options.get_enum("fact_order"))),
      num_states(0),
      landmark_graph(get_landmark_graph()),
      temp_state_buffer(new int[task->get_variables().size()]) {
    DEBUG = opts.get<bool>("debug");
    assert(max_time >= 0);

    verify_no_axioms_no_conditional_effects();

    if (DEBUG)
        landmark_graph.dump();
    if (options.get<bool>("dump_graphs")) {
        write_landmark_graph(landmark_graph);
        write_causal_graph();
    }

    for (OperatorProxy op : task->get_operators())
        remaining_costs.push_back(op.get_cost());
}

CegarHeuristic::~CegarHeuristic() {
    delete[] temp_state_buffer;
    temp_state_buffer = 0;
    for (size_t i = 0; i < abstractions.size(); ++i)
        delete abstractions[i];
}

struct SortHaddValuesUp {
    const shared_ptr<AdditiveHeuristic> hadd;

    explicit SortHaddValuesUp(TaskProxy task)
        : hadd(get_additive_heuristic(task)) {}

    int get_cost(FactProxy fact) {
        return hadd->get_cost(fact.get_variable().get_id(), fact.get_value());
    }

    bool operator()(FactProxy a, FactProxy b) {
        return get_cost(a) < get_cost(b);
    }
};

vector<FactProxy> CegarHeuristic::get_fact_landmarks() const {
    vector<FactProxy> facts;
    const set<LandmarkNode *> &nodes = landmark_graph.get_nodes();
    for (LandmarkNode *node : nodes) {
        Fact raw_fact = get_fact(node);
        FactProxy fact = task->get_variables()[raw_fact.first].get_fact(raw_fact.second);
        facts.push_back(fact);
    }
    sort(facts.begin(), facts.end());
    return facts;
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
VariableToValues CegarHeuristic::get_prev_landmarks(FactProxy fact) const {
    unordered_map<int, unordered_set<int> > groups;
    LandmarkNode *node = landmark_graph.get_landmark(get_raw_fact(fact));
    assert(node);
    vector<const LandmarkNode *> open;
    unordered_set<const LandmarkNode *> closed;
    for (const auto &parent_and_edge: node->parents) {
        const LandmarkNode *parent = parent_and_edge.first;
        open.push_back(parent);
    }
    while (!open.empty()) {
        const LandmarkNode *ancestor = open.back();
        open.pop_back();
        if (closed.count(ancestor))
            continue;
        closed.insert(ancestor);
        Fact ancestor_fact = get_fact(ancestor);
        groups[ancestor_fact.first].insert(ancestor_fact.second);
        for (auto it = ancestor->parents.begin(); it != ancestor->parents.end(); ++it) {
            const LandmarkNode *parent = it->first;
            open.push_back(parent);
        }
    }
    return groups;
}

void CegarHeuristic::order_facts(vector<FactProxy> &facts) const {
    cout << "Sort " << facts.size() << " facts" << endl;
    if (fact_order == ORIGINAL) {
        // Nothing to do.
    } else if (fact_order == MIXED) {
        random_shuffle(facts.begin(), facts.end());
    } else if (fact_order == HADD_UP || fact_order == HADD_DOWN) {
        sort(facts.begin(), facts.end(), SortHaddValuesUp(*task));
        if (fact_order == HADD_DOWN)
            reverse(facts.begin(), facts.end());
    } else {
        cerr << "Not a valid fact ordering strategy: " << fact_order << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
}

vector<FactProxy> CegarHeuristic::get_facts(Decomposition decomposition) const {
    assert(decomposition != Decomposition::NONE);
    vector<FactProxy> facts;
    if (decomposition == Decomposition::LANDMARKS) {
        facts = get_fact_landmarks();
    } else if (decomposition == Decomposition::GOALS) {
        for (FactProxy goal : task->get_goals()) {
            facts.push_back(goal);
        }
    } else {
        cerr << "Invalid decomposition: " << static_cast<int>(decomposition) << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    // Filter facts that are true in initial state.
    facts.erase(remove_if(
            facts.begin(),
            facts.end(),
            [&](FactProxy fact){
                return task->get_initial_state()[fact.get_variable()] == fact;}),
        facts.end());
    order_facts(facts);
    return facts;
}

void adapt_remaining_costs(vector<int> &remaining_costs, const vector<int> &needed_costs) {
    assert(remaining_costs.size() == needed_costs.size());
    if (DEBUG)
        cout << "Remaining: " << to_string(remaining_costs) << endl;
    if (DEBUG)
        cout << "Needed:    " << to_string(needed_costs) << endl;
    for (size_t op_index = 0; op_index < remaining_costs.size(); ++op_index) {
        assert(in_bounds(op_index, remaining_costs));
        assert(remaining_costs[op_index] >= 0);
        assert(needed_costs[op_index] <= remaining_costs[op_index]);
        remaining_costs[op_index] -= needed_costs[op_index];
        assert(remaining_costs[op_index] >= 0);
    }
    if (DEBUG)
        cout << "Remaining: " << to_string(remaining_costs) << endl;
}

void CegarHeuristic::build_abstractions(Decomposition decomposition) {
    vector<FactProxy> facts;
    int num_abstractions = 1;
    int max_abstractions = options.get<int>("max_abstractions");
    if (decomposition == Decomposition::NONE) {
        if (max_abstractions != INF)
            num_abstractions = max_abstractions;
    } else {
        facts = get_facts(decomposition);
        num_abstractions = min(static_cast<int>(facts.size()), max_abstractions);
    }

    for (int i = 0; i < num_abstractions; ++i) {
        cout << endl;
        shared_ptr<AbstractTask> orig_task_impl = g_root_task();

        Options opts;
        opts.set<shared_ptr<AbstractTask> >("transform", orig_task_impl);
        opts.set<vector<int> >("operator_costs", remaining_costs);
        shared_ptr<ModifiedCostsTask> modified_costs_task = make_shared<ModifiedCostsTask>(opts);

        shared_ptr<AbstractTask> abstracted_task;
        unordered_set<FactProxy> reachable_facts;
        if (decomposition == Decomposition::NONE) {
            abstracted_task = modified_costs_task;
        } else {
            FactProxy landmark = facts[i];
            VariableToValues groups;
            if (options.get<bool>("combine_facts") && decomposition == Decomposition::LANDMARKS) {
                groups = get_prev_landmarks(landmark);
            }
            abstracted_task = make_shared<LandmarkTask>(modified_costs_task, landmark, groups);
            if (decomposition == Decomposition::LANDMARKS)
                reachable_facts = compute_reachable_facts(*task, landmark);
        }

        TaskProxy abstracted_task_proxy = TaskProxy(abstracted_task.get());
        dump_task(abstracted_task_proxy);

        shared_ptr<AdditiveHeuristic> additive_heuristic = get_additive_heuristic(abstracted_task_proxy);

        // TODO: Fix this hack.
        Values::initialize_static_members(abstracted_task_proxy);

        Abstraction *abstraction = new Abstraction(abstracted_task_proxy, additive_heuristic);

        int rem_tasks = num_abstractions - i;
        abstraction->set_max_states((max_states - num_states) / rem_tasks);
        abstraction->set_max_time(ceil((max_time - g_timer()) / rem_tasks));
        abstraction->set_dump_graphs(options.get<bool>("dump_graphs"));
        abstraction->set_use_astar(options.get<bool>("use_astar"));
        abstraction->set_use_general_costs(options.get<bool>("general_costs"));

        abstraction->set_pick_strategy(PickStrategy(options.get_enum("pick")));

        if (decomposition == Decomposition::LANDMARKS)
            abstraction->separate_unreachable_facts(compute_reachable_facts(
                abstracted_task_proxy,
                abstracted_task_proxy.get_goals()[0]));

        abstraction->build();
        num_states += abstraction->get_num_states();
        vector<int> needed_costs = abstraction->get_needed_costs();
        adapt_remaining_costs(remaining_costs, needed_costs);
        int init_h = abstraction->get_init_h();
        abstraction->release_memory();

        if (init_h > 0) {
            tasks.push_back(abstracted_task);
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
    Decomposition decomposition(Decomposition(options.get_enum("decomposition")));
    vector<Decomposition> decompositions;
    if (decomposition == Decomposition::LANDMARKS_AND_GOALS_AND_NONE) {
        decompositions.push_back(Decomposition::LANDMARKS);
        decompositions.push_back(Decomposition::GOALS);
        decompositions.push_back(Decomposition::NONE);
    } else if (decomposition == Decomposition::LANDMARKS_AND_GOALS) {
        decompositions.push_back(Decomposition::LANDMARKS);
        decompositions.push_back(Decomposition::GOALS);
    } else {
        decompositions.push_back(decomposition);
    }
    for (size_t i = 0; i < decompositions.size(); ++i) {
        cout << endl << "Using decomposition " << static_cast<int>(decompositions[i]) << endl;
        build_abstractions(decompositions[i]);
        cout << endl;
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
    cout << "CEGAR abstractions: " << abstractions.size() << endl;
    cout << "Total abstract states: " << num_states << endl;
    cout << "Init h: " << compute_heuristic(g_initial_state()) << endl;
    cout << endl;
}

int CegarHeuristic::compute_heuristic(const GlobalState &global_state) {
    assert(abstractions.size() == tasks.size());
    int sum_h = 0;
    for (size_t i = 0; i < abstractions.size(); ++i) {
        shared_ptr<AbstractTask> task = tasks[i];
        State state = TaskProxy(task.get()).convert_global_state(global_state);

        int h = abstractions[i]->get_h(state);
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
    parser.add_option<bool>("combine_facts", "combine landmark facts", "true");
    parser.add_option<bool>("use_astar", "use A* for finding the *single* next solution", "true");
    parser.add_option<bool>("general_costs", "allow negative costs in cost-partitioning", "true");
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
