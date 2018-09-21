#include "doc_utils.h"

#include "option_parser.h"
#include "predefinitions.h"

#include <algorithm>

using namespace std;

namespace options {
void PluginInfo::fill_docs(Registry &registry) {
    OptionParser parser(key, registry, Predefinitions(), true, true);
    doc_factory(parser);
}

string PluginInfo::get_type_name() const {
    return type_name_factory();
}
}
