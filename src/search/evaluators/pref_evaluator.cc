#include "pref_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"

#include "../plugins/plugin.h"

using namespace std;

namespace pref_evaluator {
PrefEvaluator::PrefEvaluator(
    const shared_ptr<AbstractTask> &task, const string &description,
    utils::Verbosity verbosity)
    : Evaluator(task, false, false, false, description, verbosity) {
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

class PrefEvaluatorFeature
    : public plugins::TaskIndependentFeature<TaskIndependentEvaluator> {
public:
    PrefEvaluatorFeature() : TaskIndependentFeature("pref") {
        document_subcategory("evaluators_basic");
        document_title("Preference evaluator");
        document_synopsis("Returns 0 if preferred is true and 1 otherwise.");

        add_evaluator_options_to_feature(*this, "pref");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<PrefEvaluator, Evaluator>(
            get_evaluator_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<PrefEvaluatorFeature> _plugin;
}
