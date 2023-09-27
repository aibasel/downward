#include "sum_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>
#include <utility>

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
    : CombiningEvaluator(log,
                         subevaluators,
                         unparsed_config,
                         use_for_reporting_minima,
                         use_for_boosting,
                         use_for_counting_evaluations) {
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
      log(log) {
}

TaskIndependentSumEvaluator::~TaskIndependentSumEvaluator() {
}


shared_ptr<SumEvaluator> TaskIndependentSumEvaluator::create_task_specific_SumEvaluator(shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) {
    shared_ptr<SumEvaluator> task_specific_sum_evaluator;
    if (component_map->contains_key(make_pair(task, static_cast<void *>(this)))) {
        log << "Reuse task specific SumEvaluator..." << endl;
        task_specific_sum_evaluator = plugins::any_cast<shared_ptr<SumEvaluator>>(
            component_map->get_dual_key_value(task, this));
    } else {
        log << "Creating task specific SumEvaluator..." << endl;
        vector<shared_ptr<Evaluator>> td_subevaluators(subevaluators.size());
        transform(subevaluators.begin(), subevaluators.end(), td_subevaluators.begin(),
                  [this, &task, &component_map](const shared_ptr<TaskIndependentEvaluator> &eval) {
                      return eval->create_task_specific_Evaluator(task, component_map);
                  }
                  );
        task_specific_sum_evaluator = make_shared<SumEvaluator>(log, td_subevaluators, unparsed_config);
        component_map->add_dual_key_entry(task, this, plugins::Any(task_specific_sum_evaluator));
    }
    return task_specific_sum_evaluator;
}


shared_ptr<SumEvaluator> TaskIndependentSumEvaluator::create_task_specific_SumEvaluator(shared_ptr<AbstractTask> &task) {
    log << "Creating SumEvaluator as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_SumEvaluator(task, component_map);
}


shared_ptr<combining_evaluator::CombiningEvaluator> TaskIndependentSumEvaluator::create_task_specific_CombiningEvaluator(shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map) {
    shared_ptr<SumEvaluator> x = create_task_specific_SumEvaluator(task, component_map);
    return static_pointer_cast<combining_evaluator::CombiningEvaluator>(x);
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
                                                        opts.get<bool>("use_for_reporting_minima", false),
                                                        opts.get<bool>("use_for_boosting", false),
                                                        opts.get<bool>("use_for_counting_evaluations", false)
                                                        );
    }
};

static plugins::FeaturePlugin<SumEvaluatorFeature> _plugin;
}
