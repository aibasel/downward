#include "null_pruning_method.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace null_pruning_method {
NullPruningMethod::NullPruningMethod() {
    cout << "pruning method: none" << endl;
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "No pruning",
        "pruning method without pruning");

    if (parser.dry_run()) {
	return nullptr;
    }
    
    return make_shared<NullPruningMethod>();
}

static PluginShared<PruningMethod> _plugin("null", _parse);
}
