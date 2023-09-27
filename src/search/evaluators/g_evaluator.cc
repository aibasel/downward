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


shared_ptr<GEvaluator> TaskIndependentGEvaluator::create_task_specific_GEvaluator(std::shared_ptr<AbstractTask> &task,
                                                                                  std::shared_ptr<ComponentMap> &component_map) {
    shared_ptr<GEvaluator> task_specific_g_evaluator;

    if (component_map->contains_key(make_pair(task, static_cast<void *>(this)))) {
        log << "Reuse task specific GEvaluator..." << endl;
        task_specific_g_evaluator = plugins::any_cast<shared_ptr<GEvaluator>>(
            component_map->get_dual_key_value(task, this));
    } else {
        log << "Creating task specific GEvaluator..." << endl;
        task_specific_g_evaluator = make_shared<GEvaluator>(log, unparsed_config);
        component_map->add_dual_key_entry(task, this, plugins::Any(task_specific_g_evaluator));
    }
    return task_specific_g_evaluator;
}


shared_ptr<GEvaluator> TaskIndependentGEvaluator::create_task_specific_GEvaluator(shared_ptr<AbstractTask> &task) {
    log << "Creating GEvaluator as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_GEvaluator(task, component_map);
}


shared_ptr<Evaluator> TaskIndependentGEvaluator::create_task_specific_Evaluator(shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map) {
    shared_ptr<GEvaluator> x = create_task_specific_GEvaluator(task, component_map);
    return static_pointer_cast<Evaluator>(x);
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
