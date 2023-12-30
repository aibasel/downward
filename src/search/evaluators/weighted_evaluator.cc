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
        shared_ptr<Evaluator> evaluator,
        int weight,
        const string &name,
        utils::Verbosity verbosity)
    : Evaluator(name, verbosity,
                false,
                false,
                false
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


TaskIndependentWeightedEvaluator::TaskIndependentWeightedEvaluator(
                                                                   shared_ptr<TaskIndependentEvaluator> evaluator,
                                                                   int weight,
                                                                   const string &name,
                                                                   utils::Verbosity verbosity)
    : TaskIndependentEvaluator(name, verbosity, false, false,
                               false),
      evaluator(evaluator),
      weight(weight) {
}


shared_ptr<Evaluator> TaskIndependentWeightedEvaluator::get_task_specific(
    const shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map, int depth) const {
    shared_ptr<WeightedEvaluator> task_specific_x;

    if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific WeightedEvaluator..." << endl;
        task_specific_x = dynamic_pointer_cast<WeightedEvaluator>(
            component_map->at(static_cast<const TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific WeightedEvaluator..." << endl;

        task_specific_x = create_ts(task, component_map, depth);
        component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>(
                                  static_cast<const TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}

    std::shared_ptr<WeightedEvaluator>
    TaskIndependentWeightedEvaluator::create_ts(const shared_ptr <AbstractTask> &task,
                                                unique_ptr <ComponentMap> &component_map, int depth) const {
        return make_shared<WeightedEvaluator>(
                evaluator->get_task_specific(task, component_map, depth),
                weight,
                name,
                verbosity);
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
        return make_shared<TaskIndependentWeightedEvaluator>(
                                                             opts.get<shared_ptr<TaskIndependentEvaluator>>("eval"),
                                                             opts.get<int>("weight"),
                                                             opts.get<string>("name"),
                                                             opts.get<utils::Verbosity>("verbosity")
                                                             );
    }
};

static plugins::FeaturePlugin<WeightedEvaluatorFeature> _plugin;
}
