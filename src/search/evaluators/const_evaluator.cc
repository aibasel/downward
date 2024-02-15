#include "const_evaluator.h"

#include "../plugins/plugin.h"

using namespace std;

namespace const_evaluator {
ConstEvaluator::ConstEvaluator(
    int value,
    bool use_for_reporting_minima,
    bool use_for_boosting,
    bool use_for_counting_evaluations,
    const string &description,
    utils::Verbosity verbosity)
    : Evaluator(use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations, description, verbosity), value(value) {
}

EvaluationResult ConstEvaluator::compute_result(EvaluationContext &) {
    EvaluationResult result;
    result.set_evaluator_value(value);
    return result;
}

class ConstEvaluatorFeature : public plugins::TypedFeature<Evaluator, ConstEvaluator> {
public:
    ConstEvaluatorFeature() : TypedFeature("const") {
        document_subcategory("evaluators_basic");
        document_title("Constant evaluator");
        document_synopsis("Returns a constant value.");

        add_option<int>(
            "value",
            "the constant value",
            "1",
            plugins::Bounds("0", "infinity"));
        add_evaluator_options_to_feature(*this, "const");
    }

    virtual shared_ptr<ConstEvaluator> create_component(
        const plugins::Options &opts, const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<ConstEvaluator>(
            opts.get<int>("value"),
            get_evaluator_default_arguments(),
            get_evaluator_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<ConstEvaluatorFeature> _plugin;
}
