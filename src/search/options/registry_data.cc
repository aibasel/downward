#include "registry_data.h"

using namespace std;

namespace options {
PluginTypeData::PluginTypeData(const string type_name,
                               const string documentation,
                               const type_index type)
    : type_name(type_name),
      documentation(documentation),
      type(type) { }

PluginGroupData::PluginGroupData(const string group_id,
                                 const string doc_title)
    : group_id(group_id),
      doc_title(doc_title) { }

PluginData::PluginData(const string key,
                       const Any factory,
                       const string group,
                       const PluginTypeNameGetter type_name_factory,
                       const DocFactory doc_factory,
                       const type_index type)
    : key(key),
      factory(factory),
      group(group),
      type_name_factory(type_name_factory),
      doc_factory(doc_factory),
      type(type) { }




void RegistryData::insert_plugin_type_data(
    const string &type_name, const string &documentation, type_index type) {
    plugin_types.emplace_back(type_name, documentation, type);
}

void RegistryData::insert_plugin_group_data(
    const string &group_id, const string &doc_title) {
    plugin_groups.emplace_back(group_id, doc_title);
}

void RegistryData::insert_plugin_data(
    const string &key, const Any &factory, const string &group,
    PluginTypeNameGetter type_name_factory, DocFactory doc_factory,
    type_index type) {
    plugins.emplace_back(key, factory, group, type_name_factory, doc_factory,
                         type);
}

const vector<PluginTypeData> &RegistryData::get_plugin_type_data() const {
    return plugin_types;
}

const vector<PluginGroupData> &RegistryData::get_plugin_group_data() const {
    return plugin_groups;
}

const vector<PluginData> &RegistryData::get_plugin_data() const {
    return plugins;
}
}
