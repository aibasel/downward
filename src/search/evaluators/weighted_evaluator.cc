#include "weighted_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

#include <cstdlib>
#include <sstream>

using namespace std;

namespace weighted_evaluator {
WeightedEvaluator::WeightedEvaluator(const plugins::Options &opts)
    : Evaluator(opts),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
      weight(opts.get<int>("weight")) {
}

WeightedEvaluator::WeightedEvaluator(
                           utils::LogProxy log,
                           shared_ptr<Evaluator> evaluator,
                           int weight,
                           basic_string<char> unparsed_config,
                           bool use_for_reporting_minima,
                           bool use_for_boosting,
                           bool use_for_counting_evaluations,
                           vector<shared_ptr<Evaluator>> subevaluators)
        : Evaluator(log,unparsed_config,
                    use_for_reporting_minima,
                    use_for_boosting,
                    use_for_counting_evaluations
                    ),
                    evaluator(evaluator),
          weight(weight) {
}

WeightedEvaluator::~WeightedEvaluator() {
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

class WeightedEvaluatorFeature : public plugins::TypedFeature<Evaluator, WeightedEvaluator> {
public:
    WeightedEvaluatorFeature() : TypedFeature("weight") {
        document_subcategory("evaluators_basic");
        document_title("Weighted evaluator");
        document_synopsis(
            "Multiplies the value of the evaluator with the given weight.");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<int>("weight", "weight");
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<WeightedEvaluatorFeature> _plugin;
}
