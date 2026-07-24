#include "axiom_handling_evaluator.h"

#include "simple_task_transforming_evaluator.h"

#include "../evaluation_context.h"

#include "../task_utils/task_properties.h"

using namespace std;

namespace axiom_handling_evaluator {
shared_ptr<Evaluator>
TaskIndependentAxiomHandlingEvaluator::create_task_specific_component(
    const shared_ptr<AbstractTask> &task) const {
    TaskProxy proxy(*task);
    if (task_properties::has_axioms(proxy)) {
        shared_ptr<AbstractTask> axioms_task =
            make_shared<tasks::DefaultValueAxiomsTask>(task, axioms);
        shared_ptr<Evaluator> eval = nested->bind_task(axioms_task);
        return make_shared<simple_task_transforming_evaluator::
                               SimpleTaskTransformingEvaluator>(
            task, axioms_task, eval, description, verbosity);
    } else {
        /*
          If the task has no axioms, there is no need to wrap the evaluator
          and we can bind the unmodified task to the nested evaluator directly.
        */
        return nested->bind_task(task);
    }
}

TaskIndependentAxiomHandlingEvaluator::TaskIndependentAxiomHandlingEvaluator(
    shared_ptr<TaskIndependentEvaluator> nested,
    tasks::AxiomHandlingType axioms, const string &description,
    utils::Verbosity verbosity)
    : nested(move(nested)),
      axioms(axioms),
      description(description),
      verbosity(verbosity) {
}

shared_ptr<TaskIndependentEvaluator> wrap_in_axiom_handling_evaluator(
    const shared_ptr<TaskIndependentEvaluator> &eval,
    const plugins::Options &opts) {
    return components::make_shared_from_arg_tuples<
        axiom_handling_evaluator::TaskIndependentAxiomHandlingEvaluator>(
        eval, tasks::get_axioms_arguments_from_options(opts),
        get_evaluator_arguments_from_options(opts));
}

}
