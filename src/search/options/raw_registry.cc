#include "raw_registry.h"

using namespace std;

namespace options {
RawPluginInfo::RawPluginInfo(
    const string &key,
    const Any &factory,
    const string &group,
    const PluginTypeNameGetter &type_name_factory,
    const DocFactory &doc_factory,
    const type_index &type)
    : key(key),
      factory(factory),
      group(group),
      type_name_factory(type_name_factory),
      doc_factory(doc_factory),
      type(type) {
}

void RawRegistry::insert_plugin_type_data(
    type_index type, const string &type_name, const string &documentation,
    const string &predefinition_key, const string &alias,
    const PredefinitionFunction &predefinition_function) {
    plugin_types.emplace_back(type, type_name, documentation, predefinition_key,
                              alias, predefinition_function);
}

void RawRegistry::insert_plugin_group_data(
    const string &group_id, const string &doc_title) {
    plugin_groups.emplace_back(group_id, doc_title);
}

void RawRegistry::insert_plugin_data(
    const string &key,
    const Any &factory,
    const string &group,
    PluginTypeNameGetter &type_name_factory,
    DocFactory &doc_factory,
    type_index &type) {
    plugins.emplace_back(key, factory, group, type_name_factory, doc_factory,
                         type);
}

const vector<PluginTypeInfo> &RawRegistry::get_plugin_type_data() const {
    return plugin_types;
}

const vector<PluginGroupInfo> &RawRegistry::get_plugin_group_data() const {
    return plugin_groups;
}

const vector<RawPluginInfo> &RawRegistry::get_plugin_data() const {
    return plugins;
}
}
