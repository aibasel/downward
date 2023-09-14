#include "sum_evaluator.h"

#include "../evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(const plugins::Options &opts)
    : CombiningEvaluator(opts) {
}

SumEvaluator::SumEvaluator(utils::LogProxy log,
                           vector<shared_ptr<Evaluator>> subevaluators,
                           basic_string<char> unparsed_config,
                           bool use_for_reporting_minima,
                           bool use_for_boosting,
                           bool use_for_counting_evaluations)
    : CombiningEvaluator(log, subevaluators, unparsed_config, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations) {
}

SumEvaluator::~SumEvaluator() {
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

TaskIndependentSumEvaluator::TaskIndependentSumEvaluator(utils::LogProxy log,
                                                         std::vector<std::shared_ptr<TaskIndependentEvaluator>> subevaluators,
                                                         std::basic_string<char> unparsed_config,
                                                         bool use_for_reporting_minima,
                                                         bool use_for_boosting,
                                                         bool use_for_counting_evaluations)
    : TaskIndependentCombiningEvaluator(log, subevaluators, unparsed_config, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations),
      unparsed_config(unparsed_config), log(log) {
}

TaskIndependentSumEvaluator::~TaskIndependentSumEvaluator() {
}

plugins::Any TaskIndependentSumEvaluator::create_task_specific(shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) {
    //TODO issue559 could this be moved into the TI_CombiningEvaluator class? In TI_MaxEvaluator we would do the very same...

    shared_ptr<SumEvaluator> task_specific_sum_evaluator;
    plugins::Any any_obj;

    if (component_map -> contains_key(make_pair(task, static_cast<void*>(this)))){
        utils::g_log << "Reuse task specific SumEvaluator..." << endl;
        any_obj = component_map -> get_value(make_pair(task, static_cast<void*>(this)));
    } else {


        utils::g_log << "Creating task specific SumEvaluator..." << endl;
        vector<shared_ptr<Evaluator>> td_subevaluators(subevaluators.size());
        transform(subevaluators.begin(), subevaluators.end(), td_subevaluators.begin(),
                  [this, &task, &component_map](const shared_ptr<TaskIndependentEvaluator> &eval) {
                      return plugins::any_cast<shared_ptr<Evaluator>>(eval->create_task_specific(task, component_map));
                  }
        );

        task_specific_sum_evaluator = make_shared<SumEvaluator>(log, td_subevaluators, unparsed_config);
        component_map -> add_entry(make_pair(task, static_cast<void*>(this)), task_specific_sum_evaluator);
    }
    return task_specific_sum_evaluator;

    //problem with the casting... is a Component class required?

}

class SumEvaluatorFeature : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentSumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(*this);
    }

    virtual shared_ptr<TaskIndependentSumEvaluator> create_component(
        const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentEvaluator>>(context, opts, "evals");
        return make_shared<TaskIndependentSumEvaluator>(utils::get_log_from_options(opts),
                                         opts.get_list<shared_ptr<TaskIndependentEvaluator>>("evals"),
                                         opts.get_unparsed_config(),
                                         opts.get<bool>("use_for_reporting_minima"),
                                         opts.get<bool>("use_for_boosting"),
                                         opts.get<bool>("use_for_counting_evaluations")
                                         );
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
