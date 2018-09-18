#include "registries.h"

using namespace std;
using utils::ExitCode;

namespace options {
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

PluginTypeRegistry *PluginTypeRegistry::instance() {
    static PluginTypeRegistry the_instance;
    return &the_instance;
}

void PluginTypeRegistry::insert(const PluginTypeInfo &info) {
    if (registry.count(info.get_type())) {
        cerr << "duplicate type in registry: "
             << info.get_type().name() << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
    registry.insert(make_pair(info.get_type(), info));
}

const PluginTypeInfo &PluginTypeRegistry::get(const type_index &type) const {
    if (!registry.count(type)) {
        ABORT("attempt to retrieve non-existing type info from registry: " +
              string(type.name()));
    }
    return registry.at(type);
}

vector<PluginTypeInfo> PluginTypeRegistry::get_sorted_types() const {
    vector<PluginTypeInfo> types;
    for (auto it : registry) {
        types.push_back(it.second);
    }
    sort(types.begin(), types.end());
    return types;
}

PluginGroupRegistry *PluginGroupRegistry::instance() {
    static PluginGroupRegistry the_instance;
    return &the_instance;
}

void PluginGroupRegistry::insert(const PluginGroupInfo &info) {
    if (registry.count(info.group_id)) {
        cerr << "duplicate group in registry: "
             << info.group_id << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
    registry[info.group_id] = info;
}

const PluginGroupInfo &PluginGroupRegistry::get(
    const string &group) const {
    if (!registry.count(group)) {
        ABORT("attempt to retrieve non-existing group info from registry: " +
              string(group));
    }
    return registry.at(group);
}
}
