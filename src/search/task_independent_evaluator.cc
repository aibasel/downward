#include "task_independent_evaluator.h"

#include "plugins/plugin.h"

using namespace std;

TaskIndependentEvaluator::TaskIndependentEvaluator(utils::LogProxy log,
                                                   const string unparsed_config,
                                                   bool use_for_reporting_minima,
                                                   bool use_for_boosting,
                                                   bool use_for_counting_evaluations)
    : description(unparsed_config),
      use_for_reporting_minima(use_for_reporting_minima),
      use_for_boosting(use_for_boosting),
      use_for_counting_evaluations(use_for_counting_evaluations),
      log(log) {
}

static class TaskIndependentEvaluatorCategoryPlugin : public plugins::TypedCategoryPlugin<TaskIndependentEvaluator> {
public:
    TaskIndependentEvaluatorCategoryPlugin() : TypedCategoryPlugin("TIEvaluator") {
        document_synopsis(
            "An evaluator specification is either a newly created evaluator "
            "instance or an evaluator that has been defined previously. "
            "This page describes how one can specify a new evaluator instance. "
            "For re-using evaluators, see OptionSyntax#Evaluator_Predefinitions.\n\n"
            "If the evaluator is a heuristic, "
            "definitions of //properties// in the descriptions below:\n\n"
            " * **admissible:** h(s) <= h*(s) for all states s\n"
            " * **consistent:** h(s) <= c(s, s') + h(s') for all states s "
            "connected to states s' by an action with cost c(s, s')\n"
            " * **safe:** h(s) = infinity is only true for states "
            "with h*(s) = infinity\n"
            " * **preferred operators:** this heuristic identifies "
            "preferred operators ");
        allow_variable_binding();
    }
}
_category_plugin;
