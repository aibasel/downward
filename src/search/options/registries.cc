#include "registries.h"

using namespace std;
using utils::ExitCode;

namespace options {
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
