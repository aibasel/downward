#include "cegar_heuristic.h"

#include "abstraction.h"
#include "abstract_state.h"
#include "cartesian_heuristic.h"
#include "modified_costs_task.h"
#include "modified_goals_task.h"
#include "utils.h"
#include "utils_landmarks.h"
#include "values.h"

#include "../additive_heuristic.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

#include "../landmarks/landmark_graph.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace cegar {
CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      search(opts.get<bool>("search")),
      max_states(options.get<int>("max_states")),
      max_time(options.get<int>("max_time")),
      fact_order(GoalOrder(options.get_enum("fact_order"))),
      num_states(0),
      landmark_graph(get_landmark_graph()) {
    DEBUG = opts.get<bool>("debug");
    assert(max_time >= 0);

    verify_no_axioms_no_conditional_effects();

    if (DEBUG)
        landmark_graph->dump();
    if (options.get<bool>("write_graphs")) {
        write_landmark_graph(landmark_graph);
    }

    for (OperatorProxy op : task->get_operators())
        remaining_costs.push_back(op.get_cost());
}

CegarHeuristic::~CegarHeuristic() {
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
        vector<Fact> raw_facts = get_fact_landmarks(landmark_graph);
        for (Fact raw_fact : raw_facts)
            facts.push_back(get_fact(*task, raw_fact));
    } else if (decomposition == Decomposition::GOALS) {
        for (FactProxy goal : task->get_goals())
            facts.push_back(goal);
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

        Options opts;
        opts.set<shared_ptr<AbstractTask> >("transform", g_root_task());
        opts.set<vector<int> >("operator_costs", remaining_costs);

        shared_ptr<AbstractTask> abstracted_task = make_shared<ModifiedCostsTask>(opts);
        if (decomposition == Decomposition::GOALS || decomposition == Decomposition::LANDMARKS) {
            vector<Fact> goals = {get_raw_fact(facts[i])};
            abstracted_task = make_shared<ModifiedGoalsTask>(abstracted_task, goals);

            if (decomposition == Decomposition::LANDMARKS) {
                VarToGroups groups;
                if (options.get<bool>("combine_facts")) {
                    VarToValues landmark_groups = get_prev_landmarks(landmark_graph, get_raw_fact(facts[i]));
                    for (auto &pair : landmark_groups) {
                        int var = pair.first;
                        vector<int> &group = pair.second;
                        if (group.size() >= 2)
                            groups[var].push_back(group);
                    }
                }
                abstracted_task = make_shared<DomainAbstractedTask>(abstracted_task, groups);
            }
        }

        TaskProxy abstracted_task_proxy = TaskProxy(abstracted_task.get());
        if (DEBUG)
            dump_task(abstracted_task_proxy);

        shared_ptr<AdditiveHeuristic> additive_heuristic = get_additive_heuristic(abstracted_task_proxy);

        // TODO: Fix this hack.
        Values::initialize_static_members(abstracted_task_proxy);

        Abstraction abstraction(abstracted_task_proxy, additive_heuristic);

        int rem_tasks = num_abstractions - i;
        abstraction.set_max_states((max_states - num_states) / rem_tasks);
        abstraction.set_max_time(ceil((max_time - g_timer()) / rem_tasks));
        abstraction.set_write_graphs(options.get<bool>("write_graphs"));
        abstraction.set_use_astar(options.get<bool>("use_astar"));
        abstraction.set_use_general_costs(options.get<bool>("general_costs"));

        abstraction.set_pick_strategy(PickStrategy(options.get_enum("pick")));

        if (decomposition == Decomposition::LANDMARKS)
            abstraction.separate_unreachable_facts();

        abstraction.build();
        num_states += abstraction.get_num_states();
        vector<int> needed_costs = abstraction.get_needed_costs();
        adapt_remaining_costs(remaining_costs, needed_costs);
        int init_h = abstraction.get_init_h();

        if (init_h > 0) {
            Options opts;
            opts.set<int>("cost_type", 0);
            opts.set<TaskProxy *>("task_proxy", &abstracted_task_proxy);
            opts.set<SplitTree>("split_tree", abstraction.get_split_tree());
            heuristics.emplace_back(abstracted_task, opts);
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
    cout << "CEGAR abstractions: " << heuristics.size() << endl;
    cout << "Total abstract states: " << num_states << endl;
    cout << "Init h: " << compute_heuristic(g_initial_state()) << endl;
    cout << endl;
}

int CegarHeuristic::compute_heuristic(const GlobalState &global_state) {
    int sum_h = 0;
    for (CartesianHeuristic heuristic : heuristics) {
        heuristic.evaluate(global_state);
        int h = heuristic.get_heuristic();
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
    parser.add_option<bool>("write_graphs", "write causal and landmark graphs", "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cegar", _parse);
}
