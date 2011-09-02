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

void DocStore::add_value_explanations(string k,
                                      string arg_name,
                                      ValueExplanations value_explanations) {
    vector<ArgumentInfo> &args = registered[k].arg_help;
    for(int i(0); i != args.size(); ++i) {
        if (args[i].kwd.compare(arg_name) == 0) {
            args[i].value_explanations = value_explanations;
            break;
        }
    }
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


DocPrinter::DocPrinter(ostream &out)
    :os(out) {
}

DocPrinter::~DocPrinter() {
}


void DocPrinter::print_all() {
    DocStore *ds = DocStore::instance();
    vector<string> types = ds->get_types();
    for(size_t n(0); n != types.size(); ++n) {
        print_category(types[n]);
    }        
}

void DocPrinter::print_category(string category_name) {
    print_category_header(category_name);
    DocStore *ds = DocStore::instance();
    vector<string> keys = ds->get_keys();
    for(size_t i(0); i != keys.size(); ++i) {
        DocStruct info = ds->get(keys[i]);
        if(info.type.compare(category_name) != 0)
            continue;
        print_element(keys[i], info);
    }
    print_category_footer();
}

Txt2TagsPrinter::Txt2TagsPrinter(ostream &out)
    :DocPrinter(out) {
}

Txt2TagsPrinter::~Txt2TagsPrinter() {
}

void Txt2TagsPrinter::print_element(string call_name, DocStruct &info) {
    os << "== " << info.full_name << " ==" << endl
         << info.synopsis << endl
         << "``` " << call_name << "(";
    for(size_t j(0); j != info.arg_help.size(); ++j){
        ArgumentInfo arg = info.arg_help[j];
        os << arg.kwd;
        if(arg.default_value.compare("") != 0){
            os << " = " << arg.default_value;
        }
        if(j != info.arg_help.size() - 1)
            os << ", ";
    }
    os << ")" << endl << endl << endl;
    for(size_t j(0); j != info.arg_help.size(); ++j){
        ArgumentInfo arg = info.arg_help[j];
        os << "- //" << arg.kwd << "// (" 
             << arg.type_name << "): "
             << arg.help << endl;
        if(!arg.value_explanations.empty()) {
            for(size_t k(0); k != arg.value_explanations.size(); ++k) {
                pair<string, string> explanation = 
                    arg.value_explanations[k];
                os << " - ``" << explanation.first << "``: "
                     << explanation.second << endl;
            }
        }
    }
    //notes:
    for(size_t j(0); j != info.notes.size(); ++j) {
        NoteInfo note = info.notes[j];
        os << "**" << note.name << ":** "
             << note.description << endl << endl;
    }            
    //language features:
    if(!info.support_help.empty()) {
        os << "Language features supported:" << endl;
    }
    for(size_t j(0); j != info.support_help.size(); ++j) {
        LanguageSupportInfo ls = info.support_help[j];
        os << "- **" << ls.feature << ":** "
             << ls.description << endl;
    }
    //properties:
    if(!info.property_help.empty()) {
        os << "Properties:" << endl;
    }
    for(size_t j(0); j != info.property_help.size(); ++j) {
        PropertyInfo p = info.property_help[j];
        os << "- **" << p.property << ":** "
             << p.description << endl;
    }   
}

void Txt2TagsPrinter::print_category_header(string category_name) {
    os << ">>>>CATEGORY: " << category_name << "s" << "<<<<" << endl;
}

void Txt2TagsPrinter::print_category_footer() {
    os << endl
       << ">>>>CATEGORYEND<<<<" << endl;
}

PlainPrinter::PlainPrinter(ostream &out)
    :DocPrinter(out) {
}

PlainPrinter::~PlainPrinter() {
}

void PlainPrinter::print_element(string call_name, DocStruct &info) {
    os << info.full_name << " is a " << info.type << "." << endl
         << info.synopsis << endl
         << "Usage:" << endl
         << call_name << "(";
    for(size_t j(0); j != info.arg_help.size(); ++j){
        os << info.arg_help[j].kwd;
        if(info.arg_help[j].default_value.compare("") != 0){
            os << " = " << info.arg_help[j].default_value;
        }
        if(j != info.arg_help.size() - 1)
            os << ", ";
    }
    os << ")" << endl;
    for(size_t j(0); j != info.arg_help.size(); ++j){
        ArgumentInfo arg = info.arg_help[j];
        os << arg.kwd
             << " (" << arg.type_name << "):"
             << arg.help << endl;
        if(!arg.value_explanations.empty()) {
            for(size_t k(0); k != arg.value_explanations.size(); ++k) {
                pair<string, string> explanation = 
                    arg.value_explanations[k];
                os << " - " << explanation.first << ": "
                     << explanation.second << endl;
            }
        }
    }
    os << endl;
}

void PlainPrinter::print_category_header(string category_name) {
    os << "Help for " << category_name << "s" << endl << endl;
}

void PlainPrinter::print_category_footer() {
    os << endl;
}


