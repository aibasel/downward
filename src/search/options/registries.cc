#include "registries.h"

#include "errors.h"
#include "option_parser.h"
#include "predefinitions.h"

#include "../utils/collections.h"
#include "../utils/strings.h"

#include <iostream>
#include <typeindex>

using namespace std;

namespace options {
static void print_initialization_errors_and_exit(const vector<string> &errors) {
    throw OptionParserError("\n" + utils::join(errors, "\n") + "\n" + get_demangling_hint("[TYPE]"));
}


Registry::Registry(const RawRegistry &raw_registry) {
    vector<string> errors;
    insert_plugin_types(raw_registry, errors);
    insert_plugin_groups(raw_registry, errors);
    insert_plugins(raw_registry, errors);
    if (!errors.empty()) {
        sort(errors.begin(), errors.end());
        print_initialization_errors_and_exit(errors);
    }
    // The documentation generation requires an error free, fully initialized registry.
    for (const RawPluginInfo &plugin : raw_registry.get_plugin_data()) {
        OptionParser parser(plugin.key, *this, Predefinitions(), true, true);
        plugin.doc_factory(parser);
    }
}

void Registry::insert_plugin_types(const RawRegistry &raw_registry,
                                   vector<string> &errors) {
    unordered_map<string, vector<type_index>> occurrences_names;
    unordered_map<type_index, vector<string>> occurrences_types;
    unordered_map<string, vector<string>> occurrences_predefinition;
    for (const PluginTypeInfo &plugin_type_info :
         raw_registry.get_plugin_type_data()) {
        occurrences_names[plugin_type_info.type_name].push_back(plugin_type_info.type);
        occurrences_types[plugin_type_info.type].push_back(plugin_type_info.type_name);
        bool predefine_error = false;
        for (const string &predefinition_key :
             {plugin_type_info.predefinition_key, plugin_type_info.alias}) {
            if (!predefinition_key.empty()) {
                occurrences_predefinition[predefinition_key].push_back(
                    plugin_type_info.type_name);
                if (occurrences_predefinition[predefinition_key].size() > 1)
                    predefine_error = true;
            }
        }

        if (occurrences_names[plugin_type_info.type_name].size() == 1 &&
            occurrences_types[plugin_type_info.type].size() == 1 &&
            !predefine_error) {
            insert_type_info(plugin_type_info);
        }
    }

    for (auto it : occurrences_names) {
        if (it.second.size() > 1) {
            errors.push_back(
                "Multiple definitions for PluginTypePlugin " + it.first +
                " (types: " +
                utils::join(utils::map_vector<string>(
                                it.second,
                                [](const type_index &type) {return type.name();}),
                            ", ") + ")");
        }
    }
    for (auto it : occurrences_types) {
        if (it.second.size() > 1) {
            errors.push_back(
                "Multiple definitions for PluginTypePlugin of type " +
                string(it.first.name()) +
                " (names: " + utils::join(it.second, ", ") + ")");
        }
    }
    for (auto it : occurrences_predefinition) {
        if (it.second.size() > 1) {
            errors.push_back("Multiple PluginTypePlugins use the predefinition "
                             "key " + it.first + " (types: " +
                             utils::join(it.second, ", ") + ")");
        }
    }
}

void Registry::insert_plugin_groups(const RawRegistry &raw_registry,
                                    vector<string> &errors) {
    unordered_map<string, int> occurrences;
    for (const PluginGroupInfo &pgi : raw_registry.get_plugin_group_data()) {
        ++occurrences[pgi.group_id];
        if (occurrences[pgi.group_id] == 1) {
            insert_group_info(pgi);
        }
    }

    for (auto it : occurrences) {
        if (it.second > 1) {
            errors.push_back("Multiple definitions (" + to_string(it.second) +
                             ") for PluginGroupPlugin " + it.first);
        }
    }
}

void Registry::insert_plugins(const RawRegistry &raw_registry,
                              vector<string> &errors) {
    unordered_map<string, vector<type_index>> occurrences;
    for (const RawPluginInfo &plugin : raw_registry.get_plugin_data()) {
        bool error = false;
        if (!plugin.group.empty() && !plugin_group_infos.count(plugin.group)) {
            errors.push_back(
                "No PluginGroupPlugin with name " + plugin.group +
                " for Plugin " + plugin.key +
                " of type " + plugin.type.name());
            error = true;
        }
        if (!plugin_type_infos.count(plugin.type)) {
            errors.push_back(
                "No PluginTypePlugin of type " + string(plugin.type.name()) +
                " for Plugin " + plugin.key + " (can be spurious if associated "
                "PluginTypePlugin is defined multiple times)");
            error = true;
        }
        occurrences[plugin.key].push_back(plugin.type);
        if (occurrences[plugin.key].size() != 1) {
            // Error message generated below.
            error = true;
        }
        if (!error) {
            insert_plugin(plugin.key, plugin.factory, plugin.group,
                          plugin.type_name_factory, plugin.type);
        }
    }

    for (auto it : occurrences) {
        if (it.second.size() > 1) {
            errors.push_back(
                "Multiple definitions for Plugin " + it.first + " (types: " +
                utils::join(utils::map_vector<string>(
                                it.second,
                                [](const type_index &type) {return type.name();}),
                            ", ") +
                ")");
        }
    }
}

void Registry::insert_type_info(const PluginTypeInfo &info) {
    assert(!plugin_type_infos.count(info.type));
    for (const string &predefinition_key : {info.predefinition_key, info.alias}) {
        if (!predefinition_key.empty()) {
            assert(!is_predefinition(predefinition_key));
            predefinition_functions[predefinition_key] = info.predefinition_function;
        }
    }
    plugin_type_infos.insert(make_pair(info.type, info));
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
    assert(!plugin_group_infos.count(info.group_id));
    plugin_group_infos.insert(make_pair(info.group_id, info));
}

const PluginGroupInfo &Registry::get_group_info(const string &group) const {
    if (!plugin_group_infos.count(group)) {
        ABORT("attempt to retrieve non-existing group info from registry: " +
              string(group));
    }
    return plugin_group_infos.at(group);
}

void Registry::insert_plugin(
    const string &key, const Any &factory,
    const string &group,
    const PluginTypeNameGetter &type_name_factory,
    const type_index &type) {
    assert(!plugin_infos.count(key));
    assert(!plugin_factories.count(type) || !plugin_factories[type].count(key));

    PluginInfo doc;
    doc.key = key;
    // Plugin names can be set with document_synopsis. Otherwise, we use the key.
    doc.name = key;
    doc.type_name = type_name_factory(*this);
    doc.synopsis = "";
    doc.group = group;
    doc.hidden = false;

    plugin_infos.insert(make_pair(key, doc));
    plugin_factories[type][key] = factory;
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

bool Registry::is_predefinition(const string &key) const {
    return predefinition_functions.count(key);
}

void Registry::handle_predefinition(
    const string &key, const string &arg, Predefinitions &predefinitions,
    bool dry_run) {
    predefinition_functions.at(key)(arg, *this, predefinitions, dry_run);
}
}
