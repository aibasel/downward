#include "g_evaluator.h"

#include "../evaluation_context.h"

#include "../plugins/plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluator::GEvaluator(const plugins::Options &opts)
    : Evaluator(opts) {
}

GEvaluator::GEvaluator(utils::LogProxy log,
                       basic_string<char> unparsed_config,
                       bool use_for_reporting_minima,
                       bool use_for_boosting,
                       bool use_for_counting_evaluations)
    : Evaluator(log, unparsed_config, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations) {
}

EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_g_value());
    return result;
}



TaskIndependentGEvaluator::TaskIndependentGEvaluator(utils::LogProxy log,
                                                     std::basic_string<char> unparsed_config,
                                                     bool use_for_reporting_minima,
                                                     bool use_for_boosting,
                                                     bool use_for_counting_evaluations)
    : TaskIndependentEvaluator(log, unparsed_config, use_for_reporting_minima, use_for_boosting, use_for_counting_evaluations),
      unparsed_config(unparsed_config), log(log) {
}

TaskIndependentGEvaluator::~TaskIndependentGEvaluator() {
}


shared_ptr<Evaluator> TaskIndependentGEvaluator::create_task_specific(const std::shared_ptr<AbstractTask> &task,
                                                                                  std::unique_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<GEvaluator> task_specific_x;

    if (component_map->count( static_cast<TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific GEvaluator..." << endl;
        task_specific_x = dynamic_pointer_cast<GEvaluator>(
            component_map->at(static_cast<TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific GEvaluator..." << endl;
        task_specific_x = make_shared<GEvaluator>(log, unparsed_config);
        component_map->insert(make_pair<TaskIndependentComponent *, std::shared_ptr<Component>>(static_cast<TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}



class TaskIndependentGEvaluatorFeature : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentGEvaluator> {
public:
    TaskIndependentGEvaluatorFeature() : TypedFeature("g") {
        document_subcategory("evaluators_basic");
        document_title("g-value evaluator");
        document_synopsis(
            "Returns the g-value (path cost) of the search node.");
        add_evaluator_options_to_feature(*this);
    }

    virtual shared_ptr<TaskIndependentGEvaluator> create_component(
        const plugins::Options &opts, const utils::Context &) const override {
        return make_shared<TaskIndependentGEvaluator>(utils::get_log_from_options(opts),
                                                      opts.get_unparsed_config(),
                                                      opts.get<bool>("use_for_reporting_minima", false),
                                                      opts.get<bool>("use_for_boosting", false),
                                                      opts.get<bool>("use_for_counting_evaluations", false));
    }
};

static plugins::FeaturePlugin<TaskIndependentGEvaluatorFeature> _plugin;
}
