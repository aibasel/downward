#ifndef OPTIONS_REGISTRIES_H
#define OPTIONS_REGISTRIES_H

#include "../utils/system.h"

#include "any.h"

#include <algorithm>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace options {
class OptionParser;


/*
  The plugin type info class contains meta-information for a given
  type of plugins (e.g. "SearchEngine" or "MergeStrategyFactory").
*/
class PluginTypeInfo {
    std::type_index type;

    /*
      The type name should be "user-friendly". It is for example used
      as the name of the wiki page that documents this plugin type.
      It follows wiki conventions (e.g. "Heuristic", "SearchEngine",
      "ShrinkStrategy").
    */
    std::string type_name;

    /*
      General documentation for the plugin type. This is included at
      the top of the wiki page for this plugin type.
    */
    std::string documentation;
public:
    PluginTypeInfo(const std::type_index &type,
                   const std::string &type_name,
                   const std::string &documentation);

    ~PluginTypeInfo() = default;

    const std::type_index &get_type() const;
    const std::string &get_type_name() const;
    const std::string &get_documentation() const;

    bool operator<(const PluginTypeInfo &other) const;
};


struct PluginGroupInfo {
    std::string group_id;
    std::string doc_title;
};


class Registry {
private:
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
    bool contains_factory(const std::string &key) const {
        std::type_index type(typeid(T));
        return plugin_factories.count(type)
               && plugin_factories.at(type).count(key);
    }

    template<typename T>
    std::function<T(OptionParser &)> get_factory(const std::string &key) const {
        std::type_index type(typeid(T));
        return any_cast<std::function<T(OptionParser &)>>(plugin_factories.at(type).at(key));
    }

    template<typename T>
    std::vector<std::string> get_sorted_factory_keys() const {
        std::type_index type(typeid(T));
        std::vector<std::string> keys;
        for (auto it : plugin_factories.at(type)) {
            keys.push_back(it.first);
        }
        sort(keys.begin(), keys.end());
        return keys;
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
