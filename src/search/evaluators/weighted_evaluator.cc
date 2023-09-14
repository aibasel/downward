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
                           bool use_for_counting_evaluations)
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


TaskIndependentWeightedEvaluator::TaskIndependentWeightedEvaluator(utils::LogProxy log,
                                                         shared_ptr<TaskIndependentEvaluator> evaluator,
                                                         int weight,
                                                         basic_string<char> unparsed_config,
                                                         bool use_for_reporting_minima,
                                                         bool use_for_boosting,
                                                         bool use_for_counting_evaluations)
        : TaskIndependentEvaluator(log, unparsed_config, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations),
          evaluator(evaluator),
          weight(weight) {
}

TaskIndependentWeightedEvaluator::~TaskIndependentWeightedEvaluator() {
}

plugins::Any TaskIndependentWeightedEvaluator::create_task_specific(shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) {
    shared_ptr<WeightedEvaluator> task_specific_weighted_evaluator;
    plugins::Any any_obj;

    if (component_map -> contains_key(make_pair(task, static_cast<void*>(this)))){
        utils::g_log << "Reuse task specific WeightedEvaluator..." << endl;
        any_obj = component_map -> get_value(make_pair(task, static_cast<void*>(this)));
    } else {


        utils::g_log << "Creating task specific WeightedEvaluator..." << endl;

        task_specific_weighted_evaluator = make_shared<WeightedEvaluator>(
                log, plugins::any_cast<shared_ptr<Evaluator>>(evaluator->create_task_specific(task,component_map)), weight);
        any_obj = plugins::Any(task_specific_weighted_evaluator);
        component_map -> add_entry(make_pair(task, static_cast<void*>(this)), any_obj);
    }
    return any_obj;
}



class WeightedEvaluatorFeature : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentWeightedEvaluator> {
public:
    WeightedEvaluatorFeature() : TypedFeature("weight") {
        document_subcategory("evaluators_basic");
        document_title("Weighted evaluator");
        document_synopsis(
            "Multiplies the value of the evaluator with the given weight.");

        add_option<shared_ptr<TaskIndependentEvaluator>>("eval", "evaluator");
        add_option<int>("weight", "weight");
        add_evaluator_options_to_feature(*this);
    }

    virtual shared_ptr<TaskIndependentWeightedEvaluator> create_component(
            const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentEvaluator>>(context, opts, "evals");
        return make_shared<TaskIndependentWeightedEvaluator>(utils::get_log_from_options(opts),
                                                             opts.get<shared_ptr<TaskIndependentEvaluator>>("eval"),
                                                             opts.get<int>("weight"),
                                                             opts.get_unparsed_config(),
                                                             opts.get<bool>("use_for_reporting_minima"),
                                                             opts.get<bool>("use_for_boosting"),
                                                             opts.get<bool>("use_for_counting_evaluations")
        );
    }
};

static plugins::FeaturePlugin<WeightedEvaluatorFeature> _plugin;
}
