#include "sum_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(const plugins::Options &opts)
    : CombiningEvaluator(opts) {
}
SumEvaluator::SumEvaluator(
    vector<shared_ptr<Evaluator>> evals,
    bool use_for_reporting_minima,
    bool use_for_boosting,
    bool use_for_counting_evaluations,
    const string &description,
    utils::Verbosity verbosity
)
    : CombiningEvaluator(evals, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations, description, verbosity) {
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

class SumEvaluatorFeature : public plugins::TypedFeature<Evaluator, SumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(*this, "sum");
    }

    virtual shared_ptr<SumEvaluator> create_component(const plugins::Options &options, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<Evaluator>>(context, options, "evals");
        return plugins::make_shared_from_arg_tuples<SumEvaluator>(
            combining_evaluator::get_combining_evaluator_arguments_from_options(options));
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
