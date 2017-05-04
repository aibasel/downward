#include "doc_store.h"

#include "option_parser.h"
#include "parse_tree.h"
#include "registries.h"
#include "type_namer.h"

#include <algorithm>

using namespace std;

namespace options {
string ArgumentInfo::get_type_name() const {
    return type_name;
}

void DocStore::register_plugin_type(const string &type_name, const string &synopsis) {
    PluginTypeDocumentation doc;
    doc.type_name = type_name;
    doc.synopsis = synopsis;
    plugin_type_docs.push_back(move(doc));
}

void DocStruct::fill_docs() {
    ParseTree parse_tree;
    parse_tree.insert(parse_tree.begin(), ParseNode(full_name));
    OptionParser parser(parse_tree, true);
    parser.set_help_mode(true);
    doc_factory(parser);
}

string DocStruct::get_type_name() const {
    return type_name_factory();
}

void DocStore::register_plugin(const string &key, DocFactory doc_factory, TypeNameFactory type_name_factory) {
    DocStruct doc = DocStruct();
    doc.doc_factory = doc_factory;
    doc.type_name_factory = type_name_factory;
    doc.full_name = key;
    doc.synopsis = "";
    doc.hidden = false;
    registered.insert(make_pair(key, doc));
}

void DocStore::add_arg(
    const string &key,
    const string &arg_name,
    const string &help,
    const string &type_name,
    const string &default_value,
    const Bounds &bounds,
    const ValueExplanations &value_explanations) {
    registered.at(key).arg_help.emplace_back(
        arg_name, help, type_name, default_value, bounds, value_explanations);
}

void DocStore::add_value_explanations(
    const string &key,
    const string &arg_name,
    const ValueExplanations &value_explanations) {
    vector<ArgumentInfo> &args = registered.at(key).arg_help;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].key == arg_name) {
            args[i].value_explanations = value_explanations;
            break;
        }
    }
}

void DocStore::set_synopsis(
    const string &key, const string &name, const string &description) {
    registered.at(key).full_name = name;
    registered.at(key).synopsis = description;
}

void DocStore::add_property(
    const string &key, const string &name, const string &description) {
    registered.at(key).property_help.emplace_back(name, description);
}

void DocStore::add_feature(
    const string &key, const string &feature, const string &description) {
    registered.at(key).support_help.emplace_back(feature, description);
}

void DocStore::add_note(
    const string &key, const string &name, const string &description, bool long_text) {
    registered.at(key).notes.emplace_back(name, description, long_text);
}

void DocStore::hide(const string &key) {
    registered.at(key).hidden = true;
}

bool DocStore::contains(const string &key) {
    return registered.find(key) != registered.end();
}

DocStruct &DocStore::get(const string &key) {
    return registered.at(key);
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
        const string type_name = it.second.get_type_name();
        if (!type_name.empty() &&
            find(types.begin(), types.end(), type_name) == types.end()) {
            types.push_back(type_name);
        }
    }
    return types;
}

const std::vector<PluginTypeDocumentation> &DocStore::get_plugin_type_docs() const {
    return plugin_type_docs;
}
}
