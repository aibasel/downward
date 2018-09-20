#ifndef OPTIONS_REGISTRIES_H
#define OPTIONS_REGISTRIES_H

#include "../utils/system.h"

#include "any.h"
#include "doc_utils.h"

#include <algorithm>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>


namespace options {
class OptionParser;


class Registry {
    std::unordered_map<std::type_index, std::unordered_map<std::string, Any>> plugin_factories;
    /*
      plugin_type_infos collects information about all plugin types
      in use and gives access to the underlying information. This is used,
      for example, to generate the complete help output.
    */
    std::unordered_map<std::type_index, PluginTypeInfo> plugin_type_infos;
    /*
      The plugin group registry collects information about plugin groups.
      A plugin group is a set of plugins (which should be of the same
      type, although the code does not enforce this) that should be
      grouped together in user documentation.

      For example, all PDB heuristics could be grouped together so that
      the documention page for the heuristics looks nicer.
    */
    std::unordered_map<std::string, PluginGroupInfo> plugin_group_infos;

    Registry() = default;

public:
    template<typename T>
    void insert_factory(
        const std::string &key,
        std::function<T(OptionParser &)> factory) {
        std::type_index type(typeid(T));

        if (plugin_factories.count(type) && plugin_factories[type].count(key)) {
            std::cerr << "duplicate key in registry: " << key << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        plugin_factories[type][key] = factory;
    }

    template<typename T>
    std::function<T(OptionParser &)> get_factory(const std::string &key) const {
        std::type_index type(typeid(T));
        return any_cast<std::function<T(OptionParser &)>>(plugin_factories.at(type).at(key));
    }

    void insert_type_info(const PluginTypeInfo &info);
    const PluginTypeInfo &get_type_info(const std::type_index &type) const;
    std::vector<PluginTypeInfo> get_sorted_type_infos() const;

    void insert_group_info(const PluginGroupInfo &info);
    const PluginGroupInfo &get_group_info(const std::string &key) const;

    static Registry *instance() {
        static Registry instance_;
        return &instance_;
    }
};
}

#endif
