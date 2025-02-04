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

class SumEvaluatorFeature
    : public plugins::TypedFeature<Evaluator, SumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(
            *this, "sum");
    }

    virtual shared_ptr<SumEvaluator> create_component(
        const plugins::Options &opts,
        const utils::Context &context) const override {
        shared_ptr<SumEvaluator> result;
        try {
            result = plugins::make_shared_from_arg_tuples<SumEvaluator>(
                combining_evaluator::get_combining_evaluator_arguments_from_options(
                    opts));
        } catch (const utils::Exception &error) {
            context.error(error.get_message());
        }
        return result;
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
