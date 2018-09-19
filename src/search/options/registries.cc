#include "registries.h"

using namespace std;
using utils::ExitCode;

namespace options {
void Registry::insert_type_info(const PluginTypeInfo &info) {
    if (plugin_type_infos.count(info.get_type())) {
        cerr << "duplicate type in registry: "
             << info.get_type().name() << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
    plugin_type_infos.insert(make_pair(info.get_type(), info));
}

const PluginTypeInfo &Registry::get_type_info(const type_index &type) const {
    if (!plugin_type_infos.count(type)) {
        ABORT("attempt to retrieve non-existing type info from registry: " +
              string(type.name()));
    }
    return plugin_type_infos.at(type);
}

std::vector<PluginTypeInfo> Registry::get_sorted_type_infos() const {
    std::vector<PluginTypeInfo> types;
    for (auto it : plugin_type_infos) {
        types.push_back(it.second);
    }
    sort(types.begin(), types.end());
    return types;
}

void Registry::insert_group_info(const PluginGroupInfo &info) {
    if (plugin_group_infos.count(info.group_id)) {
        cerr << "duplicate group in registry: "
             << info.group_id << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
    plugin_group_infos[info.group_id] = info;
}

const PluginGroupInfo &Registry::get_group_info(const string &group) const {
    if (!plugin_group_infos.count(group)) {
        ABORT("attempt to retrieve non-existing group info from registry: " +
              string(group));
    }
    return plugin_group_infos.at(group);
}
}
