#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"

#include "../plugins/plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluator::GEvaluator(
    const shared_ptr<AbstractTask> &task, const string &description,
    utils::Verbosity verbosity)
    : Evaluator(task, false, false, false, description, verbosity) {
}

EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_g_value());
    return result;
}

class GEvaluatorFeature
    : public plugins::TaskIndependentFeature<TaskIndependentEvaluator> {
public:
    GEvaluatorFeature() : TaskIndependentFeature("g") {
        document_subcategory("evaluators_basic");
        document_title("g-value evaluator");
        document_synopsis(
            "Returns the g-value (path cost) of the search node.");
        add_evaluator_options_to_feature(*this, "g");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<
            GEvaluator, Evaluator>(get_evaluator_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<GEvaluatorFeature> _plugin;
}
