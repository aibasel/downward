#include "null_pruning_method.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

using namespace std;

namespace null_pruning_method {
NullPruningMethod::NullPruningMethod(
    const shared_ptr<AbstractTask> &task, utils::Verbosity verbosity)
    : PruningMethod(task, verbosity) {
}

void NullPruningMethod::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    log << "pruning method: none" << endl;
}

class NullPruningMethodFeature
    : public plugins::TaskIndependentFeature<TaskIndependentPruningMethod> {
public:
    NullPruningMethodFeature() : TaskIndependentFeature("null") {
        // document_group("");
        document_title("No pruning");
        document_synopsis(
            "This is a skeleton method that does not perform any pruning, i.e., "
            "all applicable operators are applied in all expanded states. ");

        add_pruning_options_to_feature(*this);
    }

    virtual shared_ptr<TaskIndependentPruningMethod> create_component(
        const plugins::Options &opts) const override {
        return components::make_shared_component<NullPruningMethod, PruningMethod>(
            get_pruning_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<NullPruningMethodFeature> _plugin;
}
