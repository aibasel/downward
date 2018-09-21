#include "registries.h"

#include <iostream>
#include <unordered_set>

using namespace std;
using utils::ExitCode;

namespace options {

void RegistryDataCollection::insert_plugin_type_data(const string& type_name, 
    const string& documentation, type_index type_index) {
    plugin_types.emplace_back(type_name, documentation, type_index);
}

void RegistryDataCollection::insert_plugin_group_data(const string& group_id,
    const string& doc_title) {
    plugin_groups.emplace_back(group_id, doc_title);
}

void RegistryDataCollection::insert_plugin_data(
    const string& key, Any factory, const string& group, 
    PluginTypeNameGetter type_name_factory, DocFactory doc_factory,
    type_index type_index) {
    plugins.emplace_back(key, factory, group, type_name_factory, doc_factory,
        type_index);
}

const vector<PluginTypeData>& RegistryDataCollection::get_plugin_type_data() const{
    return plugin_types;
}

const vector<PluginGroupData>& RegistryDataCollection::get_plugin_group_data() const {
    return plugin_groups;
}

const vector<PluginData>& RegistryDataCollection::get_plugin_data() const {
    return plugins;
}


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

void Registry::insert_type_info(const PluginTypeInfo &info) {
    if (plugin_type_infos.count(info.get_type())) {
        cerr << "duplicate type in registry: "
             << info.get_type().name() << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
    plugin_type_infos.insert(make_pair(info.get_type(), info));
}

const PluginTypeInfo &Registry::get_type_info(const type_index &type) const {
    if (!plugin_type_infos.count(type)) {
        ABORT("attempt to retrieve non-existing type info from registry: " +
              string(type.name()));
    }
    return plugin_type_infos.at(type);
}

vector<PluginTypeInfo> Registry::get_sorted_type_infos() const {
    vector<PluginTypeInfo> types;
    for (auto it : plugin_type_infos) {
        types.push_back(it.second);
    }
    sort(types.begin(), types.end());
    return types;
}

void Registry::insert_group_info(const PluginGroupInfo &info) {
    if (plugin_group_infos.count(info.group_id)) {
        cerr << "duplicate group in registry: "
             << info.group_id << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
    plugin_group_infos[info.group_id] = info;
}

const PluginGroupInfo &Registry::get_group_info(const string &group) const {
    if (!plugin_group_infos.count(group)) {
        ABORT("attempt to retrieve non-existing group info from registry: " +
              string(group));
    }
    return plugin_group_infos.at(group);
}

Registry::Registry(const RegistryDataCollection& collection) {
    vector<string> errors;
    collect_plugin_types(collection, errors);
    collect_plugin_groups(collection, errors);
    collect_plugins(collection, errors);
}

void Registry::collect_plugin_types(const RegistryDataCollection& collection,
    vector<string> &/*errors*/) {
    for (PluginTypeData ptd : collection.get_plugin_type_data()) {
        const string &type_name = std::get<0>(ptd);
        const string &documentation = std::get<1>(ptd);
        const type_index type = std::get<2>(ptd);
        cout << "START" << endl;
        //cout << "TYPE" << type << endl;
        cout << "NAME " << type_name << endl;
        cout << "DOC" << documentation << endl;
        PluginTypeInfo info(type, type_name, documentation);
        insert_type_info(info);
    }
}

void Registry::collect_plugin_groups(const RegistryDataCollection& collection,
    vector<string> &/*errors*/) {
    for (PluginGroupData pgd : collection.get_plugin_group_data()) {
        const string &group_id = std::get<0>(pgd);
        const string &doc_title = std::get<1>(pgd);
        PluginGroupInfo info {group_id, doc_title};
        insert_group_info(info);
    }
}

void Registry::collect_plugins(const RegistryDataCollection& collection,
    vector<string> &/*errors*/) {
    for (PluginData pd : collection.get_plugin_data()) {
        const string &key = std::get<0>(pd);
        const Any &factory = std::get<1>(pd);
        const string &group = std::get<2>(pd);
        const PluginTypeNameGetter &type_name_factory = std::get<3>(pd);
        const DocFactory &doc_factory = std::get<4>(pd);
        type_index type = std::get<5>(pd);
        
        insert_plugin(key, factory, type_name_factory, doc_factory, group, 
            type);
    }
}

void Registry::insert_plugin(const std::string &key, Any factory,
        PluginTypeNameGetter type_name_factory, DocFactory doc_factory,
        const std::string &group, const std::type_index type) {
        //using TPtr = std::shared_ptr<T>;
        insert_plugin_info(key, doc_factory, type_name_factory, group);
        insert_factory(key, factory, type);
    }

void Registry::insert_factory(const std::string &key, Any factory,
    std::type_index type) {
    if (plugin_factories.count(type) && plugin_factories[type].count(key)) {
        std::cerr << "duplicate key in registry: " << key << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    plugin_factories[type][key] = factory;
    }

void Registry::insert_plugin_info(
    const string &key,
    DocFactory doc_factory,
    PluginTypeNameGetter type_name_factory,
    const string &group) {
    if (plugin_infos.count(key)) {
        ABORT("Registry already contains a plugin with name \"" + key + "\"");
    }
    PluginInfo doc;
    doc.doc_factory = doc_factory;
    doc.type_name_factory = type_name_factory;
    doc.key = key;
    // Plugin names can be set with document_synopsis. Otherwise, we use the key.
    doc.name = key;
    doc.synopsis = "";
    doc.group = group;
    doc.hidden = false;
    plugin_infos[key] = doc;
}

void Registry::add_plugin_info_arg(
    const string &key,
    const string &arg_name,
    const string &help,
    const string &type_name,
    const string &default_value,
    const Bounds &bounds,
    const ValueExplanations &value_explanations) {
    get_plugin_info(key).arg_help.emplace_back(
        arg_name, help, type_name, default_value, bounds, value_explanations);
}

void Registry::set_plugin_info_synopsis(
    const string &key, const string &name, const string &description) {
    get_plugin_info(key).name = name;
    get_plugin_info(key).synopsis = description;
}

void Registry::add_plugin_info_property(
    const string &key, const string &name, const string &description) {
    get_plugin_info(key).property_help.emplace_back(name, description);
}

void Registry::add_plugin_info_feature(
    const string &key, const string &feature, const string &description) {
    get_plugin_info(key).support_help.emplace_back(feature, description);
}

void Registry::add_plugin_info_note(
    const string &key, const string &name, const string &description, bool long_text) {
    get_plugin_info(key).notes.emplace_back(name, description, long_text);
}

PluginInfo &Registry::get_plugin_info(const string &key) {
    /* Use at() to get an error when trying to modify a plugin that has not been
       registered with insert_plugin_info. */
    return plugin_infos.at(key);
}

vector<string> Registry::get_sorted_plugin_info_keys() {
    vector<string> keys;
    for (const auto &it : plugin_infos) {
        keys.push_back(it.first);
    }
    sort(keys.begin(), keys.end());
    return keys;
}
}
