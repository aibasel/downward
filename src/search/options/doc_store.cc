#include "doc_store.h"

#include "registries.h"

#include <algorithm>

using namespace std;

namespace options {
string ArgumentInfo::get_type_name() const {
    return PluginTypeRegistry::instance()->get(type).get_type_name();
}

void DocStore::register_object(const string &key, const string &type) {
    registered[key] = DocStruct();
    registered[key].type = type;
    registered[key].full_name = key;
    registered[key].synopsis = "";
    registered[key].hidden = false;
}

void DocStore::add_arg(
    const string &key,
    const string &arg_name,
    const string &help,
    const type_info &type,
    const string &default_value,
    const Bounds &bounds,
    const ValueExplanations &value_explanations) {
    registered[key].arg_help.emplace_back(
        arg_name, help, type, default_value, bounds, value_explanations);
}

void DocStore::add_value_explanations(
    const string &key,
    const string &arg_name,
    const ValueExplanations &value_explanations) {
    vector<ArgumentInfo> &args = registered[key].arg_help;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].key == arg_name) {
            args[i].value_explanations = value_explanations;
            break;
        }
    }
}

void DocStore::set_synopsis(
    const string &key, const string &name, const string &description) {
    registered[key].full_name = name;
    registered[key].synopsis = description;
}

void DocStore::add_property(
    const string &key, const string &name, const string &description) {
    registered[key].property_help.emplace_back(name, description);
}

void DocStore::add_feature(
    const string &key, const string &feature, const string &description) {
    registered[key].support_help.emplace_back(feature, description);
}

void DocStore::add_note(
    const string &key, const string &name, const string &description, bool long_text) {
    registered[key].notes.emplace_back(name, description, long_text);
}

void DocStore::hide(const string &key) {
    registered[key].hidden = true;
}

bool DocStore::contains(const string &key) {
    return registered.find(key) != registered.end();
}

DocStruct DocStore::get(const string &key) {
    return registered[key];
}

vector<string> DocStore::get_keys() {
    vector<string> keys;
    for (const auto it : registered) {
        keys.push_back(it.first);
    }
    return keys;
}

vector<string> DocStore::get_types() {
    vector<string> types;
    for (const auto it : registered) {
        /* Entries for the category itself have an empty type. We filter
           duplicates but keep the original ordering by key. */
        if (!it.second.type.empty() &&
            find(types.begin(), types.end(), it.second.type) == types.end()) {
            types.push_back(it.second.type);
        }
    }
    return types;
}
}
