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
      decompositions(opts.get_list<shared_ptr<Decomposition> >("decompositions")),
      max_states(options.get<int>("max_states")),
      timer(new CountdownTimer(options.get<double>("max_time"))),
      num_abstractions(0),
      num_states(0) {
    DEBUG = opts.get<bool>("debug");

    verify_no_axioms(task_proxy);
    verify_no_conditional_effects(task_proxy);

    for (OperatorProxy op : task_proxy.get_operators())
        remaining_costs.push_back(op.get_cost());
}

void AdditiveCartesianHeuristic::reduce_remaining_costs(
        const vector<int> &needed_costs) {
    assert(remaining_costs.size() == needed_costs.size());
    for (size_t i = 0; i < remaining_costs.size(); ++i) {
        assert(needed_costs[i] <= remaining_costs[i]);
        remaining_costs[i] -= needed_costs[i];
        assert(remaining_costs[i] >= 0);
    }
}

shared_ptr<AbstractTask> AdditiveCartesianHeuristic::get_remaining_costs_task(
        shared_ptr<AbstractTask> parent) const {
    Options opts;
    opts.set<Subtask>("transform", parent);
    opts.set<vector<int> >("operator_costs", remaining_costs);
    return make_shared<ModifiedCostsTask>(opts);
}

bool AdditiveCartesianHeuristic::may_build_another_abstraction() {
    return num_states < max_states &&
           !timer->is_expired() &&
           memory_padding_is_reserved() &&
           compute_heuristic(g_initial_state()) != DEAD_END;
}

void AdditiveCartesianHeuristic::build_abstractions(
        const Decomposition &decomposition) {
    Subtasks subtasks = decomposition.get_subtasks();

    int rem_subtasks = subtasks.size();
    for (Subtask subtask : subtasks) {
        cout << endl;

        subtask = get_remaining_costs_task(subtask);

        if (DEBUG) {
            cout << "Next subtask:" << endl;
            dump_task(TaskProxy(*subtask));
        }

        Options abs_opts(options);
        abs_opts.set<Subtask>("transform", subtask);
        abs_opts.set<int>("max_states", (max_states - num_states) / rem_subtasks);
        abs_opts.set<double>("max_time", timer->get_remaining_time() / rem_subtasks);
        // TODO: Should we only do this for LandmarkDecompositions?
        abs_opts.set<bool>("separate_unreachable_facts", true);
        Abstraction abstraction(abs_opts);

        ++num_abstractions;
        num_states += abstraction.get_num_states();
        vector<int> needed_costs = abstraction.get_needed_costs();
        reduce_remaining_costs(needed_costs);
        int init_h = abstraction.get_h_value_of_initial_state();

        if (init_h > 0) {
            Options opts;
            opts.set<int>("cost_type", 0);
            opts.set<Subtask>("transform", subtask);
            opts.set<SplitTree>("split_tree", abstraction.get_split_tree());
            heuristics.emplace_back(opts);
        }
        if (!may_build_another_abstraction())
            break;

        --rem_subtasks;
    }
}

void AdditiveCartesianHeuristic::initialize() {
    Log() << "Initializing additive Cartesian heuristic...";
    reserve_memory_padding();
    for (shared_ptr<Decomposition> decomposition : decompositions) {
        build_abstractions(*decomposition);
        cout << endl;
        if (!may_build_another_abstraction())
            break;
    }
    if (memory_padding_is_reserved())
        release_memory_padding();
    print_statistics();
    cout << endl;
}

void AdditiveCartesianHeuristic::print_statistics() const {
    Log() << "Done initializing additive Cartesian heuristic";
    cout << "Cartesian abstractions built: " << num_abstractions << endl;
    cout << "Cartesian heuristics stored: " << heuristics.size() << endl;
    cout << "Cartesian states: " << num_states << endl;
}

int AdditiveCartesianHeuristic::compute_heuristic(const GlobalState &global_state) {
    int sum_h = 0;
    for (CartesianHeuristic heuristic : heuristics) {
        heuristic.evaluate(global_state);
        int h = heuristic.get_heuristic();
        if(h == DEAD_END)
            return DEAD_END;
        sum_h += h;
    }
    assert(sum_h >= 0);
    return sum_h;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.add_list_option<shared_ptr<Decomposition> >(
        "decompositions", "task decompositions", "[landmarks,goals]");
    parser.add_option<int>(
        "max_states", "maximum number of abstract states", "infinity");
    parser.add_option<double>(
        "max_time", "maximum time in seconds for building abstractions", "900");
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
    parser.add_option<bool>(
        "use_general_costs", "allow negative costs in cost partitioning", "true");
    parser.add_option<bool>(
        "debug", "print debugging output", "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return new AdditiveCartesianHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cegar", _parse);
}
