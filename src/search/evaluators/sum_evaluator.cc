#include "sum_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(
    [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
    const vector<shared_ptr<Evaluator>> &evals, const string &description,
    utils::Verbosity verbosity)
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

using TaskIndependentSumEvaluator =
    TaskIndependentComponentFeature<SumEvaluator, Evaluator, SumEvaluatorArgs>;

class SumEvaluatorFeature
    : public plugins::TypedFeature<Evaluator, TaskIndependentSumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(
            *this, "sum");
    }

    virtual shared_ptr<TaskIndependentSumEvaluator> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples_NEW<
            TaskIndependentSumEvaluator>(
            combining_evaluator::get_combining_evaluator_arguments_from_options(
                opts));
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
