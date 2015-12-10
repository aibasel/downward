#include "registries.h"

using namespace std;
using Utils::ExitCode;


PluginTypeRegistry *PluginTypeRegistry::instance() {
    static PluginTypeRegistry the_instance;
    return &the_instance;
}

void PluginTypeRegistry::insert(const PluginTypeInfo &info) {
    if (registry.count(info.get_type())) {
        std::cerr << "duplicate type in registry: "
                  << info.get_type().name() << std::endl;
        Utils::exit_with(ExitCode::CRITICAL_ERROR);
    }
    registry.insert(make_pair(info.get_type(), info));
}

const PluginTypeInfo &PluginTypeRegistry::get(const type_index &type) const {
    return registry.at(type);
}
