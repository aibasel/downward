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
    const PredefinitionFunctional &predefine_functional)
    : type(type),
      type_name(type_name),
      documentation(documentation),
      predefine(predefine),
      alias(alias),
      predefine_functional(predefine_functional) {
}

bool PluginTypeInfo::operator<(const PluginTypeInfo &other) const {
    return make_pair(type_name, type) < make_pair(other.type_name, other.type);
}
}
