#include "max_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>

using namespace std;

namespace max_evaluator {
MaxEvaluator::MaxEvaluator(
    const vector<shared_ptr<Evaluator>> &evals,
    bool use_for_reporting_minima,
    bool use_for_boosting,
    bool use_for_counting_evaluations,
    const string &description,
    utils::Verbosity verbosity)
    : CombiningEvaluator(evals, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations, description, verbosity) {
}

int MaxEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result = max(result, value);
    }
    return result;
}

class MaxEvaluatorFeature : public plugins::TypedFeature<Evaluator, MaxEvaluator> {
public:
    MaxEvaluatorFeature() : TypedFeature("max") {
        document_subcategory("evaluators_basic");
        document_title("Max evaluator");
        document_synopsis(
            "Calculates the maximum of the sub-evaluators.");
        combining_evaluator::add_combining_evaluator_options_to_feature(*this, "max");
    }

    virtual shared_ptr<MaxEvaluator> create_component(const plugins::Options &options, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<Evaluator>>(context, options, "evals");
        return plugins::make_shared_from_arg_tuples<MaxEvaluator>(
            combining_evaluator::get_combining_evaluator_arguments_from_options(options));
    }
};

static plugins::FeaturePlugin<MaxEvaluatorFeature> _plugin;
}
