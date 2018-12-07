#include "doc_utils.h"

#include "option_parser.h"
#include "predefinitions.h"

using namespace std;

namespace options {
PluginTypeInfo::PluginTypeInfo(
    const type_index &type,
    const string &type_name,
    const string &documentation,
    const string &predefine,
    const string &alias,
    const PredefinitionFunction &predefinition_function)
    : type(type),
      type_name(type_name),
      documentation(documentation),
      predefine(predefine),
      alias(alias),
      predefinition_function(predefinition_function) {
}

bool PluginTypeInfo::operator<(const PluginTypeInfo &other) const {
    return make_pair(type_name, type) < make_pair(other.type_name, other.type);
}
}
