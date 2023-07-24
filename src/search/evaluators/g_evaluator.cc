#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluator::GEvaluator(const plugins::Options &opts)
    : Evaluator(opts) {
}

GEvaluator::GEvaluator(utils::LogProxy log,
                       basic_string<char> unparsed_config,
                       bool use_for_reporting_minima,
                       bool use_for_boosting,
                       bool use_for_counting_evaluations)
        : Evaluator(log, unparsed_config, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations) {
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
        add_evaluator_options_to_feature(*this);
    }

    virtual shared_ptr<GEvaluator> create_component(
            const plugins::Options &opts, const utils::Context &) const override {
        return make_shared<GEvaluator>(utils::get_log_from_options(opts),
                                       opts.get_unparsed_config(),
                                       opts.get<bool>("use_for_reporting_minima"),
                                       opts.get<bool>("use_for_boosting"),
                                       opts.get<bool>("use_for_counting_evaluations") );
    }
};

static plugins::FeaturePlugin<GEvaluatorFeature> _plugin;
}
