#include "plugin.h"

#include <typeindex>

using namespace std;

namespace options {
void register_plugin_type_plugin(
    const type_info &type, const string &type_name, const string &documentation) {
    PluginTypeInfo info(type_index(type), type_name, documentation);
    PluginTypeRegistry::instance()->insert(info);
}

PluginGroupPlugin::PluginGroupPlugin(const string &group_id,
                                     const string &doc_title) {
    PluginGroupInfo info {group_id, doc_title};
    PluginGroupRegistry::instance()->insert(info);
}
}
