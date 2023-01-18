#include "const_evaluator.h"

#include "../plugins/plugin.h"

using namespace std;

namespace const_evaluator {
ConstEvaluator::ConstEvaluator(const plugins::Options &opts)
    : Evaluator(opts), value(opts.get<int>("value")) {
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
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<ConstEvaluatorFeature> _plugin;
}
