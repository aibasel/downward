#include "null_pruning_method.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace null_pruning_method {
void NullPruningMethod::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    cout << "pruning method: none" << endl;
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "No pruning",
        "This is a skeleton method that does not perform any pruning, i.e., "
        "all applicable operators are applied in all expanded states. ");

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<NullPruningMethod>();
}

static PluginShared<PruningMethod> _plugin("null", _parse);
}
