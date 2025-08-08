#include "weighted_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"

#include "../plugins/plugin.h"

#include <cstdlib>
#include <sstream>

using namespace std;

namespace weighted_evaluator {
WeightedEvaluator::WeightedEvaluator(
    [[maybe_unused]] const shared_ptr<AbstractTask> &task,
    const shared_ptr<Evaluator> &eval, int weight,
    const string &description, utils::Verbosity verbosity)
    : Evaluator(false, false, false, description, verbosity),
      evaluator(eval),
      weight(weight) {
}

bool WeightedEvaluator::dead_ends_are_reliable() const {
    return evaluator->dead_ends_are_reliable();
}

EvaluationResult WeightedEvaluator::compute_result(
    EvaluationContext &eval_context) {
    // Note that this produces no preferred operators.
    EvaluationResult result;
    int value = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    if (value != EvaluationResult::INFTY) {
        // TODO: Check for overflow?
        value *= weight;
    }
    result.set_evaluator_value(value);
    return result;
}

void WeightedEvaluator::get_path_dependent_evaluators(set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

using TaskIndependentWeightedEvaluator = TaskIndependentComponentFeature<WeightedEvaluator, Evaluator, WeightedEvaluatorArgs>;

class WeightedEvaluatorFeature
    : public plugins::TypedFeature<TaskIndependentComponentType<Evaluator>, TaskIndependentWeightedEvaluator> {
public:
    WeightedEvaluatorFeature() : TypedFeature("weight") {
        document_subcategory("evaluators_basic");
        document_title("Weighted evaluator");
        document_synopsis(
            "Multiplies the value of the evaluator with the given weight.");

        add_option<shared_ptr<TaskIndependentComponentType<Evaluator>>>("eval", "evaluator");
        add_option<int>("weight", "weight");
        add_evaluator_options_to_feature(*this, "weight");
    }

    virtual shared_ptr<TaskIndependentWeightedEvaluator>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples_NEW<TaskIndependentWeightedEvaluator>(
            opts.get<shared_ptr<TaskIndependentComponentType<Evaluator>>>("eval"),
            opts.get<int>("weight"),
            get_evaluator_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<WeightedEvaluatorFeature> _plugin;
}
