#include "additive_cartesian_heuristic.h"

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
AdditiveCartesianHeuristic::AdditiveCartesianHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      decompositions(opts.get_list<Decomposition *>("decompositions")),
      max_states(options.get<int>("max_states")),
      timer(new CountdownTimer(options.get<double>("max_time"))),
      num_states(0) {
    DEBUG = opts.get<bool>("debug");

    verify_no_axioms_no_conditional_effects();

    for (OperatorProxy op : task_proxy.get_operators())
        remaining_costs.push_back(op.get_cost());
}

void reduce_remaining_costs(vector<int> &remaining_costs, const vector<int> &needed_costs) {
    assert(remaining_costs.size() == needed_costs.size());
    for (size_t i = 0; i < remaining_costs.size(); ++i) {
        assert(needed_costs[i] <= remaining_costs[i]);
        remaining_costs[i] -= needed_costs[i];
        assert(remaining_costs[i] >= 0);
    }
}

shared_ptr<AbstractTask> AdditiveCartesianHeuristic::get_remaining_costs_task(shared_ptr<AbstractTask> parent) const {
    Options opts;
    opts.set<Subtask>("transform", parent);
    opts.set<vector<int> >("operator_costs", remaining_costs);
    return make_shared<ModifiedCostsTask>(opts);
}

void AdditiveCartesianHeuristic::build_abstractions(const Decomposition &decomposition) {
    Subtasks subtasks = decomposition.get_subtasks();

    int rem_subtasks = subtasks.size();
    for (Subtask subtask : subtasks) {
        cout << endl;

        subtask = get_remaining_costs_task(subtask);

        if (DEBUG) {
            cout << "Next subtask:" << endl;
            dump_task(TaskProxy(*subtask));
        }

        double rem_time = options.get<double>("max_time") - timer->get_elapsed_time();
        Options abs_opts(options);
        abs_opts.set<shared_ptr<AbstractTask> >("transform", subtask);
        abs_opts.set<int>("max_states", (max_states - num_states) / rem_subtasks);
        abs_opts.set<double>("max_time", rem_time / rem_subtasks);
        // TODO: Can we only do this for LandmarkDecompositions?
        abs_opts.set<bool>("separate_unreachable_facts", true);
        Abstraction abstraction(abs_opts);

        num_states += abstraction.get_num_states();
        vector<int> needed_costs = abstraction.get_needed_costs();
        reduce_remaining_costs(remaining_costs, needed_costs);
        int init_h = abstraction.get_init_h();

        if (init_h > 0) {
            Options opts;
            opts.set<int>("cost_type", 0);
            opts.set<shared_ptr<AbstractTask> >("transform", subtask);
            opts.set<SplitTree>("split_tree", abstraction.get_split_tree());
            heuristics.emplace_back(subtask, opts);
        }
        if (init_h == INF || num_states >= max_states || timer->is_expired())
            break;

        --rem_subtasks;
    }
}

void AdditiveCartesianHeuristic::initialize() {
    Log() << "Initializing CEGAR heuristic...";
    for (Decomposition *decomposition : decompositions) {
        build_abstractions(*decomposition);
        cout << endl;
        if (num_states >= max_states || timer->is_expired() ||
            compute_heuristic(g_initial_state()) == DEAD_END)
            break;
    }
    print_statistics();
    cout << endl;
}

void AdditiveCartesianHeuristic::print_statistics() {
    Log() << "Done initializing CEGAR heuristic";
    cout << "Cartesian abstractions: " << heuristics.size() << endl;
    cout << "Total abstract states: " << num_states << endl;
    cout << "Initial h value: " << compute_heuristic(g_initial_state()) << endl;
}

int AdditiveCartesianHeuristic::compute_heuristic(const GlobalState &global_state) {
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
    parser.add_list_option<Decomposition *>("decompositions", "Task decompositions", "[landmarks,goals]");
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
    parser.add_enum_option(
        "pick", pick_strategies, "how to pick the next flaw", "MAX_REFINED");
    parser.add_option<bool>("use_astar", "use A* to find the *single* next solution", "true");
    parser.add_option<bool>("use_general_costs", "allow negative costs in cost partitioning", "true");
    parser.add_option<bool>("debug", "print debugging output", "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new AdditiveCartesianHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cegar", _parse);
}
