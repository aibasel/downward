#include "doc_store.h"

#include "option_parser.h"

#include <algorithm>

using namespace std;

namespace options {
void DocStruct::fill_docs() {
    OptionParser parser(full_name, true);
    parser.set_help_mode(true);
    doc_factory(parser);
}

string DocStruct::get_type_name() const {
    return type_name_factory();
}


void DocStore::register_plugin(
    const string &key, DocFactory doc_factory, PluginTypeNameGetter type_name_factory) {
    DocStruct doc;
    doc.doc_factory = doc_factory;
    doc.type_name_factory = type_name_factory;
    doc.full_name = key;
    doc.synopsis = "";
    doc.hidden = false;
    registered[key] = doc;
}

void DocStore::add_arg(
    const string &key,
    const string &arg_name,
    const string &help,
    const string &type_name,
    const string &default_value,
    const Bounds &bounds,
    const ValueExplanations &value_explanations) {
    get(key).arg_help.emplace_back(
        arg_name, help, type_name, default_value, bounds, value_explanations);
}

void DocStore::add_value_explanations(
    const string &key,
    const string &arg_name,
    const ValueExplanations &value_explanations) {
    vector<ArgumentInfo> &args = get(key).arg_help;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].key == arg_name) {
            args[i].value_explanations = value_explanations;
            break;
        }
    }
}

void DocStore::set_synopsis(
    const string &key, const string &name, const string &description) {
    get(key).full_name = name;
    get(key).synopsis = description;
}

void DocStore::add_property(
    const string &key, const string &name, const string &description) {
    get(key).property_help.emplace_back(name, description);
}

void DocStore::add_feature(
    const string &key, const string &feature, const string &description) {
    get(key).support_help.emplace_back(feature, description);
}

void DocStore::add_note(
    const string &key, const string &name, const string &description, bool long_text) {
    get(key).notes.emplace_back(name, description, long_text);
}

void DocStore::hide(const string &key) {
    get(key).hidden = true;
}

bool DocStore::contains(const string &key) {
    return registered.find(key) != registered.end();
}

DocStruct &DocStore::get(const string &key) {
    /* Use at() to get an error when trying to modify a plugin that has not been
       registered with register_plugin. */
    return registered.at(key);
}

vector<string> DocStore::get_keys() {
    vector<string> keys;
    for (const auto it : registered) {
        keys.push_back(it.first);
    }
    return keys;
}
}
