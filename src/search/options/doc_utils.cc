#include "doc_utils.h"

#include "option_parser.h"
#include "predefinitions.h"

using namespace std;

namespace options {
PluginTypeInfo::PluginTypeInfo(const type_index &type,
                               const string &type_name,
                               const string &documentation,
                               const PredefinitionConfig &predefine)
    : type(type),
      type_name(type_name),
      documentation(documentation),
      predefine(predefine) {
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

const PredefinitionConfig &PluginTypeInfo::get_predefine() const {
    return predefine;
}

bool PluginTypeInfo::operator<(const PluginTypeInfo &other) const {
    return make_pair(type_name, type) < make_pair(other.type_name, other.type);
}
}
