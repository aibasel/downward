#include "search_algorithm.h"

#include "evaluation_context.h"
#include "evaluator.h"

#include "algorithms/ordered_set.h"
#include "plugins/plugin.h"
#include "task_utils/successor_generator.h"
#include "task_utils/task_properties.h"
#include "tasks/root_task.h"
#include "utils/countdown_timer.h"
#include "utils/rng_options.h"
#include "utils/system.h"
#include "utils/timer.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;
using utils::ExitCode;


static successor_generator::SuccessorGenerator &get_successor_generator(
    const TaskProxy &task_proxy, utils::LogProxy &log) {
    log << "Building successor generator..." << flush;
    int peak_memory_before = utils::get_peak_memory_in_kb();
    utils::Timer successor_generator_timer;
    successor_generator::SuccessorGenerator &successor_generator =
        successor_generator::g_successor_generators[task_proxy];
    successor_generator_timer.stop();
    log << "done!" << endl;
    int peak_memory_after = utils::get_peak_memory_in_kb();
    int memory_diff = peak_memory_after - peak_memory_before;
    log << "peak memory difference for successor generator creation: "
        << memory_diff << " KB" << endl
        << "time for successor generation creation: "
        << successor_generator_timer << endl;
    return successor_generator;
}

SearchAlgorithm::SearchAlgorithm(
    OperatorCost cost_type, int bound,
    double min_gen,
    double min_eval,
    double min_exp,
    double min_time,
    double max_gen,
    double max_eval,
    double max_exp,
    double max_time,
    const string &description, utils::Verbosity verbosity)
    : description(description),
      status(IN_PROGRESS),
      solution_found(false),
      task(tasks::g_root_task),
      task_proxy(*task),
      log(utils::get_log_for_verbosity(verbosity)),
      state_registry(task_proxy),
      successor_generator(get_successor_generator(task_proxy, log)),
      search_space(state_registry, log),
      statistics(log),
      bound(bound),
      cost_type(cost_type),
      is_unit_cost(task_properties::is_unit_cost(task_proxy)),
      min_gen(min_gen),
      min_eval(min_eval),
      min_exp(min_exp),
      min_time(min_time),
      max_gen(max_gen),
      max_eval(max_eval),
      max_exp(max_exp),
      max_time(max_time) {
    if (bound < 0) {
        cerr << "error: negative cost bound " << bound << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    task_properties::print_variable_statistics(task_proxy);
}

SearchAlgorithm::SearchAlgorithm(const plugins::Options &opts) // TODO options object is needed for iterated search, the prototype for issue559 resolves this
    : description(opts.get_unparsed_config()),
      status(IN_PROGRESS),
      solution_found(false),
      task(tasks::g_root_task),
      task_proxy(*task),
      log(utils::get_log_for_verbosity(
              opts.get<utils::Verbosity>("verbosity"))),
      state_registry(task_proxy),
      successor_generator(get_successor_generator(task_proxy, log)),
      search_space(state_registry, log),
      statistics(log),
      cost_type(opts.get<OperatorCost>("cost_type")),
      is_unit_cost(task_properties::is_unit_cost(task_proxy)),
      min_gen(opts.get<double>("min_gen")),
      min_eval(opts.get<double>("min_eval")),
      min_exp(opts.get<double>("min_exp")),
      min_time(opts.get<double>("min_time")),
      max_gen(opts.get<double>("max_gen")),
      max_eval(opts.get<double>("max_eval")),
      max_exp(opts.get<double>("max_exp")),
      max_time(opts.get<double>("max_time")) {
    if (opts.get<int>("bound") < 0) {
        cerr << "error: negative cost bound " << opts.get<int>("bound") << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    bound = opts.get<int>("bound");
    task_properties::print_variable_statistics(task_proxy);
}

SearchAlgorithm::~SearchAlgorithm() {
}

bool SearchAlgorithm::found_solution() const {
    return solution_found;
}

SearchStatus SearchAlgorithm::get_status() const {
    return status;
}

const Plan &SearchAlgorithm::get_plan() const {
    assert(solution_found);
    return plan;
}

void SearchAlgorithm::set_plan(const Plan &p) {
    solution_found = true;
    plan = p;
}

void SearchAlgorithm::search() {
    initialize();
    utils::g_log << "Hard limits:" << endl;
    utils::g_log << "Max runtime: " << max_time << " sec" << endl;
    utils::g_log << "Max evaluations: " << max_eval << " states" << endl;
    utils::g_log << "Max expansions: " << max_exp << " states" << endl;
    utils::g_log << "Max generations: " << max_gen << " states" << endl;

    // Implementing the default values for soft limits is a bit complicated because
    // the default "unspecified" value requires a special treatment.
    //
    // Setting the default value to a small (e.g. negative) value which is always reached is wrong because
    // the search stops immediately when no soft limits are given for exp/gen/eval/elapsed.
    //
    // Setting the default value to a large (e.g. infinity) value is also wrong,
    // because such a limit is never reached even if all other *specified* soft limits are reached,
    // making the search continue although it is supposed to stop.

    bool use_soft_limit = (min_time > 0) || (min_gen > 0) || (min_eval > 0) || (min_exp > 0);
    if (use_soft_limit){
	utils::g_log << "Soft limits:" << endl;
	utils::g_log << "Min runtime: " << min_time << " sec" << endl;
	utils::g_log << "Min evaluations: " << min_eval << " states" << endl;
	utils::g_log << "Min expansions: " << min_exp << " states" << endl;
	utils::g_log << "Min generations: " << min_gen << " states" << endl;
    }

    utils::CountdownTimer timer_max(max_time);
    utils::CountdownTimer timer_min(min_time);

    while (status == IN_PROGRESS) {
        status = step();
        auto ex = statistics.get_expanded();
        auto ev = statistics.get_evaluated_states();
        auto gen = statistics.get_generated();
        if (timer_max.is_expired() || (gen >= max_gen) || (ev >= max_eval) || (ex >= max_exp)) {
            utils::g_log << "One of the hard limits is reached. Aborting search." << endl;
            status = TIMEOUT;
            break;
        }
	if (use_soft_limit) {	// note: without this, it stops immediately when all soft limits are 0
	    if (   (min_time > 0 && timer_min.is_expired())
		&& (min_gen  > 0 && gen >= min_gen )
		&& (min_eval > 0 && ev  >= min_eval)
		&& (min_exp  > 0 && ex  >= min_exp )) {
		utils::g_log << "Soft limit: Spent all minimum required amount of computation. Aborting search." << endl;
		status = TIMEOUT;
		break;
	    }
	}
    }
    // TODO: Revise when and which search times are logged.
    log << "Actual search time: " << timer_max.get_elapsed_time() << endl;
}

bool SearchAlgorithm::check_goal_and_set_plan(const State &state) {
    if (task_properties::is_goal_state(task_proxy, state)) {
        log << "Solution found!" << endl;
        Plan plan;
        search_space.trace_path(state, plan);
        set_plan(plan);
        return true;
    }
    return false;
}

void SearchAlgorithm::save_plan_if_necessary() {
    if (found_solution()) {
        plan_manager.save_plan(get_plan(), task_proxy);
    }
}

int SearchAlgorithm::get_adjusted_cost(const OperatorProxy &op) const {
    return get_adjusted_action_cost(op, cost_type, is_unit_cost);
}



void print_initial_evaluator_values(
    const EvaluationContext &eval_context) {
    eval_context.get_cache().for_each_evaluator_result(
        [] (const Evaluator *eval, const EvaluationResult &result) {
            if (eval->is_used_for_reporting_minima()) {
                eval->report_value_for_initial_state(result);
            }
        }
        );
}

/* TODO: merge this into add_options_to_feature when all search
         algorithms support pruning.

   Method doesn't belong here because it's only useful for certain derived classes.
   TODO: Figure out where it belongs and move it there. */
void add_search_pruning_options_to_feature(plugins::Feature &feature) {
    feature.add_option<shared_ptr<PruningMethod>>(
        "pruning",
        "Pruning methods can prune or reorder the set of applicable operators in "
        "each state and thereby influence the number and order of successor states "
        "that are considered.",
        "null()");
}

tuple<shared_ptr<PruningMethod>>
get_search_pruning_arguments_from_options(
    const plugins::Options &opts) {
    return make_tuple(opts.get<shared_ptr<PruningMethod>>("pruning"));
}

void add_search_algorithm_options_to_feature(
    plugins::Feature &feature, const string &description) {
    ::add_cost_type_options_to_feature(feature);
    feature.add_option<int>(
        "bound",
        "exclusive depth bound on g-values. Cutoffs are always performed according to "
        "the real cost, regardless of the cost_type parameter", "infinity");
    feature.add_option<double>(
        "max_time",
        "maximum time in seconds the search is allowed to run for. The "
        "timeout is only checked after each complete search step "
        "(usually a node expansion), so the actual runtime can be arbitrarily "
        "longer. Therefore, this parameter should not be used for time-limiting "
        "experiments. Timed-out searches are treated as failed searches, "
        "just like incomplete search algorithms that exhaust their search space.",
        "infinity");
    feature.add_option<double>(
        "max_eval",
        "maximum number of evaluated states (measured by statistics.get_evaluated_states()).",
        "infinity");
    feature.add_option<double>(
        "max_gen",
        "maximum number of generated states (measured by statistics.get_generated()).",
        "infinity");
    feature.add_option<double>(
        "max_exp",
        "maximum number of expanded states (measured by statistics.get_expanded()).",
        "infinity");
    feature.add_option<double>(
        "min_time",
        "specifies a soft limit i.e. minimum time in seconds the search must run for. "
	"The same accuracy statement as the one for max_time applies to this option. "
	"\n"
	"0 and negative values indicate that the soft limit is disabled. "
	"\n"
	"Soft limit addresses the limitation of hard limits (e.g. max_time) that one limit interferes another. "
	"For example, when max_eval = 100k and max_time = 5 sec, "
	"majority of runs are cut off before reaching 100k evaluations, "
	"and the result is not an appropriate representation of running 100k evaluations. "
	"To measure the evaluations and time separately, therefore one would run two separate experiments, "
	"which is a waste of compute for instances solved by both. "
	"Instead of running them separately, this option allows one to run the union of the two configurations "
	"which can be later filtered to produce distinct tables/figures. "
	"\n"
	"Soft limit works as follows: "
	"It continues the search until the solution is found, or if all soft limits are met. ",
        "0");
    feature.add_option<double>(
        "min_eval",
        "specifies a soft limit i.e. the minimum number of evaluated states (measured by statistics.get_evaluated_states())."
	"\n"
	"0 and negative values indicate that the soft limit is disabled. ",
        "0");
    feature.add_option<double>(
        "min_gen",
        "specifies a soft limit i.e. minimum number of generated states (measured by statistics.get_generated())."
	"\n"
	"0 and negative values indicate that the soft limit is disabled. ",
        "0");
    feature.add_option<double>(
        "min_exp",
        "specifies a soft limit i.e. minimum number of expanded states (measured by statistics.get_expanded())."
	"\n"
	"0 and negative values indicate that the soft limit is disabled. ",
        "0");
    feature.add_option<string>(
        "description",
        "description used to identify search algorithm in logs",
        "\"" + description + "\"");
    utils::add_log_options_to_feature(feature);
}

tuple<OperatorCost, int, double, double, double, double, double, double, double, double, string, utils::Verbosity>
get_search_algorithm_arguments_from_options(
    const plugins::Options &opts) {
    return tuple_cat(
        ::get_cost_type_arguments_from_options(opts),
        make_tuple(
            opts.get<int>("bound"),
            opts.get<double>("min_gen"),
            opts.get<double>("min_eval"),
            opts.get<double>("min_exp"),
            opts.get<double>("min_time"),
            opts.get<double>("max_gen"),
            opts.get<double>("max_eval"),
            opts.get<double>("max_exp"),
            opts.get<double>("max_time"),
            opts.get<string>("description")
            ),
        utils::get_log_arguments_from_options(opts)
        );
}

/* Method doesn't belong here because it's only useful for certain derived classes.
   TODO: Figure out where it belongs and move it there. */
void add_successors_order_options_to_feature(
    plugins::Feature &feature) {
    feature.add_option<bool>(
        "randomize_successors",
        "randomize the order in which successors are generated",
        "false");
    feature.add_option<bool>(
        "preferred_successors_first",
        "consider preferred operators first",
        "false");
    feature.document_note(
        "Successor ordering",
        "When using randomize_successors=true and "
        "preferred_successors_first=true, randomization happens before "
        "preferred operators are moved to the front.");
    utils::add_rng_options_to_feature(feature);
}

tuple<bool, bool, int> get_successors_order_arguments_from_options(
    const plugins::Options &opts) {
    return tuple_cat(
        make_tuple(
            opts.get<bool>("randomize_successors"),
            opts.get<bool>("preferred_successors_first")
            ),
        utils::get_rng_arguments_from_options(opts)
        );
}

static class SearchAlgorithmCategoryPlugin : public plugins::TypedCategoryPlugin<SearchAlgorithm> {
public:
    SearchAlgorithmCategoryPlugin() : TypedCategoryPlugin("SearchAlgorithm") {
        // TODO: Replace add synopsis for the wiki page.
        // document_synopsis("...");
    }
}
_category_plugin;

void collect_preferred_operators(
    EvaluationContext &eval_context,
    Evaluator *preferred_operator_evaluator,
    ordered_set::OrderedSet<OperatorID> &preferred_operators) {
    if (!eval_context.is_evaluator_value_infinite(preferred_operator_evaluator)) {
        for (OperatorID op_id : eval_context.get_preferred_operators(preferred_operator_evaluator)) {
            preferred_operators.insert(op_id);
        }
    }
}
