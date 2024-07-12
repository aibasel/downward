#include "null_pruning_method.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

using namespace std;

namespace null_pruning_method {
NullPruningMethod::NullPruningMethod(utils::Verbosity verbosity)
    : PruningMethod(verbosity) {
}

void NullPruningMethod::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    log << "pruning method: none" << endl;
}

class NullPruningMethodFeature
    : public plugins::TypedFeature<PruningMethod, NullPruningMethod> {
public:
    NullPruningMethodFeature() : TypedFeature("null") {
        // document_group("");
        document_title("No pruning");
        document_synopsis(
            "This is a skeleton method that does not perform any pruning, i.e., "
            "all applicable operators are applied in all expanded states. ");

        add_pruning_options_to_feature(*this);
    }

    virtual shared_ptr<NullPruningMethod> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<NullPruningMethod>(
            get_pruning_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<NullPruningMethodFeature> _plugin;
}
