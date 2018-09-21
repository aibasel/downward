#ifndef OPTIONS_REGISTRIES_H
#define OPTIONS_REGISTRIES_H

#include "any.h"
#include "doc_utils.h"

#include "../utils/system.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>


namespace options {
class OptionParser;

using PluginTypeData = std::tuple<std::string /*type_name*/, std::string /*documentation*/, std::type_index>;
using PluginGroupData = std::tuple<std::string /*group_id*/, std::string /*doc_title*/>;
using PluginData = std::tuple<std::string /*key*/, Any /*factory*/, std::string /*group*/, PluginTypeNameGetter /*type_name_factory*/, DocFactory /*doc_factory*/, std::type_index>;
    
class RegistryDataCollection {
    
    std::vector<PluginTypeData> plugin_types;
    std::vector<PluginGroupData> plugin_groups;
    std::vector<PluginData> plugins;
    
    RegistryDataCollection() = default;
    
public:
    void insert_plugin_type_data(const std::string &type_name,
        const std::string &documentation, std::type_index type_index);
    
    void insert_plugin_group_data(const std::string &group_id,
        const std::string &doc_title);
    
    void insert_plugin_data(const std::string &key,
        Any factory,
        const std::string &group,
        PluginTypeNameGetter type_name_factory,
        DocFactory doc_factory,
        std::type_index type_index);
    
    const std::vector<PluginTypeData> &get_plugin_type_data() const;
    const std::vector<PluginGroupData> &get_plugin_group_data() const;
    const std::vector<PluginData> &get_plugin_data() const;
    
    static RegistryDataCollection *instance() {
        static RegistryDataCollection instance_;
        return &instance_;
    }
};


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
    
    void collect_plugin_types(const RegistryDataCollection &collection,
        std::vector<std::string> &errors);
    void collect_plugin_groups(const RegistryDataCollection &collection,
        std::vector<std::string> &errors);
    void collect_plugins(const RegistryDataCollection &collection,
        std::vector<std::string> &errors);
    
    void insert_plugin(const std::string &key, Any factory,
        PluginTypeNameGetter type_name_factory, DocFactory doc_factory,
        const std::string &group, const std::type_index type);
    void insert_factory(
        const std::string &key,
        Any factory,
        const std::type_index type);
    void insert_plugin_info(
        const std::string &key,
        DocFactory factory,
        PluginTypeNameGetter type_name_factory,
        const std::string &group);
    void insert_type_info(const PluginTypeInfo &info);
    void insert_group_info(const PluginGroupInfo &info);
    
public:
    Registry(const RegistryDataCollection &collection);
    
    template<typename T>
    std::function<T(OptionParser &)> get_factory(const std::string &key) const {
        std::type_index type(typeid(T));
        return any_cast<std::function<T(OptionParser &)>>(plugin_factories.at(type).at(key));
    }

    const PluginTypeInfo &get_type_info(const std::type_index &type) const;
    std::vector<PluginTypeInfo> get_sorted_type_infos() const;

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
};
}

#endif
