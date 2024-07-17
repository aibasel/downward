#include "null_pruning_method.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

using namespace std;

namespace null_pruning_method {
NullPruningMethod::NullPruningMethod(const utils::Verbosity verbosity)
    : PruningMethod(verbosity) {
}

void NullPruningMethod::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    log << "pruning method: none" << endl;
}

TaskIndependentNullPruningMethod::TaskIndependentNullPruningMethod(const std::string &name, utils::Verbosity verbosity)
    : TaskIndependentPruningMethod(name, verbosity) {}

shared_ptr<PruningMethod> TaskIndependentNullPruningMethod::create_task_specific(
    [[maybe_unused]] const shared_ptr <AbstractTask> &task,
    [[maybe_unused]] unique_ptr <ComponentMap> &component_map,
    [[maybe_unused]] int depth) const {
    return make_shared<NullPruningMethod>(verbosity);
}

class TaskIndependentNullPruningMethodFeature : public plugins::TypedFeature<TaskIndependentPruningMethod, TaskIndependentNullPruningMethod> {
public:
    TaskIndependentNullPruningMethodFeature() : TypedFeature("null") {
        document_title("No pruning");
        document_synopsis(
            "This is a skeleton method that does not perform any pruning, i.e., "
            "all applicable operators are applied in all expanded states. ");
        add_pruning_options_to_feature(*this, "null_pruning");
    }

    virtual shared_ptr<TaskIndependentNullPruningMethod> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<TaskIndependentNullPruningMethod>(
            get_pruning_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<TaskIndependentNullPruningMethodFeature> _plugin;
}
