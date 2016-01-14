#include "null_pruning_method.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace null_pruning_method {
void NullPORMethod::initialize() {
    cout << "partial order reduction method: none" << endl;
}

static shared_ptr<PORMethod> _parse(OptionParser &parser) {
    parser.document_synopsis("No pruning", "por method without pruning");

    return make_shared<NullPORMethod>();
}

static PluginShared<PORMethod> _plugin("null", _parse);
}
