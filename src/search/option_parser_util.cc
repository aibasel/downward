#include <algorithm>
#include <string>
#include <vector>
#include "option_parser_util.h"


using namespace std;

void DocStore::register_object(string k, string type) {
    transform(k.begin(), k.end(), k.begin(), ::tolower); //k to lowercase
    registered[k] = DocStruct();
    registered[k].type = type;
    registered[k].full_name = k;
    registered[k].synopsis = "";
}


void DocStore::add_arg(string k,
                       string arg_name,
                       string help,
                       string type,
                       string default_value,
                       ValueExplanations value_explanations) {
    registered[k].arg_help.push_back(
        ArgumentInfo(arg_name, help, type, default_value,
                     value_explanations));
}

void DocStore::set_synopsis(string k, 
                            string name, string description) {
    registered[k].full_name = name;
    registered[k].synopsis = description;
}

void DocStore::add_property(string k,
                            string name, string description) {
    registered[k].property_help.push_back(PropertyInfo(name, description));
}

void DocStore::add_feature(string k,
                           string feature, string description) {
    registered[k].support_help.push_back(LanguageSupportInfo(feature,
                                                             description));
}

void DocStore::add_note(string k,
                        string name, string description) {
    registered[k].notes.push_back(NoteInfo(name, description));
}
                      

bool DocStore::contains(string k) {
    return registered.find(k) != registered.end();
}

DocStruct DocStore::get(string k) {
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
        if(find(types.begin(), types.end(), it->second.type)
           == types.end()){
            types.push_back(it->second.type);
        }   
    }
    return types;
}


