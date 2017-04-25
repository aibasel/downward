#include "doc_store.h"

#include <algorithm>

using namespace std;

namespace options {
void DocStore::register_object(string k, const string &type) {
    transform(k.begin(), k.end(), k.begin(), ::tolower); //k to lowercase
    registered[k] = DocStruct();
    registered[k].type = type;
    registered[k].full_name = k;
    registered[k].synopsis = "";
    registered[k].hidden = false;
}

void DocStore::add_arg(const string &k,
                       const string &arg_name,
                       const string &help,
                       const string &type,
                       const string &default_value,
                       Bounds bounds,
                       ValueExplanations value_explanations) {
    registered[k].arg_help.push_back(
        ArgumentInfo(arg_name, help, type, default_value, bounds,
                     value_explanations));
}

void DocStore::add_value_explanations(const string &k,
                                      const string &arg_name,
                                      ValueExplanations value_explanations) {
    vector<ArgumentInfo> &args = registered[k].arg_help;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].kwd.compare(arg_name) == 0) {
            args[i].value_explanations = value_explanations;
            break;
        }
    }
}


void DocStore::set_synopsis(
    const string &k, const string &name, const string &description) {
    registered[k].full_name = name;
    registered[k].synopsis = description;
}

void DocStore::add_property(
    const string &k, const string &name, const string &description) {
    registered[k].property_help.push_back(PropertyInfo(name, description));
}

void DocStore::add_feature(
    const string &k, const string &feature, const string &description) {
    registered[k].support_help.push_back(LanguageSupportInfo(feature,
                                                             description));
}

void DocStore::add_note(
    const string &k, const string &name, const string &description, bool long_text) {
    registered[k].notes.push_back(NoteInfo(name, description, long_text));
}

void DocStore::hide(const string &k) {
    registered[k].hidden = true;
}


bool DocStore::contains(const string &k) {
    return registered.find(k) != registered.end();
}

DocStruct DocStore::get(const string &k) {
    return registered[k];
}

vector<string> DocStore::get_keys() {
    vector<string> keys;
    for (map<string, DocStruct>::iterator it =
             registered.begin();
         it != registered.end(); ++it) {
        keys.push_back(it->first);
    }
    return keys;
}

vector<string> DocStore::get_types() {
    vector<string> types;
    for (map<string, DocStruct>::iterator it =
             registered.begin();
         it != registered.end(); ++it) {
        if (find(types.begin(), types.end(), it->second.type)
            == types.end()) {
            types.push_back(it->second.type);
        }
    }
    return types;
}
}
