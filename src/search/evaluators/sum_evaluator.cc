#include "sum_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(
    const vector<shared_ptr<Evaluator>> &evals,
    const string &description, utils::Verbosity verbosity)
    : CombiningEvaluator(evals, description, verbosity) {
}

int SumEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result += value;
        assert(result >= 0); // Check against overflow.
    }
    return result;
}

TaskIndependentSumEvaluator::TaskIndependentSumEvaluator(
    const vector<shared_ptr<TaskIndependentEvaluator>> &subevaluators,
    const string &description,
    utils::Verbosity verbosity)
    : TaskIndependentCombiningEvaluator(
          subevaluators,
          description,
          verbosity) {
}

std::shared_ptr<Evaluator> TaskIndependentSumEvaluator::create_ts(
    const shared_ptr <AbstractTask> &task,
    unique_ptr <ComponentMap> &component_map, int depth) const {
    vector<shared_ptr<Evaluator>> td_subevaluators(subevaluators.size());
    transform(subevaluators.begin(), subevaluators.end(), td_subevaluators.begin(),
              [this, &task, &component_map, &depth](const shared_ptr<TaskIndependentEvaluator> &eval) {
                  return eval->get_task_specific(task, component_map, depth >= 0 ? depth + 1 : depth);
              }
              );
    return make_shared<SumEvaluator>(td_subevaluators, description, verbosity);
}

class SumEvaluatorFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentSumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(
            *this, "sum");
    }

    virtual shared_ptr<TaskIndependentSumEvaluator> create_component(
        const plugins::Options &opts,
        const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentEvaluator>>(
            context, opts, "evals");
        return plugins::make_shared_from_arg_tuples<TaskIndependentSumEvaluator>(
            combining_evaluator::get_combining_evaluator_arguments_from_options(
                opts));
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
