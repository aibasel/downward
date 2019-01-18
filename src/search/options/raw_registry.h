#ifndef OPTIONS_RAW_REGISTRY_H
#define OPTIONS_RAW_REGISTRY_H

#include "any.h"
#include "doc_utils.h"

#include <vector>


namespace options {
struct RawPluginInfo {
    std::string key;
    Any factory;
    std::string group;
    PluginTypeNameGetter type_name_factory;
    DocFactory doc_factory;
    std::type_index type;

    RawPluginInfo(
        const std::string &key,
        const Any &factory,
        const std::string &group,
        const PluginTypeNameGetter &type_name_factory,
        const DocFactory &doc_factory,
        const std::type_index &type);
};


class RawRegistry {
    std::vector<PluginTypeInfo> plugin_types;
    std::vector<PluginGroupInfo> plugin_groups;
    std::vector<RawPluginInfo> plugins;

public:
    void insert_plugin_type_data(
        std::type_index type, const std::string &type_name,
        const std::string &documentation, const std::string &predefinition_key,
        const std::string &alias,
        const PredefinitionFunction &predefinition_function);

    void insert_plugin_group_data(
        const std::string &group_id, const std::string &doc_title);

    void insert_plugin_data(
        const std::string &key,
        const Any &factory,
        const std::string &group,
        PluginTypeNameGetter &type_name_factory,
        DocFactory &doc_factory,
        std::type_index &type);

    const std::vector<PluginTypeInfo> &get_plugin_type_data() const;
    const std::vector<PluginGroupInfo> &get_plugin_group_data() const;
    const std::vector<RawPluginInfo> &get_plugin_data() const;

    static RawRegistry *instance() {
        static RawRegistry instance_;
        return &instance_;
    }
};
}

#endif
