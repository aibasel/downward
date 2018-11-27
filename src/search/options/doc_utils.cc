#include "doc_utils.h"

#include "option_parser.h"
#include "predefinitions.h"

using namespace std;

namespace options {
void PluginInfo::fill_docs() {
    OptionParser parser(key, registry, Predefinitions(), true, true);
    doc_factory(parser);
}

string PluginInfo::get_type_name() const {
    return type_name_factory(registry);
}

PluginTypeInfo::PluginTypeInfo(const type_index &type,
                               const string &type_name,
                               const string &documentation)
    : type(type),
      type_name(type_name),
      documentation(documentation) {
}

const type_index &PluginTypeInfo::get_type() const {
    return type;
}

const string &PluginTypeInfo::get_type_name() const {
    return type_name;
}

const string &PluginTypeInfo::get_documentation() const {
    return documentation;
}

bool PluginTypeInfo::operator<(const PluginTypeInfo &other) const {
    return make_pair(type_name, type) < make_pair(other.type_name, other.type);
}
}
