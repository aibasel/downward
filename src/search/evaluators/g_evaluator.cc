#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluator::GEvaluator(const string &description,
                       utils::Verbosity verbosity)
    : Evaluator(false, false, false, description, verbosity) {
}


EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_g_value());
    return result;
}


TaskIndependentGEvaluator::TaskIndependentGEvaluator(const string &description,
                                                     utils::Verbosity verbosity)
    : TaskIndependentEvaluator(
          false,
          false,
          false,
          description,
          verbosity) {
}

std::shared_ptr<Evaluator> TaskIndependentGEvaluator::create_ts([[maybe_unused]] const shared_ptr <AbstractTask> &task,
                                                                [[maybe_unused]] unique_ptr <ComponentMap> &component_map,
                                                                [[maybe_unused]] int depth) const {
    return make_shared<GEvaluator>(description, verbosity);
}

class GEvaluatorFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentGEvaluator> {
public:
    GEvaluatorFeature() : TypedFeature("g") {
        document_subcategory("evaluators_basic");
        document_title("g-value evaluator");
        document_synopsis(
            "Returns the g-value (path cost) of the search node.");
        add_evaluator_options_to_feature(*this, "g");
    }

    virtual shared_ptr<TaskIndependentGEvaluator> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<TaskIndependentGEvaluator>(
            get_evaluator_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<GEvaluatorFeature> _plugin;
}
