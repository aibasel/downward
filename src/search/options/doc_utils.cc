#include "doc_utils.h"

#include "option_parser.h"
#include "predefinitions.h"

#include <algorithm>

using namespace std;

namespace options {
void PluginInfo::fill_docs() {
    OptionParser parser(key, Predefinitions(), true, true);
    doc_factory(parser);
}

string PluginInfo::get_type_name() const {
    return type_name_factory();
}
}
