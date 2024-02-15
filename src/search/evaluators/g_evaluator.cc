#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluator::GEvaluator(bool use_for_reporting_minima,
                       bool use_for_boosting,
                       bool use_for_counting_evaluations,
                       const string &description,
                       utils::Verbosity verbosity)
    : Evaluator(use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations, description, verbosity) {
}


EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_g_value());
    return result;
}

class GEvaluatorFeature : public plugins::TypedFeature<Evaluator, GEvaluator> {
public:
    GEvaluatorFeature() : TypedFeature("g") {
        document_subcategory("evaluators_basic");
        document_title("g-value evaluator");
        document_synopsis(
            "Returns the g-value (path cost) of the search node.");
        add_evaluator_options_to_feature(*this, "g");
    }

    virtual shared_ptr<GEvaluator> create_component(
        const plugins::Options &opts, const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<GEvaluator>(
            get_evaluator_default_arguments(),
            get_evaluator_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<GEvaluatorFeature> _plugin;
}
