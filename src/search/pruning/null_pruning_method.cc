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

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "No pruning",
        "This is a skeleton method that does not perform any pruning, i.e., "
        "all applicable operators are applied in all expanded states. ");
    add_pruning_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<NullPruningMethod>(opts);
}

static Plugin<PruningMethod> _plugin("null", _parse);
}
