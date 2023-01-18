#include "null_pruning_method.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

using namespace std;

namespace null_pruning_method {
NullPruningMethod::NullPruningMethod(const plugins::Options &opts)
    : PruningMethod(opts) {
}

void NullPruningMethod::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    log << "pruning method: none" << endl;
}

class NullPruningMethodFeature : public plugins::TypedFeature<PruningMethod, NullPruningMethod> {
public:
    NullPruningMethodFeature() : TypedFeature("null") {
        // document_group("");
        document_title("No pruning");
        document_synopsis(
            "This is a skeleton method that does not perform any pruning, i.e., "
            "all applicable operators are applied in all expanded states. ");

        add_pruning_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<NullPruningMethodFeature> _plugin;
}
