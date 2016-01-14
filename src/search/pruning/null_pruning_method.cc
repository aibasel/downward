#include "null_pruning_method.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace null_pruning_method {
void NullPruningMethod::initialize() {
    cout << "pruning method: none" << endl;
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "No pruning",
        "pruning method without pruning");

    return make_shared<NullPruningMethod>();
}

static PluginShared<PruningMethod> _plugin("null", _parse);
}
