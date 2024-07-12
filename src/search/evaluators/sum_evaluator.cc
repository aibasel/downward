#include "sum_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(
    const vector<shared_ptr<Evaluator>> &evals,
    const string &description, utils::Verbosity verbosity)
    : CombiningEvaluator(evals, description, verbosity) {
}

int SumEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result += value;
        assert(result >= 0); // Check against overflow.
    }
    return result;
}

TaskIndependentSumEvaluator::TaskIndependentSumEvaluator(
    std::vector<std::shared_ptr<TaskIndependentEvaluator>> subevaluators,
    const string &name,
    utils::Verbosity verbosity)
    : TaskIndependentCombiningEvaluator(
          subevaluators,
          name,
          verbosity) {
}


using ConcreteProduct = SumEvaluator;
using AbstractProduct = Evaluator;
using Concrete = TaskIndependentSumEvaluator;
// TODO issue559 use templates as 'get_task_specific' is EXACTLY the same for all TI_Components
shared_ptr<AbstractProduct> Concrete::get_task_specific(
    [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) const {
    shared_ptr<ConcreteProduct> task_specific_x;

    if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific " << get_product_name() << " '" << description << "'..." << endl;
        task_specific_x = dynamic_pointer_cast<ConcreteProduct>(
            component_map->at(static_cast<const TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific " << get_product_name() << " '" << description << "'..." << endl;
        task_specific_x = create_ts(task, component_map, depth);
        component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>
                                  (static_cast<const TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}

std::shared_ptr<ConcreteProduct> Concrete::create_ts(const shared_ptr <AbstractTask> &task,
                                                     unique_ptr <ComponentMap> &component_map,
                                                     int depth) const {
    vector<shared_ptr<Evaluator>> td_subevaluators(subevaluators.size());
    transform(subevaluators.begin(), subevaluators.end(), td_subevaluators.begin(),
              [this, &task, &component_map, &depth](const shared_ptr<TaskIndependentEvaluator> &eval) {
                  return eval->get_task_specific(task, component_map, depth >= 0 ? depth + 1 : depth);
              }
              );
    return make_shared<SumEvaluator>(td_subevaluators, description, verbosity);
}


class SumEvaluatorFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentSumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(
            *this, "sum");
    }

    virtual shared_ptr<TaskIndependentSumEvaluator> create_component(
        const plugins::Options &opts,
        const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentEvaluator>>(
            context, opts, "evals");
        return plugins::make_shared_from_arg_tuples<TaskIndependentSumEvaluator>(
            combining_evaluator::get_combining_evaluator_arguments_from_options(
                opts));
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
