#include "sum_evaluator.h"

#include "../task_independent_evaluator.h"

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

shared_ptr<Evaluator> TaskIndependentSumEvaluator::create_task_specific(shared_ptr<AbstractTask> &task) {
    //TODO issue559: could this be moved into the TI_CombiningEvaluator class? In TI_MaxEvaluator we would do the very same...
    utils::g_log << "Creating task specific SumEvaluator..." << endl;
    vector<shared_ptr<Evaluator>> ti_subevaluators(subevaluators.size());
    transform(subevaluators.begin(), subevaluators.end(), ti_subevaluators.begin(),
              [this, &task](const shared_ptr<TaskIndependentEvaluator> &eval) {
                  return eval->create_task_specific(task);
              }
              );
    return make_shared<SumEvaluator>(log, ti_subevaluators, unparsed_config);
}

class SumEvaluatorFeature : public plugins::TypedFeature<Evaluator, SumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(*this);
    }

    virtual shared_ptr<SumEvaluator> create_component(
        const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<Evaluator>>(context, opts, "evals");
        return make_shared<SumEvaluator>(utils::get_log_from_options(opts),
                                         opts.get_list<shared_ptr<Evaluator>>("evals"),
                                         opts.get_unparsed_config(),
                                         opts.get<bool>("use_for_reporting_minima"),
                                         opts.get<bool>("use_for_boosting"),
                                         opts.get<bool>("use_for_counting_evaluations")
                                         );
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
