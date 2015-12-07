#include "null_por_method.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <iostream>

using namespace std;

namespace NullPORMethod {

NullPORMethod::NullPORMethod() {}

NullPORMethod::~NullPORMethod() {}

void NullPORMethod::dump_options() const {
    cout << "partial order reduction method: none" << endl;
}

static shared_ptr<PORMethod> _parse(OptionParser &parser) {
    parser.document_synopsis("No pruning", "por method without pruning");

    return make_shared<NullPORMethod>();
}

static PluginShared<PORMethod> _plugin("null", _parse);
}
