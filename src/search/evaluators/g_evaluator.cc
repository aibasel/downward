#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluator::GEvaluator(
    [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
	const string &description,
                       utils::Verbosity verbosity)
    : Evaluator(false, false, false, description, verbosity) {
}


EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_g_value());
    return result;
}



//TaskIndependentGEvaluator::TaskIndependentGEvaluator(
//    TaskIndepenendGEvaluatorArgs args)
//    : TaskIndependentComponent<Evaluator>(
//          get<0>(args),
//          get<1>(args)) {
//	cout << "NEW CALL" << description << endl;
//}
//
//TaskIndependentGEvaluator::TaskIndependentGEvaluator(const string &description,
//                                                     utils::Verbosity verbosity)
//    : TaskIndependentComponent<Evaluator>(
//          description,
//          verbosity) {
//	cout << "OLD CALL" << description << endl;
//}
//
//shared_ptr<Evaluator> TaskIndependentGEvaluator::create_task_specific( const shared_ptr <AbstractTask> &task,
//     unique_ptr <ComponentMap> &component_map,
//     int depth) const {
//    return specify<GEvaluator>(description,verbosity,task,component_map,depth);
//}



class GEvaluatorFeature
    : public plugins
::TypedFeature<TaskIndependentComponentType<Evaluator>, TaskIndependentComponentFeature<GEvaluator, Evaluator, GEvaluatorArgs>> {
public:
    GEvaluatorFeature() : TypedFeature("g") {
        document_subcategory("evaluators_basic");
        document_title("g-value evaluator");
        document_synopsis(
            "Returns the g-value (path cost) of the search node.");
        add_evaluator_options_to_feature(*this, "g");
    }

    virtual shared_ptr<TaskIndependentComponentFeature<GEvaluator,Evaluator,GEvaluatorArgs>>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples_NEW<TaskIndependentComponentFeature<GEvaluator, Evaluator, GEvaluatorArgs>>(
            get_evaluator_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<GEvaluatorFeature> _plugin;
}
