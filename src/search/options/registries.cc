#include "registries.h"

#include "errors.h"
#include "option_parser.h"

#include "../utils/collections.h"
#include "../utils/strings.h"

#include <iostream>
#include <typeindex>

using namespace std;
using utils::ExitCode;

namespace options {
static void print_initialization_errors_and_exit(const vector<string> &errors) {
    cerr << "Plugin initialization errors:\n" + utils::join(errors, "\n") << endl;
    exit_with_demangling_hint(ExitCode::SEARCH_CRITICAL_ERROR, "[TYPE]");
}

template<typename Key, typename Type>
static void generate_duplicate_errors(
    const unordered_map<Key, Type> &occurrences, vector<string> &errors,
    function<bool(const Type &obj)> duplicate_condition,
    function<string(const Key &first, const Type &second)> message_generator) {
    vector<string> name_clash_errors;

    typename unordered_map<Key, Type>::const_iterator it;
    for (it = occurrences.begin(); it != occurrences.end(); ++it) {
        if (duplicate_condition(it->second)) {
            name_clash_errors.push_back(
                message_generator(it->first, it->second));
        }
    }
    sort(name_clash_errors.begin(), name_clash_errors.end());
    errors.insert(errors.end(), name_clash_errors.begin(), name_clash_errors.end());
}

Registry::Registry(const RawRegistry &collection) {
    vector<string> errors;
    collect_plugin_types(collection, errors);
    collect_plugin_groups(collection, errors);
    collect_plugins(collection, errors);
    if (!errors.empty()) {
        print_initialization_errors_and_exit(errors);
    }
}

void Registry::collect_plugin_types(const RawRegistry &collection,
                                    vector<string> &errors) {
    unordered_map<string, vector<type_index>> occurrences_names;
    unordered_map<type_index, vector<string>> occurrences_types;
    for (const PluginTypeInfo &pti : collection.get_plugin_type_data()) {
        occurrences_names[pti.get_type_name()].push_back(pti.get_type());
        occurrences_types[pti.get_type()].push_back(pti.get_type_name());
        if (occurrences_names[pti.get_type_name()].size() == 1 &&
            occurrences_types[pti.get_type()].size() == 1) {
            insert_type_info(pti);
        }
    }

    generate_duplicate_errors<string, vector<type_index>>(
        occurrences_names, errors,
        [](const vector<type_index> &types) {return types.size() > 1;},
        [](const string &type_name, const vector<type_index> &types) {
            return "Multiple PluginTypePlugins with the same name: " +
            type_name + " (types: " +
            utils::join(
                utils::map_vector<string>(
                    types,
                    [](const type_index &type) {return type.name();}),
                ", ") + ")";
        });

    generate_duplicate_errors<type_index, vector<string>>(
        occurrences_types, errors,
        [](const vector<string> &names) {return names.size() > 1;},
        [](const type_index &type, const vector<string> &names) {
            return "Multiple PluginTypePlugins with the same type: " +
            string(type.name()) + " (names: " + utils::join(names, ", ") + ")";
        });
}

void Registry::collect_plugin_groups(const RawRegistry &collection,
                                     vector<string> &errors) {
    unordered_map<string, int> occurrences;
    for (const PluginGroupInfo &pgi : collection.get_plugin_group_data()) {
        occurrences[pgi.group_id]++;
        if (occurrences[pgi.group_id] == 1) {
            insert_group_info(pgi);
        }
    }

    generate_duplicate_errors<string, int>(
        occurrences, errors,
        [](const int &count) {return count > 1;},
        [](const string &first, const int &count) {
            return "Multiple PluginGroupPlugins: " + first +
            " (count: " + to_string(count) + ")";
        });
}

void Registry::collect_plugins(const RawRegistry &collection,
                               vector<string> &errors) {
    vector<string> other_plugin_errors;
    unordered_map<string, vector<type_index>> occurrences;
    for (const RawPluginInfo &plugin : collection.get_plugin_data()) {
        //string key = info.key;
        //Any factory = info.factory;
        //string group = info.group;
        //PluginTypeNameGetter type_name_factory = info.type_name_factory;
        //DocFactory doc_factory = info.doc_factory;
        //type_index type = info.type;

        bool error = false;
        if (!plugin.group.empty() && !plugin_group_infos.count(plugin.group)) {
            other_plugin_errors.push_back(
                "Missing PluginGroupPlugin for Plugin " + plugin.key +
                " of type " + plugin.type.name() + ": " + plugin.group);
            error = true;
        }
        if (!plugin_type_infos.count(plugin.type)) {
            other_plugin_errors.push_back("Missing PluginTypePlugin for "
                                          "Plugin " + plugin.key + ": " +
                                          plugin.type.name());
            error = true;
        }
        occurrences[plugin.key].push_back(plugin.type);
        if (occurrences[plugin.key].size() != 1) {
            // Error message generated in generate_duplicate_errors<...> ...
            error = true;
        }
        if (!error) {
            insert_plugin(plugin.key, plugin.factory, plugin.type_name_factory,
                          plugin.doc_factory, plugin.group, plugin.type);
        }
    }

    generate_duplicate_errors<string, vector<type_index>>(
        occurrences, errors,
        [](const vector<type_index> &types) {return types.size() > 1;},
        [](const string &type_name, const vector<type_index> &types) {
            return "Multiple Plugins: " + type_name + " (types: " +
            utils::join(
                utils::map_vector<string>(
                    types,
                    [](const type_index &type) {return type.name();}),
                ", ") +
            ")";
        });

    sort(other_plugin_errors.begin(), other_plugin_errors.end());
    errors.insert(errors.end(), other_plugin_errors.begin(), other_plugin_errors.end());
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
    const PluginTypeNameGetter &type_name_factory,
    const DocFactory &doc_factory, const string &group, const type_index &type) {
    insert_plugin_info(key, doc_factory, type_name_factory, group);
    insert_factory(key, factory, type);
}

void Registry::insert_factory(const string &key, const Any &factory,
                              const type_index &type) {
    if (plugin_factories.count(type) && plugin_factories[type].count(key)) {
        ABORT("duplicate key in registry: " + key + "\n");
    }
    plugin_factories[type][key] = factory;
}

void Registry::insert_plugin_info(
    const string &key,
    const DocFactory &doc_factory,
    const PluginTypeNameGetter &type_name_factory,
    const string &group) {
    if (plugin_infos.count(key)) {
        ABORT("Registry already contains a plugin with name \"" + key + "\"");
    }
    PluginInfo doc;
    doc.key = key;
    // Plugin names can be set with document_synopsis. Otherwise, we use the key.
    doc.name = key;
    doc.type_name = type_name_factory(*this);
    doc.synopsis = "";
    doc.group = group;
    doc.hidden = false;

    plugin_infos.insert(make_pair(key, doc));
    OptionParser parser(key, *this, Predefinitions(), true, true);
    doc_factory(parser);
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
