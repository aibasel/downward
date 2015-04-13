#include "cegar_heuristic.h"

#include "abstraction.h"
#include "cartesian_heuristic.h"
#include "decompositions.h"
#include "modified_costs_task.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace cegar {

CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      max_states(options.get<int>("max_states")),
      timer(new CountdownTimer(options.get<double>("max_time"))),
      num_states(0) {
    DEBUG = opts.get<bool>("debug");

    verify_no_axioms_no_conditional_effects();

    for (OperatorProxy op : task->get_operators())
        remaining_costs.push_back(op.get_cost());
}

void adapt_remaining_costs(vector<int> &remaining_costs, const vector<int> &needed_costs) {
    assert(remaining_costs.size() == needed_costs.size());
    if (DEBUG)
        cout << "Remaining: " << remaining_costs << endl;
    if (DEBUG)
        cout << "Needed:    " << needed_costs << endl;
    for (size_t op_index = 0; op_index < remaining_costs.size(); ++op_index) {
        assert(in_bounds(op_index, remaining_costs));
        assert(remaining_costs[op_index] >= 0);
        assert(needed_costs[op_index] <= remaining_costs[op_index]);
        remaining_costs[op_index] -= needed_costs[op_index];
        assert(remaining_costs[op_index] >= 0);
    }
    if (DEBUG)
        cout << "Remaining: " << remaining_costs << endl;
}

shared_ptr<AbstractTask> CegarHeuristic::get_remaining_costs_task(shared_ptr<AbstractTask> parent) const {
    Options opts;
    opts.set<Subtask>("transform", parent);
    opts.set<vector<int> >("operator_costs", remaining_costs);
    return make_shared<ModifiedCostsTask>(opts);
}

void CegarHeuristic::build_abstractions(DecompositionStrategy decomposition) {
    Options decomposition_opts(options);
    decomposition_opts.set<TaskProxy *>("task_proxy", task);
    Subtasks subtasks;
    if (decomposition == DecompositionStrategy::LANDMARKS) {
        subtasks = LandmarkDecomposition(decomposition_opts).get_subtasks();
    } else if (decomposition == DecompositionStrategy::GOALS) {
        subtasks = GoalDecomposition(decomposition_opts).get_subtasks();
    } else {
        subtasks = NoDecomposition(decomposition_opts).get_subtasks();
    }

    int rem_subtasks = subtasks.size();
    for (Subtask subtask : subtasks) {
        cout << endl;

        subtask = get_remaining_costs_task(subtask);

        TaskProxy subtask_proxy = TaskProxy(subtask.get());
        if (DEBUG)
            dump_task(subtask_proxy);

        double rem_time = options.get<double>("max_time") - timer->get_elapsed_time();
        Options abs_opts(options);
        abs_opts.set<TaskProxy *>("task_proxy", &subtask_proxy);
        abs_opts.set<int>("max_states", (max_states - num_states) / rem_subtasks);
        abs_opts.set<double>("max_time", rem_time / rem_subtasks);
        abs_opts.set<bool>("separate_unreachable_facts", decomposition == DecompositionStrategy::LANDMARKS);
        Abstraction abstraction(abs_opts);

        num_states += abstraction.get_num_states();
        vector<int> needed_costs = abstraction.get_needed_costs();
        adapt_remaining_costs(remaining_costs, needed_costs);
        int init_h = abstraction.get_init_h();

        if (init_h > 0) {
            Options opts;
            opts.set<int>("cost_type", 0);
            opts.set<TaskProxy *>("task_proxy", &subtask_proxy);
            opts.set<SplitTree>("split_tree", abstraction.get_split_tree());
            heuristics.emplace_back(subtask, opts);
        }
        if (init_h == INF || num_states >= max_states || timer->is_expired())
            break;
        --rem_subtasks;
    }
}

void CegarHeuristic::initialize() {
    Log() << "Initializing CEGAR heuristic...";

    DecompositionStrategy decomposition(DecompositionStrategy(options.get_enum("decomposition")));
    vector<DecompositionStrategy> decompositions;
    if (decomposition == DecompositionStrategy::LANDMARKS_AND_GOALS_AND_NONE) {
        decompositions.push_back(DecompositionStrategy::LANDMARKS);
        decompositions.push_back(DecompositionStrategy::GOALS);
        decompositions.push_back(DecompositionStrategy::NONE);
    } else if (decomposition == DecompositionStrategy::LANDMARKS_AND_GOALS) {
        decompositions.push_back(DecompositionStrategy::LANDMARKS);
        decompositions.push_back(DecompositionStrategy::GOALS);
    } else {
        decompositions.push_back(decomposition);
    }
    for (size_t i = 0; i < decompositions.size(); ++i) {
        cout << endl << "Using decomposition " << static_cast<int>(decompositions[i]) << endl;
        build_abstractions(decompositions[i]);
        cout << endl;
        if (num_states >= max_states || timer->is_expired() ||
            compute_heuristic(g_initial_state()) == DEAD_END)
            break;
    }
    cout << endl;
    print_statistics();
}

void CegarHeuristic::print_statistics() {
    Log() << "Done initializing CEGAR heuristic";
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
    parser.add_option<double>("max_time", "maximum time in seconds for building abstractions", "900");
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
    vector<string> task_order_strategies;
    task_order_strategies.push_back("ORIGINAL");
    task_order_strategies.push_back("MIXED");
    task_order_strategies.push_back("HADD_UP");
    task_order_strategies.push_back("HADD_DOWN");
    parser.add_enum_option("task_order",
                           task_order_strategies,
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
    parser.add_option<bool>("use_general_costs", "allow negative costs in cost-partitioning", "true");
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
