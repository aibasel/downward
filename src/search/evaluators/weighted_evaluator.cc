#include "weighted_evaluator.h"

#include "../evaluation_context.h"
#include "../plugins/plugin.h"

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
    : Evaluator("WeightedEvaluator", log, unparsed_config,
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
    : TaskIndependentEvaluator("WeightedEvaluator" ,log, unparsed_config, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations),
      evaluator(evaluator),
      weight(weight) {
}

TaskIndependentWeightedEvaluator::~TaskIndependentWeightedEvaluator() {
}


shared_ptr<Evaluator> TaskIndependentWeightedEvaluator::create_task_specific(
    const shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<WeightedEvaluator> task_specific_x;

    if (component_map->count( static_cast<TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific WeightedEvaluator..." << endl;
        task_specific_x = dynamic_pointer_cast<WeightedEvaluator>(
            component_map->at(static_cast<TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific WeightedEvaluator..." << endl;

        task_specific_x = make_shared<WeightedEvaluator>(
            log, evaluator->create_task_specific(task, component_map, depth), weight);
        component_map->insert(make_pair<TaskIndependentComponent *, std::shared_ptr<Component>>(static_cast<TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
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
        add_evaluator_options_to_feature(*this, "weighted_eval");
    }

    virtual shared_ptr<TaskIndependentWeightedEvaluator> create_component(
        const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentEvaluator>>(context, opts, "evals");
        return make_shared<TaskIndependentWeightedEvaluator>(utils::get_log_from_options(opts),
                                                             opts.get<shared_ptr<TaskIndependentEvaluator>>("eval"),
                                                             opts.get<int>("weight"),
                                                             opts.get_unparsed_config(),
                                                             opts.get<bool>("use_for_reporting_minima", false),
                                                             opts.get<bool>("use_for_boosting", false),
                                                             opts.get<bool>("use_for_counting_evaluations", false)
                                                             );
    }
};

static plugins::FeaturePlugin<WeightedEvaluatorFeature> _plugin;
}
