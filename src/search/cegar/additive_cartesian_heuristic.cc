#include "additive_cartesian_heuristic.h"

#include "abstraction.h"
#include "cartesian_heuristic.h"
#include "decompositions.h"
#include "utils.h"

#include "../evaluation_context.h"
#include "../logging.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"
#include "../utilities_memory.h"

#include "../tasks/modified_costs_task.h"

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
      decompositions(opts.get_list<shared_ptr<Decomposition>>("decompositions")),
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
    return make_shared<tasks::ModifiedCostsTask>(parent, remaining_costs);
}

bool AdditiveCartesianHeuristic::may_build_another_abstraction() {
    return num_states < max_states &&
           !timer->is_expired() &&
           extra_memory_padding_is_reserved() &&
           compute_heuristic(g_initial_state()) != DEAD_END;
}

void AdditiveCartesianHeuristic::build_abstractions(
    const Decomposition &decomposition) {
    Tasks subtasks = decomposition.get_subtasks(task);

    int rem_subtasks = subtasks.size();
    for (Task subtask : subtasks) {
        cout << endl;

        subtask = get_remaining_costs_task(subtask);

        if (DEBUG) {
            cout << "Next subtask:" << endl;
            dump_task(TaskProxy(*subtask));
        }

        Options abs_opts(options);
        abs_opts.set<Task>("transform", subtask);
        abs_opts.set<int>("max_states", (max_states - num_states) / rem_subtasks);
        abs_opts.set<double>("max_time", timer->get_remaining_time() / rem_subtasks);
        /*
          For landmark tasks we have to map all states in which the landmark
          might have been achieved to arbitrary abstract goal states. For the
          other decompositions our method won't find unreachable facts, but
          calling it unconditionally for subtasks with one goal doesn't hurt
          and simplifies the implementation.
        */
        abs_opts.set<bool>("separate_unreachable_facts",
                           TaskProxy(*subtask).get_goals().size() == 1);
        Abstraction abstraction(abs_opts);

        ++num_abstractions;
        num_states += abstraction.get_num_states();
        vector<int> needed_costs = abstraction.get_needed_costs();
        reduce_remaining_costs(needed_costs);
        int init_h = abstraction.get_h_value_of_initial_state();

        if (init_h > 0) {
            Options opts;
            opts.set<int>("cost_type", 0);
            opts.set<Task>("transform", subtask);
            opts.set<bool>("cache_estimates", cache_h_values);
            heuristics.push_back(make_shared<CartesianHeuristic>(
                                     opts, abstraction.get_split_tree()));
        }
        if (!may_build_another_abstraction())
            break;

        --rem_subtasks;
    }
}

void AdditiveCartesianHeuristic::initialize() {
    log("Initializing additive Cartesian heuristic...");
    reserve_extra_memory_padding();
    for (shared_ptr<Decomposition> decomposition : decompositions) {
        build_abstractions(*decomposition);
        cout << endl;
        if (!may_build_another_abstraction())
            break;
    }
    if (extra_memory_padding_is_reserved())
        release_extra_memory_padding();
    print_statistics();
    cout << endl;
}

void AdditiveCartesianHeuristic::print_statistics() const {
    log("Done initializing additive Cartesian heuristic");
    cout << "Cartesian abstractions built: " << num_abstractions << endl;
    cout << "Cartesian heuristics stored: " << heuristics.size() << endl;
    cout << "Cartesian states: " << num_states << endl;
}

int AdditiveCartesianHeuristic::compute_heuristic(const GlobalState &global_state) {
    EvaluationContext eval_context(global_state);
    int sum_h = 0;
    for (shared_ptr<CartesianHeuristic> heuristic : heuristics) {
        if (eval_context.is_heuristic_infinite(heuristic.get()))
            return DEAD_END;
        sum_h += eval_context.get_heuristic_value(heuristic.get());
    }
    assert(sum_h >= 0);
    return sum_h;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Additive CEGAR heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    parser.add_list_option<shared_ptr<Decomposition> >(
        "decompositions", "task decompositions", "[landmarks,goals]");
    parser.add_option<int>(
        "max_states", "maximum number of abstract states", "infinity");
    parser.add_option<double>(
        "max_time", "maximum time in seconds for building abstractions", "900");
    vector<string> pick_strategies;
    pick_strategies.push_back("RANDOM");
    pick_strategies.push_back("MIN_UNWANTED");
    pick_strategies.push_back("MAX_UNWANTED");
    pick_strategies.push_back("MIN_REFINED");
    pick_strategies.push_back("MAX_REFINED");
    pick_strategies.push_back("MIN_HADD");
    pick_strategies.push_back("MAX_HADD");
    parser.add_enum_option(
        "pick", pick_strategies, "split-selection strategy", "MAX_REFINED");
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
