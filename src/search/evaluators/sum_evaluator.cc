#include "sum_evaluator.h"

#include "../plugins/plugin.h"

#include <cassert>
#include <utility>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(const plugins::Options &opts)
    : CombiningEvaluator(opts) {
}


SumEvaluator::SumEvaluator(const string &name,
                           utils::LogProxy log,
                           vector<shared_ptr<Evaluator>> subevaluators,
                           basic_string<char> unparsed_config,
                           bool use_for_reporting_minima,
                           bool use_for_boosting,
                           bool use_for_counting_evaluations)
    : CombiningEvaluator(name,
                         log,
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

TaskIndependentSumEvaluator::TaskIndependentSumEvaluator(const string &name,
                                                         utils::LogProxy log,
                                                         std::vector<std::shared_ptr<
                                                                         TaskIndependentEvaluator>> subevaluators,
                                                         std::basic_string<char> unparsed_config,
                                                         bool use_for_reporting_minima,
                                                         bool use_for_boosting,
                                                         bool use_for_counting_evaluations)
    : TaskIndependentCombiningEvaluator(name,
                                        log,
                                        subevaluators,
                                        unparsed_config,
                                        use_for_reporting_minima,
                                        use_for_boosting,
                                        use_for_counting_evaluations) {
}


shared_ptr<Evaluator> TaskIndependentSumEvaluator::create_task_specific(const shared_ptr<AbstractTask> &task,
                                                                        std::unique_ptr<ComponentMap> &component_map,
                                                                        int depth) const {
    shared_ptr<SumEvaluator> task_specific_x;
    if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific SumEvaluator '" << name << "'..." << endl;
        task_specific_x = dynamic_pointer_cast<SumEvaluator>(
            component_map->at(static_cast<const TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific SumEvaluator '" << name << "'..." << endl;
        vector<shared_ptr<Evaluator>> td_subevaluators(subevaluators.size());
        transform(subevaluators.begin(), subevaluators.end(), td_subevaluators.begin(),
                  [this, &task, &component_map, &depth](const shared_ptr<TaskIndependentEvaluator> &eval) {
                      return eval->create_task_specific(task, component_map, depth >= 0 ? depth + 1 : depth);
                  }
                  );
        task_specific_x = make_shared<SumEvaluator>(name, log, td_subevaluators, unparsed_config);
        component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>(
                                  static_cast<const TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}


class SumEvaluatorFeature : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentSumEvaluator> {
public:
    SumEvaluatorFeature() : TypedFeature("sum") {
        document_subcategory("evaluators_basic");
        document_title("Sum evaluator");
        document_synopsis("Calculates the sum of the sub-evaluators.");

        combining_evaluator::add_combining_evaluator_options_to_feature(*this, "sum_eval");
    }

    virtual shared_ptr<TaskIndependentSumEvaluator> create_component(
        const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentEvaluator>>(context, opts, "evals");
        return make_shared<TaskIndependentSumEvaluator>(opts.get<string>("name"),
                                                        utils::get_log_from_options(opts),
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
