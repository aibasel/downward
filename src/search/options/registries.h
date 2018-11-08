#ifndef OPTIONS_REGISTRIES_H
#define OPTIONS_REGISTRIES_H

#include "any.h"
#include "doc_utils.h"

#include "../utils/system.h"

#include <algorithm>
#include <functional>
#include <memory>
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
    /*
       plugin_infos collects the information about all plugins. This is used,
       for example, to generate the documentation.
     */
    std::unordered_map<std::string, PluginInfo> plugin_infos;
    Registry() = default;

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

    void insert_plugin_info(
        const std::string &key,
        DocFactory factory,
        PluginTypeNameGetter type_name_factory,
        const std::string &group);
public:

    template<typename T>
    void insert_plugin(const std::string &key,
                       std::function<std::shared_ptr<T>(OptionParser &)> factory,
                       PluginTypeNameGetter type_name_factory, const std::string &group) {
        using TPtr = std::shared_ptr<T>;
        /*
          We cannot collect the plugin documentation here because this might
          require information from a TypePlugin object that has not yet been
          constructed. We therefore collect the necessary functions here and
          call them later, after all PluginType objects have been constructed.
        */
        DocFactory doc_factory = [factory](OptionParser &parser) {
                factory(parser);
            };
        insert_plugin_info(key, doc_factory, type_name_factory, group);
        insert_factory<TPtr>(key, factory);
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


    PluginInfo &get_plugin_info(const std::string &key);

    void add_plugin_info_arg(
        const std::string &key,
        const std::string &arg_name,
        const std::string &help,
        const std::string &type_name,
        const std::string &default_value,
        const Bounds &bounds,
        const ValueExplanations &value_explanations = ValueExplanations());

    void set_plugin_info_synopsis(
        const std::string &key, const std::string &name, const std::string &description);

    void add_plugin_info_property(
        const std::string &key, const std::string &name, const std::string &description);

    void add_plugin_info_feature(
        const std::string &key, const std::string &feature, const std::string &description);

    void add_plugin_info_note(
        const std::string &key,
        const std::string &name,
        const std::string &description,
        bool long_text);

    std::vector<std::string> get_sorted_plugin_info_keys();


    static Registry *instance() {
        static Registry instance_;
        return &instance_;
    }
};
}

#endif
