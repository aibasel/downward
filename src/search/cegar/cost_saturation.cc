#include "cost_saturation.h"

#include "abstraction.h"
#include "additive_cartesian_heuristic.h"
#include "cartesian_heuristic_function.h"
#include "subtask_generators.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

#include "../tasks/modified_operator_costs_task.h"

#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <string>

using namespace std;

namespace cegar {
/*
  We reserve some memory to be able to recover from out-of-memory
  situations gracefully. When the memory runs out, we stop refining and
  start the next refinement or the search. Due to memory fragmentation
  the memory used for building the abstraction (states, transitions,
  etc.) often can't be reused for things that require big continuous
  blocks of memory. It is for this reason that we require such a large
  amount of memory padding.
*/
static const int memory_padding_in_mb = 75;

CostSaturation::CostSaturation(const Options &opts)
    : task(get_task_from_options(opts)),
      subtask_generators(opts.get_list<shared_ptr<SubtaskGenerator>>("subtasks")),
      max_states(opts.get<int>("max_states")),
      timer(opts.get<double>("max_time")),
      use_general_costs(opts.get<bool>("use_general_costs")),
      pick_split(static_cast<PickSplit>(opts.get<int>("pick"))),
      num_abstractions(0),
      num_states(0),
      initial_state(TaskProxy(*task).get_initial_state()) {
    g_log << "Initializing additive Cartesian heuristic..." << endl;

    TaskProxy task_proxy(*task);

    verify_no_axioms(task_proxy);
    verify_no_conditional_effects(task_proxy);

    for (OperatorProxy op : task_proxy.get_operators())
        remaining_costs.push_back(op.get_cost());

    utils::reserve_extra_memory_padding(memory_padding_in_mb);
    for (shared_ptr<SubtaskGenerator> subtask_generator : subtask_generators) {
        SharedTasks subtasks = subtask_generator->get_subtasks(task);
        build_abstractions(subtasks);
        if (!may_build_another_abstraction())
            break;
    }
    if (utils::extra_memory_padding_is_reserved())
        utils::release_extra_memory_padding();
    print_statistics();
}

vector<unique_ptr<CartesianHeuristicFunction>>
CostSaturation::extract_heuristic_functions() {
    return move(heuristic_functions);
}

void CostSaturation::reduce_remaining_costs(
    const vector<int> &saturated_costs) {
    assert(remaining_costs.size() == saturated_costs.size());
    for (size_t i = 0; i < remaining_costs.size(); ++i) {
        int &remaining = remaining_costs[i];
        const int &saturated = saturated_costs[i];
        assert(saturated <= remaining);
        /* Since we ignore transitions from states s with h(s)=INF, all
           saturated costs (h(s)-h(s')) are finite or -INF. */
        assert(saturated != INF);
        if (remaining == INF) {
            // INF - x = INF for finite values x.
        } else if (saturated == -INF) {
            remaining = INF;
        } else {
            remaining -= saturated;
        }
        assert(remaining >= 0);
    }
}

shared_ptr<AbstractTask> CostSaturation::get_remaining_costs_task(
    shared_ptr<AbstractTask> &parent) const {
    vector<int> costs = remaining_costs;
    return make_shared<extra_tasks::ModifiedOperatorCostsTask>(
        parent, move(costs));
}

bool CostSaturation::initial_state_is_dead_end() const {
    for (const unique_ptr<CartesianHeuristicFunction> &func :
         heuristic_functions) {
        if (func->get_value(initial_state) == INF)
            return true;
    }
    return false;
}

bool CostSaturation::may_build_another_abstraction() {
    return num_states < max_states &&
           !timer.is_expired() &&
           utils::extra_memory_padding_is_reserved() &&
           !initial_state_is_dead_end();
}

void CostSaturation::build_abstractions(
    const vector<shared_ptr<AbstractTask>> &subtasks) {
    int rem_subtasks = subtasks.size();
    for (shared_ptr<AbstractTask> subtask : subtasks) {
        subtask = get_remaining_costs_task(subtask);

        assert(num_states < max_states);
        Abstraction abstraction(
            subtask,
            max(1, (max_states - num_states) / rem_subtasks),
            timer.get_remaining_time() / rem_subtasks,
            use_general_costs,
            pick_split);

        ++num_abstractions;
        num_states += abstraction.get_num_states();
        assert(num_states <= max_states);
        reduce_remaining_costs(abstraction.get_saturated_costs());
        int init_h = abstraction.get_h_value_of_initial_state();

        if (init_h > 0) {
            heuristic_functions.push_back(
                utils::make_unique_ptr<CartesianHeuristicFunction>(
                    subtask,
                    abstraction.extract_refinement_hierarchy()));
        }
        if (!may_build_another_abstraction())
            break;

        --rem_subtasks;
    }
}

void CostSaturation::print_statistics() const {
    g_log << "Done initializing additive Cartesian heuristic" << endl;
    cout << "Cartesian abstractions built: " << num_abstractions << endl;
    cout << "Cartesian heuristic functions stored: "
         << heuristic_functions.size() << endl;
    cout << "Cartesian states: " << num_states << endl;
    cout << endl;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Additive CEGAR heuristic",
        "See the paper introducing Counterexample-guided Abstraction "
        "Refinement (CEGAR) for classical planning:" +
        utils::format_paper_reference(
            {"Jendrik Seipp", "Malte Helmert"},
            "Counterexample-guided Cartesian Abstraction Refinement",
            "http://ai.cs.unibas.ch/papers/seipp-helmert-icaps2013.pdf",
            "Proceedings of the 23rd International Conference on Automated "
            "Planning and Scheduling (ICAPS 2013)",
            "347-351",
            "AAAI Press 2013") +
        "and the paper showing how to make the abstractions additive:" +
        utils::format_paper_reference(
            {"Jendrik Seipp", "Malte Helmert"},
            "Diverse and Additive Cartesian Abstraction Heuristics",
            "http://ai.cs.unibas.ch/papers/seipp-helmert-icaps2014.pdf",
            "Proceedings of the 24th International Conference on "
            "Automated Planning and Scheduling (ICAPS 2014)",
            "289-297",
            "AAAI Press 2014"));
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    // TODO: Is the additive version consistent as well?
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    parser.add_list_option<shared_ptr<SubtaskGenerator>>(
        "subtasks",
        "subtask generators",
        "[landmarks(),goals()]");
    parser.add_option<int>(
        "max_states",
        "maximum sum of abstract states over all abstractions",
        "infinity",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds for building abstractions",
        "900",
        Bounds("0.0", "infinity"));
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
        "use_general_costs",
        "allow negative costs in cost partitioning",
        "true");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;

    CostSaturation cost_saturation(opts);

    Options heuristic_opts;
    heuristic_opts.set<shared_ptr<AbstractTask>>(
        "transform", get_task_from_options(opts));
    heuristic_opts.set<int>(
        "cost_type", NORMAL);
    heuristic_opts.set<bool>(
        "cache_estimates", opts.get<bool>("cache_estimates"));

    return new AdditiveCartesianHeuristic(
        heuristic_opts, cost_saturation.extract_heuristic_functions());
}

static Plugin<Heuristic> _plugin("cegar", _parse);
}
