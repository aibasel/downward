#include "g_evaluator.h"

#include "../evaluation_context.h"

#include "../plugins/plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluator::GEvaluator(const plugins::Options &opts)
    : Evaluator(opts) {
}

GEvaluator::GEvaluator(const string &name,
                       utils::Verbosity verbosity)
    : Evaluator(name, verbosity, false, false, false) {
}

EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_g_value());
    return result;
}



TaskIndependentGEvaluator::TaskIndependentGEvaluator(const string &name,
                                                     utils::Verbosity verbosity)
    : TaskIndependentEvaluator(name,
                               verbosity,
                               false,
                               false,
                               false){
}


shared_ptr<Evaluator> TaskIndependentGEvaluator::get_task_specific(
    [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) const {
    shared_ptr<GEvaluator> task_specific_x;

    if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific GEvaluator '" << name << "'..." << endl;
        task_specific_x = dynamic_pointer_cast<GEvaluator>(
            component_map->at(static_cast<const TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific GEvaluator '" << name << "'..." << endl;
        task_specific_x = create_ts(task, component_map, depth);
        component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>
                                  (static_cast<const TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}

    std::shared_ptr<GEvaluator> TaskIndependentGEvaluator::create_ts([[maybe_unused]] const shared_ptr <AbstractTask> &task,
                                                                     [[maybe_unused]] unique_ptr <ComponentMap> &component_map,
                                                                     [[maybe_unused]] int depth) const {
        return make_shared<GEvaluator>(name, verbosity);
    }


    class TaskIndependentGEvaluatorFeature : public plugins::TypedFeature<TaskIndependentEvaluator, TaskIndependentGEvaluator> {
public:
    TaskIndependentGEvaluatorFeature() : TypedFeature("g") {
        document_subcategory("evaluators_basic");
        document_title("g-value evaluator");
        document_synopsis(
            "Returns the g-value (path cost) of the search node.");
        add_evaluator_options_to_feature(*this, "g_eval");
    }

    virtual shared_ptr<TaskIndependentGEvaluator> create_component(
        const plugins::Options &opts, const utils::Context &) const override {
        return make_shared<TaskIndependentGEvaluator>(opts.get<string>("name"),
                                                      opts.get<utils::Verbosity>("verbosity"));
    }
};

static plugins::FeaturePlugin<TaskIndependentGEvaluatorFeature> _plugin;
}
