#include "heuristic.h"

#include "evaluation_context.h"
#include "evaluation_result.h"

#include "plugins/plugin.h"
#include "task_utils/task_properties.h"
#include "tasks/cost_adapted_task.h"
#include "tasks/root_task.h"

#include <cassert>

using namespace std;


Heuristic::Heuristic(const std::basic_string<char> unparsed_config,
                     utils::LogProxy log,
                     bool cache_evaluator_values,
                     const shared_ptr<AbstractTask> task)
    : Evaluator(log, unparsed_config, true, true, true),
      heuristic_cache(HEntry(NO_VALUE, true)), //TODO: is true really a good idea here?
      cache_evaluator_values(cache_evaluator_values),
      task(task), task_proxy(*task) {
}


Heuristic::~Heuristic() {
}

void Heuristic::set_preferred(const OperatorProxy &op) {
    preferred_operators.insert(op.get_ancestor_operator_id(tasks::g_root_task.get()));
}

State Heuristic::convert_ancestor_state(const State &ancestor_state) const {
    return task_proxy.convert_ancestor_state(ancestor_state);
}

void Heuristic::add_options_to_feature(plugins::Feature &feature) {
    cout << "Heuristic::add_options_to_feature(plugins::Feature &feature) " << endl;
    add_evaluator_options_to_feature(feature);
    feature.add_option<shared_ptr<TaskIndependentAbstractTask>>(
        "transform",
        "Optional task transformation for the heuristic."
        " Currently, adapt_costs() and no_transform() are available.",
        "adapt_costs(normal)"); // TODO: issue559 put default value back to 'no_transform()'.
    feature.add_option<bool>("cache_estimates", "cache heuristic estimates", "true");
}

EvaluationResult Heuristic::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;

    assert(preferred_operators.empty());

    const State &state = eval_context.get_state();
    bool calculate_preferred = eval_context.get_calculate_preferred();

    int heuristic = NO_VALUE;

    if (!calculate_preferred && cache_evaluator_values &&
        heuristic_cache[state].h != NO_VALUE && !heuristic_cache[state].dirty) {
        heuristic = heuristic_cache[state].h;
        result.set_count_evaluation(false);
    } else {
        heuristic = compute_heuristic(state);
        if (cache_evaluator_values) {
            heuristic_cache[state] = HEntry(heuristic, false);
        }
        result.set_count_evaluation(true);
    }

    assert(heuristic == DEAD_END || heuristic >= 0);

    if (heuristic == DEAD_END) {
        /*
          It is permissible to mark preferred operators for dead-end
          states (thus allowing a heuristic to mark them on-the-fly
          before knowing the final result), but if it turns out we
          have a dead end, we don't want to actually report any
          preferred operators.
        */
        preferred_operators.clear();
        heuristic = EvaluationResult::INFTY;
    }

#ifndef NDEBUG
    TaskProxy global_task_proxy = state.get_task();
    OperatorsProxy global_operators = global_task_proxy.get_operators();
    if (heuristic != EvaluationResult::INFTY) {
        for (OperatorID op_id : preferred_operators)
            assert(task_properties::is_applicable(global_operators[op_id], state));
    }
#endif

    result.set_evaluator_value(heuristic);
    result.set_preferred_operators(preferred_operators.pop_as_vector());
    assert(preferred_operators.empty());

    return result;
}

bool Heuristic::does_cache_estimates() const {
    return cache_evaluator_values;
}

bool Heuristic::is_estimate_cached(const State &state) const {
    return heuristic_cache[state].h != NO_VALUE;
}

int Heuristic::get_cached_estimate(const State &state) const {
    assert(is_estimate_cached(state));
    return heuristic_cache[state].h;
}

TaskIndependentHeuristic::TaskIndependentHeuristic(const string unparsed_config,
                                                   utils::LogProxy log,
                                                   bool cache_evaluator_values,
                                                   std::shared_ptr<TaskIndependentAbstractTask> task_transformation
                                                   )
    : TaskIndependentEvaluator(log, unparsed_config, true, true, true),
      cache_evaluator_values(cache_evaluator_values), task_transformation(task_transformation) {
}

shared_ptr<Heuristic> TaskIndependentHeuristic::create_task_specific_Heuristic(const shared_ptr<AbstractTask> &task, int depth) {
    log << std::string(depth, ' ') << "Creating Heuristic as root component..." << endl;
    std::unique_ptr<ComponentMap> component_map = std::make_unique<ComponentMap>();
    return create_task_specific_Heuristic(task, component_map, depth);
}

shared_ptr<Heuristic> TaskIndependentHeuristic::create_task_specific_Heuristic([[maybe_unused]] const shared_ptr<AbstractTask> &task, [[maybe_unused]] unique_ptr<ComponentMap> &component_map, int depth) {
    cerr << "Tries to create Heuristic in an unimplemented way." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}


shared_ptr<Evaluator> TaskIndependentHeuristic::create_task_specific_Evaluator(const shared_ptr<AbstractTask> &task, unique_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<Heuristic> x = create_task_specific_Heuristic(task, component_map, depth);
    return static_pointer_cast<Evaluator>(x);
}
