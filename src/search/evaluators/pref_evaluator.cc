#include "pref_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace pref_evaluator {
PrefEvaluator::PrefEvaluator(const plugins::Options &opts)
    : Evaluator(opts) {
}

PrefEvaluator::~PrefEvaluator() {
}

EvaluationResult PrefEvaluator::compute_result(
    EvaluationContext &eval_context) {
    EvaluationResult result;
    if (eval_context.is_preferred())
        result.set_evaluator_value(0);
    else
        result.set_evaluator_value(1);
    return result;
}

class PrefEvaluatorFeature : public plugins::TypedFeature<Evaluator, PrefEvaluator> {
public:
    PrefEvaluatorFeature() : TypedFeature("pref") {
        document_subcategory("evaluators_basic");
        document_title("Preference evaluator");
        document_synopsis("Returns 0 if preferred is true and 1 otherwise.");

        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<PrefEvaluatorFeature> _plugin;
}
